#include "photonGathering.hlsli"

struct VertexPN {
    float3 Position;
    float3 Normal;
};

ConstantBuffer<MaterialParams> constantBuffer : register(b0, space1);
StructuredBuffer<uint>   indexBuffer : register(t0,space1);
StructuredBuffer<VertexPN> vertexBuffer : register(t1, space1);

VertexPN getVertex(TriangleIntersectionAttributes attrib)
{
    VertexPN v = (VertexPN)0;
    uint start = PrimitiveIndex() * 3;
    
    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }

    v.Position = computeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = normalize(computeInterpolatedAttributeF3(normalTbl, attrib.barys));
    return v;
}

[shader("closesthit")]
void materialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isShadowRay(payload))
    {
        setVisibility(payload, false);
        return;
    }

    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }
    VertexPN vtx = getVertex(attrib);
    float3 surfaceNormal = vtx.Normal;

    MaterialParams currentMaterial = constantBuffer;
    storeGBuffer(payload, currentMaterial.albedo.xyz, surfaceNormal);

    float3 scatterPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(surfaceNormal, (float3x3)ObjectToWorld4x3());

    if (executeLighting(payload, currentMaterial, scatterPosition, surfaceNormal))
    {
        return;
    }

    const float3 photon = accumulatePhoton(scatterPosition, payload.eyeDir, bestFitWorldNormal);

    RayDesc nextRay;
    nextRay.Origin = scatterPosition;
    nextRay.Direction = 0.xxx;
    payload.color += payload.throughput * photon;
    updateRay(currentMaterial, surfaceNormal, nextRay, payload.throughput);

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
}

[shader("closesthit")]
void materialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    VertexPN vtx = getVertex(attrib);
    float3 surfaceNormal = vtx.Normal;

    MaterialParams currentMaterial = constantBuffer;
    float3 scatterPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = scatterPosition;
    nextRay.Direction = 0.xxx;
    updateRay(currentMaterial, surfaceNormal, nextRay, payload.throughput, payload.lambdaNM);

    if (isPhotonStoreRequired(currentMaterial))
    {
        storePhoton(payload);
    }
    else
    {
        RAY_FLAG flags = RAY_FLAG_NONE;
        uint rayMask = 0xff;
        TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
    }
}

[shader("closesthit")]
void materialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    // no op
}