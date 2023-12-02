#include "photonGathering.hlsli"

struct VertexPN {
    float3 Position;
    float3 Normal;
};

StructuredBuffer<uint>   indexBuffer : register(t0,space1);
StructuredBuffer<VertexPN> vertexBuffer : register(t1, space1);
ConstantBuffer<MaterialParams> constantBuffer : register(b0, space1);

VertexPN GetVertex(TriangleIntersectionAttributes attrib)
{
    VertexPN v = (VertexPN)0;
    uint start = PrimitiveIndex() * 3;
    
    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }

    v.Position = ComputeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = normalize(ComputeInterpolatedAttributeF3(normalTbl, attrib.barys));
    return v;
}

[shader("closesthit")]
void materialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPayload(payload)) {
        return;
    }
    VertexPN vtx = GetVertex(attrib);

    depthPositionNormalStore(payload, vtx.Normal);

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;
    float3 curEnergy = payload.energy;
    float3 shading = SurafceShading(currentMaterial, vtx.Normal, nextRay, curEnergy);
        
    const float3 photonIrradiance = photonGather(bestFitWorldPosition, payload.eyeDir, bestHitWorldNormal);
    payload.color += shading * curEnergy * photonIrradiance;
    payload.energy = curEnergy;
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
void materialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    VertexPN vtx = GetVertex(attrib);

    uint instanceID = InstanceID();

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;
    float3 curEnergy = payload.throughput;
    float3 shading = SurafceShading(currentMaterial, bestHitWorldNormal, nextRay, curEnergy, payload.lambdaNM);
    payload.throughput = shading * curEnergy;
    if (isPhotonStoreRequired(currentMaterial))
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