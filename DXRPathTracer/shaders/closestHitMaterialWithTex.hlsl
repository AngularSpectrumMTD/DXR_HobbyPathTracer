﻿#include "photonGathering.hlsli"

struct VertexPNT
{
    float3 Position;
    float3 Normal;
    float2 UV;
};

ConstantBuffer<MaterialParams> constantBuffer : register(b0, space1);
StructuredBuffer<uint>   indexBuffer : register(t0,space1);
StructuredBuffer<VertexPNT> vertexBuffer : register(t1, space1);
Texture2D<float4> diffuseTex : register(t2, space1);
Texture2D<float> alphaMask: register(t3, space1);

VertexPNT getVertex(TriangleIntersectionAttributes attrib, inout bool isNoTexture)
{
    VertexPNT v = (VertexPNT) 0;
    uint start = PrimitiveIndex() * 3; // Triangle List.

    float3 positionTbl[3], normalTbl[3];
    float2 texcoordTbl[3];
    
    isNoTexture = false;
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
        texcoordTbl[i] = vertexBuffer[index].UV;
        if (texcoordTbl[i].x == 0xff && texcoordTbl[i].y == 0xff)
        {
            isNoTexture = true;
        }
    }
    v.Position = computeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = computeInterpolatedAttributeF3(normalTbl, attrib.barys);
    v.UV = computeInterpolatedAttributeF2(texcoordTbl, attrib.barys);

    v.Normal = normalize(v.Normal);
    return v;
}

float3 getGeometricNormal(TriangleIntersectionAttributes attrib)
{
    VertexPNT v = (VertexPNT)0;
    uint start = PrimitiveIndex() * 3;
    
    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }
    
    v.Position = computeInterpolatedAttributeF3(positionTbl, attrib.barys);
    return normalize(cross(positionTbl[1] - positionTbl[0], positionTbl[2] - positionTbl[0]));
}

void getTexColor(out float4 diffuseTexColor, out bool isIgnoreHit, in bool isNoTexture, in float2 UV)
{
    diffuseTexColor = 0.xxxx;
    float alpMask = 1;
    float2 diffTexSize = 0.xx;
    diffuseTex.GetDimensions(diffTexSize.x, diffTexSize.y);
    float2 alphaMaskSize = 0.xx;
    alphaMask.GetDimensions(alphaMaskSize.x, alphaMaskSize.y);
    const bool isAlphaMaskInvalid = (alphaMaskSize.x == 1 && alphaMaskSize.y == 1);
    bool isTexInvalid = (diffTexSize.x == 1 && diffTexSize.y == 1);

    if (!isNoTexture && !isTexInvalid)
    {
        diffuseTexColor = diffuseTex.SampleLevel(gSampler, UV, 0.0);
        alpMask = alphaMask.SampleLevel(gSampler, UV, 0.0);
        if (diffuseTexColor.r > 0 && diffuseTexColor.g == 0 && diffuseTexColor.b == 0)//1 channel
        {
            diffuseTexColor.rgb = diffuseTexColor.rrr;
        }
    }
    else
    {
        diffuseTexColor = 1.xxxx;
        alpMask = 1;
        isIgnoreHit = false;
        return;
    }
    
    isIgnoreHit = (diffuseTexColor.a < 1) || (!isAlphaMaskInvalid && alpMask < 1);
}

void editMaterial(inout MaterialParams mat)
{
    //I can't decide these params to recognize material as glass
    if (mat.metallic > 0.3 && mat.roughness > 0.5)
    {
        mat.transColor = 1.xxxx;
        mat.transRatio = 0;
        mat.albedo = 1.xxxx;
    }
    else
    {
        mat.roughness = 0.03;
        mat.transColor = 1.xxxx;
        mat.transRatio = 1;
        mat.metallic = 0;
        mat.albedo = 1.xxxx;

        //mat.emission = float4(1,1,0,0);//test
    }
}

