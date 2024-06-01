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

    uint primitiveIndex = PrimitiveIndex();
    uint instanceIndex = InstanceIndex();
    float3 scatterPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(surfaceNormal, (float3x3)ObjectToWorld4x3());

    float hitT = -1;
    float3 hitColor = 0.xxx;
    float3 hitNormal = 0.xxx;

    MaterialParams currentMaterial = constantBuffer;

    const bool isFinish = applyLighting(payload, currentMaterial, scatterPosition, surfaceNormal, hitT, hitColor, hitNormal, false);

    if(isFinish)
    {
        const bool isHitLight = (hitT > 0);
        scatterPosition = isHitLight ? (WorldRayOrigin() + hitT * WorldRayDirection()) : scatterPosition;
        float3 storeAlbedo = isHitLight ? hitColor : currentMaterial.albedo.xyz;
        float3 storeNormal = isHitLight ? hitNormal : surfaceNormal;
        storeGBuffer(payload, scatterPosition, storeAlbedo, storeNormal, primitiveIndex, instanceIndex, currentMaterial.roughness);
        return;
    }
    else
    {
        storeGBuffer(payload, scatterPosition, currentMaterial.albedo.xyz, surfaceNormal, primitiveIndex, instanceIndex, currentMaterial.roughness);
    }

    const float3 photon = accumulatePhoton(scatterPosition, payload.eyeDir, bestFitWorldNormal);

    RayDesc nextRay;
    nextRay.Origin = scatterPosition;
    nextRay.Direction = 0.xxx;
    const float3 element = payload.throughput * photon;
    payload.caustics += element;
    updateRay(currentMaterial, surfaceNormal, nextRay, payload.throughput);

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
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
        TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
    }
}

[shader("closesthit")]
void materialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    // no op
}