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

VertexPNT GetVertex(TriangleIntersectionAttributes attrib)
{
    VertexPNT v = (VertexPNT) 0;
    uint start = PrimitiveIndex() * 3; // Triangle List.

    float3 positionTbl[3], normalTbl[3];
    float2 texcoordTbl[3];
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
        texcoordTbl[i] = vertexBuffer[index].UV;
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
    VertexPNT vtx = GetVertex(attrib);
    
    float4 diffuseTexColor = diffuseTex.SampleLevel(gSampler, vtx.UV, 0.0);
    
    const bool isIgnoreHit = diffuseTexColor.a < 0.5;
    
    if (!isIgnoreHit)
    {
        depthPositionNormalStore(payload, vtx.Normal);
    }

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;
    float3 curEnergy = payload.energy;
    float3 shading = SurafceShading(currentMaterial, vtx.Normal, nextRay, curEnergy);
        
    const float3 photonIrradiance = photonGather(bestFitWorldPosition, payload.eyeDir, bestHitWorldNormal);

    if (!isIgnoreHit)
    {
        payload.color += (diffuseTexColor.rgb + shading) * curEnergy * photonIrradiance;
        payload.energy = curEnergy;
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

    VertexPNT vtx = GetVertex(attrib);

    float4 diffuseTexColor = diffuseTex.SampleLevel(gSampler, vtx.UV, 0.0);

    const bool isIgnoreHit = diffuseTexColor.a < 0.5;

    uint instanceID = InstanceID();

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;
    float3 curEnergy = payload.throughput;
    float3 shading = SurafceShading(currentMaterial, bestHitWorldNormal, nextRay, curEnergy, payload.lambdaNM);

    if (!isIgnoreHit)
    {
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