MaterialParams getCurrentMaterial(TriangleIntersectionAttributes attrib, inout VertexPNT vtx, inout bool isIgnoreHit)
{
    bool isNoTexture = false;
    vtx = getVertex(attrib, isNoTexture);
    
    float4 diffuseTexColor = 1.xxxx;

    getTexColor(diffuseTexColor, isIgnoreHit, isNoTexture, vtx.UV);

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);

    //test metallic surface
    if(isUseMetallicTest())
    {
        currentMaterial.roughness = 0.2;
        currentMaterial.metallic = 0.5;
    }

    //recognize as glass
    if (length(currentMaterial.albedo) == 0 && !isNoTexture)
    {
        editMaterial(currentMaterial);
    }

    return currentMaterial;
}

[shader("anyhit")]
void anyHitWithTex(inout Payload payload, TriangleIntersectionAttributes attrib) {
    bool isNoTexture = false;
    float4 diffuseTexColor = 0.xxxx;
    bool isIgnoreHit = false;
    VertexPNT vtx = getVertex(attrib, isNoTexture);
    getTexColor(diffuseTexColor, isIgnoreHit, isNoTexture, vtx.UV);

    if(isIgnoreHit)
    {
        IgnoreHit();
    }

    if(isSSSRay(payload))
    {
        payload.hittedCount++;
        payload.SSSnormal = vtx.Normal;
        //payload.SSSnormal = getGeometricNormal(attrib);
        payload.T = RayTCurrent();
        IgnoreHit();
    }
}

[shader("closesthit")]
void materialWithTexClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isShadowRay(payload))
    {
        setVisibility(payload, false);
        return;
    }

    if(isSSSRay(payload))
    {
        bool isNoTexture = false;
        payload.SSSnormal = getVertex(attrib, isNoTexture).Normal;
        payload.T = RayTCurrent();
        return;
    }

    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }
    uint primitiveIndex = PrimitiveIndex();
    uint instanceIndex = InstanceIndex();

    VertexPNT vtx;

    bool isIgnoreHitDummy = false;
    MaterialParams currentMaterial = getCurrentMaterial(attrib, vtx, isIgnoreHitDummy);

    float3 caustics = 0.xxx;
    if(payload.recursive <= 2)
    {
        caustics = accumulatePhoton(mul(float4(vtx.Position, 1), ObjectToWorld4x3()), mul(vtx.Normal, (float3x3)ObjectToWorld4x3()));
    }

    RayDesc nextRay;
    bool isTerminate = shadeAndSampleRay(vtx.Normal, vtx.Position, getGeometricNormal(attrib), payload, currentMaterial, nextRay, caustics);
    if(isTerminate)
    {
        return;
    }
    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceDefaultRay(flags, rayMask, nextRay, payload);
}

[shader("closesthit")]
void materialWithTexStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }
    uint primitiveIndex = PrimitiveIndex();
    uint instanceIndex = InstanceIndex();

    VertexPNT vtx;
    bool isIgnoreHitDummy = false;
    MaterialParams currentMaterial = getCurrentMaterial(attrib, vtx, isIgnoreHitDummy);
    primarySurfaceHasHighPossibilityCausticsGenerate(currentMaterial, payload);
    float3 surfaceNormal = vtx.Normal;

    float3 scatterPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = scatterPosition;
   
    nextRay.TMin = RAY_MIN_T;
    nextRay.TMax = RAY_MAX_T;
    nextRay.Direction = WorldRayDirection();
    sampleBSDF(currentMaterial, surfaceNormal, nextRay, payload.throughputU32, payload.randomSeed, payload.lambdaNM);

    if (isPhotonStoreRequired(currentMaterial, payload))
    {
        storePhoton(payload);
    }
    else
    {
        RAY_FLAG flags = RAY_FLAG_NONE;
        uint rayMask = 0xff;
        TraceDefaultPhoton(flags, rayMask, nextRay, payload);
    }
}

[shader("closesthit")]
void materialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    // no op
}