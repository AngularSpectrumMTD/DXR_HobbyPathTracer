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
Texture2D<float> bumpMap: register(t4, space1);
Texture2D<float4> normalMap: register(t5, space1);
Texture2D<float> roughnessMap: register(t6, space1);
Texture2D<float> metalnessMap: register(t7, space1);

#define MIN_ROUGHNESS 0.05f
#define MIN_METALLIC 0.05f
#define MAX_ROUGHNESS 0.95f
#define MAX_METALLIC 0.95f

float3 getGeometricNormal(TriangleIntersectionAttributes attrib)
{
    uint start = PrimitiveIndex() * 3;
    
    float3 positionTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
    }
    return normalize(cross(positionTbl[1] - positionTbl[0], positionTbl[2] - positionTbl[0]));
}

float3 getInterpolatedNormal(TriangleIntersectionAttributes attrib)
{
    uint start = PrimitiveIndex() * 3;
    
    float3 normalTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        normalTbl[i] = vertexBuffer[index].Normal;
    }
    return computeInterpolatedAttributeF3(normalTbl, attrib.barys);
}

//the values of normal map are encoded by this fomula : normalMapColor = (normal + 1) * 0.5
float3 decodeNormalMap(float3 normalMapValues)
{
    return normalMapValues * 2.0 - 1;
}

VertexPNT getVertex(TriangleIntersectionAttributes attrib)
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
    v.Position = computeInterpolatedAttributeF3(positionTbl, attrib.barys);

    MaterialParams currentMaterial = constantBuffer;
    
    v.UV = computeInterpolatedAttributeF2(texcoordTbl, attrib.barys);
    v.Normal = computeInterpolatedAttributeF3(normalTbl, attrib.barys);

    if(isUseNormalMapping())
    {
        if(hasBumpMap(currentMaterial))//checked
        {
            float4 bumpValues = bumpMap.GatherRed(gSampler, v.UV, 0.0);
            //GatherOp
            //w z
            //x y
            float nz = bumpValues.x;
            float nx = nz - bumpValues.y;
            float ny = nz - bumpValues.w;
            float3 localNormal = normalize(float3(nx, ny, nz));
            v.Normal = tangentToWorld(v.Normal, localNormal);
        }
        else if(hasNormalMap(currentMaterial))
        {
            float3 localNormal = decodeNormalMap(normalMap.SampleLevel(gSampler, v.UV, 0.0).rgb);
            v.Normal = tangentToWorld(v.Normal, localNormal);
        }
    }


    v.Normal = normalize(v.Normal);
    return v;
}


void getTexColor(out float4 diffuseTexColor, out bool isIgnoreHit, in float2 UV)
{
    diffuseTexColor = 0.xxxx;
    float alpMask = 1;

    MaterialParams currentMaterial = constantBuffer;

    if (hasDiffuseTex(currentMaterial) || hasAlphaMask(currentMaterial))
    {
        if(hasDiffuseTex(currentMaterial))
        {
            diffuseTexColor = diffuseTex.SampleLevel(gSampler, UV, 0.0);
            if (diffuseTexColor.r > 0 && diffuseTexColor.g == 0 && diffuseTexColor.b == 0)//1 channel
            {
                diffuseTexColor.rgb = diffuseTexColor.rrr;
            }
        }

         if(hasAlphaMask(currentMaterial))
         {
            alpMask = alphaMask.SampleLevel(gSampler, UV, 0.0);
         }
    }
    else
    {
        diffuseTexColor = 1.xxxx;
        alpMask = 1;
        isIgnoreHit = false;
        return;
    }
    
    isIgnoreHit = (diffuseTexColor.a < 1) || (hasAlphaMask(currentMaterial) && alpMask < 1);
}

void applyRoughnessMap(inout MaterialParams mat, in float2 UV)
{
    if(hasRoughnessMap(mat))
    {
        mat.roughness = clamp(roughnessMap.SampleLevel(gSampler, UV, 0.0), MIN_ROUGHNESS, MAX_ROUGHNESS);
    }
}

void applyMetallnessMap(inout MaterialParams mat, in float2 UV)
{
    if(hasMetallnessMap(mat))
    {
        mat.metallic = clamp(metalnessMap.SampleLevel(gSampler, UV, 0.0), MIN_METALLIC, MAX_METALLIC);
    }
}

