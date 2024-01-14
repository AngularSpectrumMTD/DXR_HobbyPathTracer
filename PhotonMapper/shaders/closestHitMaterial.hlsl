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

    storeDepthPositionNormal(payload, vtx.Normal);

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;
    
    LightSample lightSample;
    sampleLight(bestFitWorldPosition, lightSample);
    //const float3 lightIrr = Visibility(bestFitWorldPosition, lightSample) * lightSample.emission / lightSample.pdf;
    const float3 lightIrr = RIS_WRS_LightIrradiance(bestFitWorldPosition, lightSample);
    payload.color += payload.energy * (currentMaterial.emission.xyz + lightIrr * currentMaterial.roughness + accumulatePhoton(bestFitWorldPosition, payload.eyeDir, bestFitWorldNormal));
    shadeSurface(currentMaterial, vtx.Normal, nextRay, payload.energy);

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

    VertexPN vtx = getVertex(attrib);

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;

    if (isPhotonStoreRequired(currentMaterial))
    {
        storePhoton(payload);
    }
    else
    {
        shadeSurface(currentMaterial, vtx.Normal, nextRay, payload.throughput, payload.lambdaNM);
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