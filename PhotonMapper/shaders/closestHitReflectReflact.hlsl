#include "photonGathering.hlsli"
#include "opticalFunction.hlsli"

struct VertexPN
{
    float3 Position;
    float3 Normal;
};

ConstantBuffer<MaterialParams> constantBuffer : register(b0, space1);
StructuredBuffer<uint> indexBuffer : register(t0, space1);
StructuredBuffer<VertexPN> vertexBuffer : register(t1, space1);

VertexPN GetVertex(TriangleIntersectionAttributes attrib)
{
    VertexPN v = (VertexPN) 0;
    uint start = PrimitiveIndex() * 3; // Triangle List.

    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }

    v.Position = ComputeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = ComputeInterpolatedAttributeF3(normalTbl, attrib.barys);
    v.Normal = normalize(v.Normal);
    return v;
}

[shader("closesthit")]
void reflectReflactMaterialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }

    VertexPN vtx = GetVertex(attrib);

    depthPositionNormalStore(payload, vtx.Normal);
    
    MaterialParams currentMaterial = constantBuffer;

    RayDesc nextRay;
    nextRay.Origin = 0.xxx;
    nextRay.Direction = 0.xxx;
    float3 curEnergy = payload.energy;
    float3 shading = SurafceShading(currentMaterial, vtx.Normal, nextRay, curEnergy);
    float3 worldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());
    const float3 photonIrradiance = photonGather(WorldRayOrigin() + WorldRayDirection() * RayTCurrent(), payload.eyeDir, worldNormal);
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
void reflectReflactMaterialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload))
    {
        return;
    }
    VertexPN vtx = GetVertex(attrib);

    uint instanceID = InstanceID();

    MaterialParams currentMaterial = constantBuffer;

    RayDesc nextRay;
    nextRay.Origin = 0.xxx;
    nextRay.Direction = 0.xxx;
    float3 curEnergy = payload.throughput;
    float3 shading = SurafceShading(currentMaterial, vtx.Normal, nextRay, curEnergy, payload.lambdaNM);
    payload.throughput = shading * curEnergy;
    if (currentMaterial.roughness > 0.8 && currentMaterial.specularTrans < 0.8)
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
void reflectReflactMaterialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    //no op
}
