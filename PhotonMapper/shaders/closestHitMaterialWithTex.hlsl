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
        if (texcoordTbl[i].x < -10)
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

[shader("closesthit")]
void materialWithTexClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPayload(payload)) {
        return;
    }
    bool isNoTexture = false;
    VertexPNT vtx = GetVertex(attrib, isNoTexture);
    
    float4 diffuseTexColor = 1.xxxx;

    float2 diffTexSize = 0.xx;
    diffuseTex.GetDimensions(diffTexSize.x, diffTexSize.y);
    bool isTexInvalid = (diffTexSize.x == 1 && diffTexSize.y == 1);
    if (!isNoTexture && !isTexInvalid)
    {
        diffuseTexColor = diffuseTex.SampleLevel(gSampler, vtx.UV, 0.0);
    }
    
    const bool isIgnoreHit = (diffuseTexColor.a == 0);
    
    if (!isIgnoreHit)
    {
        depthPositionNormalStore(payload, vtx.Normal);
    }

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());
    
    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;

    if (!isIgnoreHit)
    {
        nextRay.Direction = 0.xxx;
        float3 curEnergy = payload.energy;
        float3 shading = SurafceShading(currentMaterial, vtx.Normal, nextRay, curEnergy);
        const float3 photonIrradiance = photonGather(bestFitWorldPosition, payload.eyeDir, bestHitWorldNormal);
        payload.color += shading * curEnergy * photonIrradiance;
        payload.energy = curEnergy;
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
    float2 diffTexSize = 0.xx;
    diffuseTex.GetDimensions(diffTexSize.x, diffTexSize.y);
    bool isTexInvalid = (diffTexSize.x == 1 && diffTexSize.y == 1);
    if (!isNoTexture && !isTexInvalid)
    {
        diffuseTexColor = diffuseTex.SampleLevel(gSampler, vtx.UV, 0.0);
    }

    const bool isIgnoreHit = (diffuseTexColor.a == 0);

    uint instanceID = InstanceID();

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
   
    nextRay.TMin = 0.001;
    nextRay.TMax = 10000;
    nextRay.Direction = WorldRayDirection();
    if (!isIgnoreHit)
    {
        nextRay.Direction = 0.xxx;
        float3 curEnergy = payload.throughput;
        float3 shading = SurafceShading(currentMaterial, bestHitWorldNormal, nextRay, curEnergy, payload.lambdaNM);
        payload.throughput = shading * curEnergy;
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