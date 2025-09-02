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

Surface constructSurface(TriangleIntersectionAttributes attrib)
{
    Surface surface = (Surface)0;

    VertexPN vertex = getVertex(attrib);
    surface.position = vertex.Position;
    surface.position = mul(float4(surface.position, 1), ObjectToWorld4x3());
    surface.normal = vertex.Normal;
    surface.interpolatedNormal = vertex.Normal;

    MaterialParams material = constantBuffer;

    if(isAlbedoOne())
    {
        material.albedo = 1.xxxx;
    }

    surface.isIgnoreHit = false;
    surface.material = material;
    surface.geomNormal = getGeometricNormal(attrib);

    return surface;
}

[shader("anyhit")]
void anyHit(inout Payload payload, TriangleIntersectionAttributes attrib) {
    // if(isShadowRay(payload))
    // {
    //     MaterialParams currentMaterial = constantBuffer;
    //     if(isTransparentMaterial(currentMaterial))
    //     {
    //         IgnoreHit();
    //     }
    // }

    Surface surface = constructSurface(attrib);

    if(isSSSRay(payload))
    {
        payload.hitCount++;
        payload.SSSnormal = surface.normal;
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
        payload.terminate();
        return;
    }
    uint primitiveIndex = PrimitiveIndex();
    uint instanceIndex = InstanceIndex();

    Surface surface = constructSurface(attrib);
    if((payload.recursive >= 2) && (surface.material.transRatio > 0))
    {
        surface.material.roughness *= 1.5;
        surface.material.roughness = saturate(surface.material.roughness);
    }

    if(!isUseEmissivePolygon())
    {
        surface.material.emission = 0.xxxx;
    }

    float3 caustics = 0.xxx;
    if(payload.recursive <= 3)
    {
        caustics = accumulatePhoton(surface.position, surface.normal);
    }

    const float3 dir = WorldRayDirection();
    const float T = RayTCurrent();

    RayDesc nextRay;
    bool isTerminate = shadeAndSampleRay(surface, payload, nextRay, caustics);

#ifdef PHOTON_AABB_DEBUG
    //photon AABB debug draw
    {
        AABB photonAABB;
        photonAABB.maxElem = gGridParam.photonExistRange.xxx;
        photonAABB.minElem = -gGridParam.photonExistRange.xxx;
        float t = 0;
        const float3 hitPos = surface.position;
        const bool intersectedPhotonAABB = isIntersectedOriginOrientedAABBvsRay(hitPos - T * dir , dir, photonAABB, t);

        if(intersectedPhotonAABB && isDirectRay(payload) && (t > 0) && (t < T))
        {
            addDI(float3(0, 1, 0));
        }
    }
#endif

    if(isTerminate)
    {
        payload.terminate();
        return;
    }

    const float execP = 0.6;
    if(rand(payload.randomSeed) < (1 - execP))
    {
        payload.terminate();
        return;
    }
    else
    {
        payload.updateThroughputByMulitiplicationF1(1 / execP);
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

    Surface surface = constructSurface(attrib);
    primarySurfaceHasHighPossibilityCausticsGenerate(surface.material, payload);

    RayDesc nextRay;
    nextRay.Origin = surface.position;
    nextRay.Direction = 0.xxx;
    sampleBSDF(surface, nextRay, payload);

    if (isPhotonStoreRequired(surface.material, payload))
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