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

float3 getGeometricNormal(TriangleIntersectionAttributes attrib)
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
    return normalize(cross(positionTbl[1] - positionTbl[0], positionTbl[2] - positionTbl[0]));
}

[shader("anyhit")]
void anyHit(inout Payload payload, TriangleIntersectionAttributes attrib) {
    if(isSSSRay(payload))
    {
        payload.hittedCount++;
        payload.SSSnormal = getVertex(attrib).Normal;
        //payload.SSSnormal = getGeometricNormal(attrib);
        payload.T = RayTCurrent();
        IgnoreHit();
    }
}

[shader("closesthit")]
void materialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isShadowRay(payload))
    {
        setVisibility(payload, false);
        return;
    }

    if(isSSSRay(payload))
    {
        payload.SSSnormal = getVertex(attrib).Normal;
        payload.T = RayTCurrent();
        return;
    }

    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }
    uint primitiveIndex = PrimitiveIndex();
    uint instanceIndex = InstanceIndex();

    VertexPN vtx = getVertex(attrib);
    
    MaterialParams currentMaterial = constantBuffer;

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
void materialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    VertexPN vtx = getVertex(attrib);
    float3 surfaceNormal = vtx.Normal;

    MaterialParams currentMaterial = constantBuffer;
    float3 scatterPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());

    primarySurfaceHasHighPossibilityCausticsGenerate(currentMaterial, payload);

    RayDesc nextRay;
    nextRay.Origin = scatterPosition;
    nextRay.Direction = 0.xxx;
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