void editMaterial(inout MaterialParams mat)
{
    //I can't decide these params to recognize material as glass
    if (mat.metallic > 0.3 && mat.roughness > 0.5)
    {
        mat.transColor = 1.xxxx;
        mat.transRatio = 0;
        mat.albedo = 1.xxxx;
    }
    else
    {
        mat.roughness = 0.1;
        mat.transColor = 1.xxxx;
        mat.transRatio = 0.9;
        mat.metallic = 0.3;
        mat.albedo = 1.xxxx;

        //mat.emission = float4(1,1,0,0);//test
    }
}

MaterialParams getCurrentMaterial(TriangleIntersectionAttributes attrib, in float2 uv, inout bool isIgnoreHit)
{
    float4 diffuseTexColor = 1.xxxx;

    getTexColor(diffuseTexColor, isIgnoreHit, uv);

    MaterialParams currentMaterial = constantBuffer;

    if(hasDiffuseTex(currentMaterial))
    {
        currentMaterial.albedo = float4(diffuseTexColor.rgb, 1);
    }

    applyRoughnessMap(currentMaterial, uv);
    applyMetallnessMap(currentMaterial, uv);

    //test metallic surface
    if(isUseMetallicTest())
    {
        currentMaterial.roughness = 0.2;
        currentMaterial.metallic = 0.5;
    }

    //recognize as glass
    if (length(currentMaterial.albedo) == 0 && !hasDiffuseTex(currentMaterial))
    {
        editMaterial(currentMaterial);
    }

    if(!isUseEmissivePolygon())
    {
        currentMaterial.emission = 0.xxxx;
    }

    return currentMaterial;
}

Surface constructSurface(TriangleIntersectionAttributes attrib)
{
    Surface surface = (Surface)0;

    VertexPNT vertex = getVertex(attrib);
    surface.position = vertex.Position;
    surface.position = mul(float4(surface.position, 1), ObjectToWorld4x3());
    surface.normal = vertex.Normal;
    surface.interpolatedNormal = getInterpolatedNormal(attrib);

    bool isIgnoreHit = false;
    MaterialParams material = getCurrentMaterial(attrib, vertex.UV, isIgnoreHit);

    if(isAlbedoOne())
    {
        material.albedo = 1.xxxx;
    }

    surface.isIgnoreHit = isIgnoreHit;
    surface.material = material;
    surface.geomNormal = getGeometricNormal(attrib);

    return surface;
}

[shader("anyhit")]
void anyHitWithTex(inout Payload payload, TriangleIntersectionAttributes attrib) {
    Surface surface = constructSurface(attrib);

    if(surface.isIgnoreHit)
    {
        IgnoreHit();
    }

    if(isShadowRay(payload))
    {
        if(isTransparentMaterial(surface.material))
        {
            IgnoreHit();
        }
    }

    if(isSSSRay(payload))
    {
        payload.hitCount++;
        payload.SSSnormal = surface.normal;
        payload.T = RayTCurrent();
        IgnoreHit();
    }
}

[shader("closesthit")]
void materialWithTexClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
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

    Surface surface = constructSurface(attrib);
    if((payload.recursive >= 2) && (surface.material.transRatio > 0))
    {
        surface.material.roughness *= 1.5;
        surface.material.roughness = saturate(surface.material.roughness);
    }

    if(isUseNormalMapping() && (dot(-WorldRayDirection(), surface.normal) < 0))
    {
        surface.normal *= -1;
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

    //debug
    // if(isDirectRay(payload))
    // {
    //     payload.throughputU32 = compressRGBasU32(float3(0, 1, 0));
    // }
    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceDefaultRay(flags, rayMask, nextRay, payload);
}

[shader("closesthit")]
void materialWithTexStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    Surface surface = constructSurface(attrib);
    primarySurfaceHasHighPossibilityCausticsGenerate(surface.material, payload);

    RayDesc nextRay;
    nextRay.Origin = surface.position;
   
    nextRay.TMin = RAY_MIN_T;
    nextRay.TMax = RAY_MAX_T;
    nextRay.Direction = WorldRayDirection();
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