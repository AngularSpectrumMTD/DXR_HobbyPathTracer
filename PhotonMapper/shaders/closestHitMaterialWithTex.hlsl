#include "photonGathering.hlsli"

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

VertexPNT GetVertex(TriangleIntersectionAttributes attrib, inout bool isNoTexture)
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
    v.Position = ComputeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = ComputeInterpolatedAttributeF3(normalTbl, attrib.barys);
    v.UV = ComputeInterpolatedAttributeF2(texcoordTbl, attrib.barys);

    v.Normal = normalize(v.Normal);
    return v;
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

[shader("closesthit")]
void materialWithTexClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (payload.isShadowRay == 1)
    {
        payload.isShadowMiss = 0;
        return;
    }

    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }

    bool isNoTexture = false;
    VertexPNT vtx = GetVertex(attrib, isNoTexture);
    
    float4 diffuseTexColor = 1.xxxx;

    bool isIgnoreHit = false;
    getTexColor(diffuseTexColor, isIgnoreHit, isNoTexture, vtx.UV);
    
    if (!isIgnoreHit)
    {
        depthPositionNormalStore(payload, vtx.Normal);
    }

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());
    
    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;

    if (!isIgnoreHit)
    {
        nextRay.Direction = 0.xxx;
        SurafceShading(currentMaterial, vtx.Normal, nextRay, payload.energy);
        LightSample lightSample;
        SampleLight(bestFitWorldPosition, lightSample);
        const float3 lightIrr = (dot(lightSample.normal, lightSample.direction) < 0) ? lightSample.emission / lightSample.pdf : float3(0, 0, 0);
        const float shadowCoef = isShadow(bestFitWorldPosition, lightSample) ? 0 : 1;
        payload.color += payload.energy * (currentMaterial.emission.xyz + shadowCoef * lightIrr * currentMaterial.roughness + photonGather(bestFitWorldPosition, payload.eyeDir, bestFitWorldNormal));
    }
    else
    {
        nextRay.TMin = 0.001;
        nextRay.TMax = 10000;
        nextRay.Direction = WorldRayDirection();
    }

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceRay(
            gRtScene,
            flags,
            rayMask,
            0, // ray index
            1, // MultiplierForGeometryContrib
            0, // miss index
            nextRay,
            payload);
}

[shader("closesthit")]
void materialWithTexStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    bool isNoTexture = false;
    VertexPNT vtx = GetVertex(attrib, isNoTexture);
    
    float4 diffuseTexColor = 1.xxxx;

    bool isIgnoreHit = false;
    getTexColor(diffuseTexColor, isIgnoreHit, isNoTexture, vtx.UV);

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
   
    nextRay.TMin = 0.001;
    nextRay.TMax = 10000;
    nextRay.Direction = WorldRayDirection();
    if (!isIgnoreHit)
    {
        nextRay.Direction = 0.xxx;
        SurafceShading(currentMaterial, vtx.Normal, nextRay, payload.throughput, payload.lambdaNM);
    }

    if (!isIgnoreHit && isPhotonStoreRequired(currentMaterial))
    {
        storePhoton(payload);
    }
    else
    {
        RAY_FLAG flags = RAY_FLAG_NONE;
        uint rayMask = 0xff;
        TraceRay(
            gRtScene,
            flags,
            rayMask,
            0, // ray index
            1, // MultiplierForGeometryContrib
            0, // miss index
            nextRay,
            payload);
    }
}

[shader("closesthit")]
void materialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    // no op
}