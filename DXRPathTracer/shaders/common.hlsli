#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define LIGHT_BASE_LENGTH 1.0f

#define DEFAULT_RAY_ID 0
#define DEFAULT_MISS_ID 0
#define DEFAULT_GEOM_CONT_MUL 1

#include "sceneCBDefinition.hlsli"
#include "compressionUtility.hlsli"

#define PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED 1 << 0
#define PAYLOAD_BIT_MASK_IS_SHADOW_RAY 1 << 1
#define PAYLOAD_BIT_MASK_IS_SHADOW_MISS 1 << 2
#define PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE 1 << 3
#define PAYLOAD_BIT_MASK_IS_SSS_RAY 1 << 4

//MIP 0 == 64 x 64
#define PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL 7

#define RAY_MIN_T 0.001f
#define RAY_MAX_T 1000000.0f

#define USE_SPECTRAL_RENDERED_CAUSTICS
//#define PHOTON_AABB_DEBUG

#define ETA_AIR 1.0f
#define MEAN_FREE_PATH 1e-5f

#define Z_AXIS float3(0, 0, 1)
#define BSDF_EPS 0.001f
#define RAY_T_BIAS 0.001

#include "materialParams.hlsli"

float computeLuminance(const float3 linearRGB)
{
    return dot(float3(0.2126, 0.7152, 0.0722), linearRGB);
}

struct OpticalGlass
{
    float A0;
    float A1;
    float A2;
    float A3;
    float A4;
    float A5;
    float A6;
    float A7;
    float A8;

    //ex computeRefIndex(lambdaNM * 1e-3)
    float computeRefIndex(float lambdaInMicroMeter)
    {
        float lambdaPow2 = lambdaInMicroMeter * lambdaInMicroMeter;
        float lambdaPow4 = lambdaPow2 * lambdaPow2;
        float invLambdaPow2 = 1 / lambdaPow2;
        float invLambdaPow4 = invLambdaPow2 * invLambdaPow2;
        float invLambdaPow6 = invLambdaPow4 * invLambdaPow2;
        float invLambdaPow8 = invLambdaPow6 * invLambdaPow2;
        float invLambdaPow10 = invLambdaPow8 * invLambdaPow2;
        float invLambdaPow12 = invLambdaPow10 * invLambdaPow2;
        
        return sqrt(A0
		+ A1 * lambdaPow2
		+ A2 * lambdaPow4
		+ A3 * invLambdaPow2
		+ A4 * invLambdaPow4
		+ A5 * invLambdaPow6
		+ A6 * invLambdaPow8
		+ A7 * invLambdaPow10
		+ A8 * invLambdaPow12);
    }
};

static OpticalGlass J_Bak4 =
{
    2.42114503E+00,
    -8.99959341E-03,
    -9.30006854E-05,
    1.43071120E-02,
    1.89993274E-04,
    6.09602388E-06,
    2.25737069E-07,
    0.00000000E+00,
    0.00000000E+00
};

struct Payload
{
    uint throughputU32;
    int recursive;
    uint flags;
    float T;//for SSS
    uint hitCount;//for SSS
    float3 SSSnormal;//for SSS
    uint f0;//for ReSTIR GI
    float p0;//for ReSTIR GI
    uint bsdfRandomSeed;//for ReSTIR GI
    uint randomSeed;

    void terminate()
    {
        throughputU32 = 0u;
    }

    void updateThroughputByMulitiplicationF3(in float3 color)
    {
        throughputU32 = compressRGBasU32(decompressU32asRGB(throughputU32) * color);
    }

    void updateThroughputByMulitiplicationF1(in float v)
    {
        throughputU32 = compressRGBasU32(decompressU32asRGB(throughputU32) * v);
    }
};

#define PHOTON_PAYLOAD_BIT_MASK_IS_PHOTON_STORED 1 << 0
#define PHOTON_PAYLOAD_BIT_MASK_IS_PRIMARY_SURFACE_HAS_HIGH_POSSIBILITY_GENERATE_CAUSTICS 1 << 1

struct PhotonPayload
{
    uint throughputU32;
    int recursive;
    float lambdaNM;
    float2 randomUV;
    uint flags;
    uint randomSeed;

    void terminate()
    {
        throughputU32 = 0u;
    }

    void updateThroughputByMulitiplicationF3(in float3 color)
    {
        throughputU32 = compressRGBasU32(decompressU32asRGB(throughputU32) * color);
    }
};

struct TriangleIntersectionAttributes
{
    float2 barys;
};

struct LightGenerateParam
{
    float3 positionORDirection;
    float3 emission;
    float3 U; //u vector for rectangle or spot light
    float3 V; //v vector for rectangle or spot light
    float sphereRadius; //radius for sphere light
    uint type; //Sphere Light 0 / Rect Light 1 / Spot Light 2 / Directional Light 3
};

struct AABB
{
    float3 maxElem;
    float3 minElem;
};

bool isIntersectedOriginOrientedAABBvsRay(in float3 rayOrigin, in float3 rayDir, in AABB aabb, out float t)
{
    const float FLT_MAX = 100000;
    const float eps = 0.0001;
    t = -FLT_MAX;
    float t_max = FLT_MAX;
    float finalT = FLT_MAX;

    for (int i = 0; i < 3; ++i) 
    {
        if (abs(rayDir[i]) < eps) 
        {
            if (rayOrigin[i] < aabb.minElem[i] || rayOrigin[i] > aabb.maxElem[i] )
            {
                return false;
            }
        } 
        else 
        {
            float t1 = (aabb.minElem[i] - rayOrigin[i]) / rayDir[i];
            float t2 = (aabb.maxElem[i] - rayOrigin[i]) / rayDir[i];
            if (t1 > t2) 
            {
                float tmp = t1; t1 = t2; t2 = tmp;
            }

            if (t1 > t)
            { 
                t = t1;
            }

            if (t2 < t_max) 
            {
                t_max = t2;
            }

            if(t > 0)
            {
                finalT = min(t, finalT);
            }

            if (t >= t_max)
            {
                return false;
            }
        }
    }

    t = finalT;
    return true;
}

#define LIGHT_TYPE_SPHERE 0
#define LIGHT_TYPE_RECT 1
#define LIGHT_TYPE_SPOT 2
#define LIGHT_TYPE_DIRECTIONAL 3

#define PI 3.1415926535
#define LIGHT_INSTANCE_MASK 0x08

#include "Grid3D_Header.hlsli"
#include "reservoir.hlsli"

// Global Root Signature
RaytracingAccelerationStructure gBVH : register(t0);
Texture2D<float4> gEquiRecEnvMap : register(t1);
StructuredBuffer<LightGenerateParam> gLightGenerateParams : register(t2);
ConstantBuffer<SceneCB> gSceneParam : register(b1);
SamplerState gSampler : register(s0);

RWStructuredBuffer<PhotonInfo> gPhotonMap : register(u0);
RWTexture2D<float4> gNormalDepthBuffer : register(u1);
RWStructuredBuffer<uint2> gPhotonGridIdBuffer : register(u2);
RWTexture2D<float4> gPositionBuffer : register(u3);
RWTexture2D<float2> gPrevIDBuffer : register(u4);

RWTexture2D<float4> gDIBuffer : register(u5);
RWTexture2D<float4> gGIBuffer : register(u6);

RWTexture2D<float4> gCausticsBuffer : register(u7);

RWStructuredBuffer<DIReservoir> gDIReservoirBuffer : register(u8);
RWStructuredBuffer<DIReservoir> gDIReservoirBufferSrc : register(u9);

RWTexture2D<uint> gPhotonRandomCounterMap : register(u10);
RWTexture2D<float> gPhotonEmissionGuideMap0 : register(u11);
RWTexture2D<float> gPhotonEmissionGuideMap1 : register(u12);
RWTexture2D<float> gPhotonEmissionGuideMap2 : register(u13);
RWTexture2D<float> gPhotonEmissionGuideMap3 : register(u14);
RWTexture2D<float> gPhotonEmissionGuideMap4 : register(u15);
RWTexture2D<float> gPhotonEmissionGuideMap5 : register(u16);
RWTexture2D<float> gPhotonEmissionGuideMap6 : register(u17);

RWStructuredBuffer<CompressedMaterialParams> gScreenSpaceMaterial : register(u18);
RWTexture2D<float4> gDebugTexture : register(u19);

RWStructuredBuffer<GIReservoir> gGIReservoirBuffer : register(u20);
RWStructuredBuffer<GIReservoir> gGIReservoirBufferSrc : register(u21);

RWTexture2D<float4> gPrevNormalDepthBuffer : register(u22);
RWTexture2D<float4> gPrevPositionBuffer : register(u23);
RWStructuredBuffer<CompressedMaterialParams> gPrevScreenSpaceMaterial : register(u24);

RWTexture2D<uint> gRandomNumber : register(u25);

struct ReSTIRParam
{
    uint4 data;
};

ConstantBuffer<ReSTIRParam> gReSTIRParam : register(b2);

#include "sceneParamInterface.hlsli"

////////////////////////////////////
//Irradiance
////////////////////////////////////
void addDI(in float3 color)
{
    gDIBuffer[DispatchRaysIndex().xy] += float4(color, 0);
}

void setDI(in float3 color)
{
    gDIBuffer[DispatchRaysIndex().xy] = float4(color, 0);
}

void addGI(in float3 color)
{
    gGIBuffer[DispatchRaysIndex().xy] += float4(color, 0);
}

void setGI(in float3 color)
{
    gGIBuffer[DispatchRaysIndex().xy] = float4(color, 0);
}

float3 getGI()
{
    return gGIBuffer[DispatchRaysIndex().xy].xyz;
}

void addCaustics(in float3 color)
{
    gCausticsBuffer[DispatchRaysIndex().xy] += float4(color, 0);
}

void setCaustics(in float3 color)
{
    gCausticsBuffer[DispatchRaysIndex().xy] = float4(color, 0);
}

////////////////////////////////////
//Random
////////////////////////////////////

#include "randomUtility.hlsli"

////////////////////////////////////
// Common Function
////////////////////////////////////
uint getDIReservoirSpatialReuseNum()
{
    return gReSTIRParam.data.x;
}

uint getGIReservoirSpatialReuseNum()
{
    return gReSTIRParam.data.y;
}

uint getDIReservoirSpatialReuseBaseRadius()
{
    return gReSTIRParam.data.z;
}

uint getGIReservoirSpatialReuseBaseRadius()
{
    return gReSTIRParam.data.w;
}

inline int serialRaysIndex(int3 dispatchRaysIndex, int3 dispatchRaysDimensions)
{
    return (dispatchRaysIndex.x + dispatchRaysDimensions.x * dispatchRaysIndex.y) * dispatchRaysDimensions.z + dispatchRaysIndex.z;
}

GIReservoir getGIReservoir()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    return gGIReservoirBuffer[serialIndex];
}

CompressedMaterialParams getScreenSpaceMaterial()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    return gScreenSpaceMaterial[serialIndex];
}

CompressedMaterialParams getScreenSpaceMaterial(in int2 id)
{
    uint3 dispatchDimensions = DispatchRaysDimensions();
    int serialIndex = serialRaysIndex(uint3(id, 0), dispatchDimensions);

    return gScreenSpaceMaterial[serialIndex];
}

void setDIReservoir(in DIReservoir reservoir)
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    gDIReservoirBuffer[serialIndex] = reservoir;
}

void setGIReservoir(in GIReservoir giReservoir)
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    gGIReservoirBuffer[serialIndex] = giReservoir;
}

inline float2 computeInterpolatedAttributeF2(float2 vertexAttributeTbl[3], float2 barycentrics)
{
    float2 ret;
    ret = vertexAttributeTbl[0];
    ret += barycentrics.x * (vertexAttributeTbl[1] - vertexAttributeTbl[0]);
    ret += barycentrics.y * (vertexAttributeTbl[2] - vertexAttributeTbl[0]);
    return ret;
}

float3 computeInterpolatedAttributeF3(float3 vertexAttributeTbl[3], float2 barycentrics)
{
    float3 ret;
    ret = vertexAttributeTbl[0];
    ret += barycentrics.x * (vertexAttributeTbl[1] - vertexAttributeTbl[0]);
    ret += barycentrics.y * (vertexAttributeTbl[2] - vertexAttributeTbl[0]);
    return ret;
}

inline bool isReachedRecursiveLimitPayload(inout Payload payload)
{
    payload.recursive++;
    if (payload.recursive >= getMaxBounceNum())
    {
        return true;
    }
    return false;
}

inline bool isShadowRay(inout Payload payload)
{
    return payload.flags & PAYLOAD_BIT_MASK_IS_SHADOW_RAY;
}

inline bool isSSSRay(inout Payload payload)
{
    return payload.flags & PAYLOAD_BIT_MASK_IS_SSS_RAY;
}

inline void setVisibility(inout Payload payload, in bool visibility)
{
    if (visibility)
    {
        payload.flags |= PAYLOAD_BIT_MASK_IS_SHADOW_MISS;
    }
    else
    {
        payload.flags &= ~PAYLOAD_BIT_MASK_IS_SHADOW_MISS;
    }
}

inline bool isSSSRayHit(in Payload payload)
{
    return payload.hitCount > 0;
}

inline bool isReachedRecursiveLimitPhotonPayload(inout PhotonPayload payload)
{
    if (payload.recursive >= getMaxPhotonBounceNum())
    {
        payload.terminate();
        return true;
    }
    payload.recursive++;
    return false;
}

////////////////////////////////////
//GBuffer
////////////////////////////////////
float compute01Depth(float3 wPos)
{
    matrix mtxViewProj = mul(gSceneParam.mtxProj, gSceneParam.mtxView);
    float4 svPosition = mul(mtxViewProj, float4(wPos, 1));
    float depth = (svPosition.z - getNearPlaneDistance())
     / (getFarPlaneDistance() - getNearPlaneDistance());
    float zeroOneDepth = 0.5 * (depth + 1); //near 1 to far 0
    return zeroOneDepth;
}

void storeGBuffer(inout Payload payload, in float3 position, in float3 albedo, in float3 normal, in float roughness, in MaterialParams material)
{
    if (!(payload.flags & PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED) && (payload.recursive <= 1))
    {
        float2 writeIndex = DispatchRaysIndex().xy;
        gNormalDepthBuffer[writeIndex] = float4(normal.x, normal.y, normal.z, (payload.recursive == 0) ? 0 : compute01Depth(position));
        gPositionBuffer[writeIndex] = float4(position, 0);
        matrix mtxViewProj = mul(gSceneParam.mtxProj, gSceneParam.mtxView);
        matrix mtxViewProjPrev = mul(gSceneParam.mtxProjPrev, gSceneParam.mtxViewPrev);
        float4 currVpPos = mul(mtxViewProj, float4(position, 1));
        currVpPos.xy /= currVpPos.w;
        float4 prevVpPos = mul(mtxViewProjPrev, float4(position, 1));
        prevVpPos.xy /= prevVpPos.w;
        const float2 velocity = prevVpPos.xy - currVpPos.xy;
        //gVelocityBuffer[writeIndex] = velocity;
        
        float2 dims = float2(DispatchRaysDimensions().xy);
        gPrevIDBuffer[writeIndex] = (prevVpPos.xy * float2(0.5, -0.5) + 0.5.xx) * dims;

        uint3 launchIndex = DispatchRaysIndex();
        uint3 dispatchDimensions = DispatchRaysDimensions();
        int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
        gScreenSpaceMaterial[serialIndex] = compressMaterialParams(material);

        payload.flags |= PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED;
    }
}

float depthLoad(uint2 index)
{
    return gNormalDepthBuffer[index].w;
}

float lengthSqr(float3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

bool isCompletelyMissRay(in Payload payload)
{
    return (payload.recursive == 0);
}

bool isDirectRay(in Payload payload)
{
    return (payload.recursive == 1);
}

bool isIndirectRay(in Payload payload)
{
    return (payload.recursive > 1);
}

void setNEEFlag(inout Payload payload, in bool isNEE_Exec)
{
    if (isNEE_Exec)
    {
        if (!(payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE))
        {
            payload.flags |= PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
        }
    }
    else
    {
        if (payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE)
        {
            payload.flags &= ~PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
        }
    }
}

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

//utility
void TraceDefaultRay(in bool flags, in uint rayMask, inout RayDesc ray, inout Payload payload)
{
    TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, ray, payload);
}

void TraceDefaultPhoton(in bool flags, in uint rayMask, inout RayDesc ray, inout PhotonPayload payload)
{
    TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, ray, payload);
}

void initializeRNG(uint2 index, out uint seed)
{
    uint3 dispatchDimensions = DispatchRaysDimensions();
    seed = gRandomNumber[index] + generateRandomSeed(index, dispatchDimensions.x);
}

void finalizeRNG(uint2 index, in uint seed)
{
    gRandomNumber[index] = seed;
}

float copysignf(float x, float y)
{
    uint ux = asuint(x);
    uint uy = asuint(y);
	ux &= 0x7fffffff;
	ux |= uy & 0x80000000;
	return asfloat(ux);
}

//Paper : "Building an Orthonormal Basis, Revisited"
void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
{
  float sign = copysignf(1.0f, normal.z);
  const float A = -1.0f / (sign + normal.z);
  const float B = normal.x * normal.y * A;
  tangent = float3(1.0f + sign * normal.x * normal.x * A, sign * B, -sign * normal.x);
  bitangent = float3(B, sign + normal.y * normal.y * A, -normal.y);
}

uint getRandomLightID(inout uint randomSeed)
{
    return min(max(0, (uint) (rand(randomSeed) * (getLightNum()) + 0.5)), getLightNum() - 1);
}

float3 tangentToWorld(float3 N, float3 tangentSpaceVec)
{
    float3 tangent;
    float3 bitangent;
    ONB(N, tangent, bitangent);

    return normalize(tangent * tangentSpaceVec.x + bitangent * tangentSpaceVec.y + N * tangentSpaceVec.z);
}

float3 worldToTangent(float3 N, float3 worldSpaceVec)
{
    float3 tangent;
    float3 bitangent;
    ONB(N, tangent, bitangent);

    return normalize(float3(dot(tangent, worldSpaceVec), dot(bitangent, worldSpaceVec), dot(N, worldSpaceVec)));
}

float3 HemisphereORCosineSampling(float3 N, bool isHemi, inout uint randomSeed, out float2 randomUV)
{
    float u = rand(randomSeed);
    float v = rand(randomSeed);
    randomUV = float2(u, v);
    float cosT = isHemi ? u : sqrt(u);
    float sinT = sqrt(1 - cosT * cosT);
    float P = 2 * PI * v;
    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);

    return tangentToWorld(N, tangentDir);
}

float3 HemisphereORCosineSamplingWithUV(float3 N, bool isHemi, in float2 randomUV)
{
    float u = randomUV.x;
    float v = randomUV.y;
    float cosT = isHemi ? u : sqrt(u);
    float sinT = sqrt(1 - cosT * cosT);
    float P = 2 * PI * v;
    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);

    return tangentToWorld(N, tangentDir);
}

bool isNearMaterial(in MaterialParams mat0, in MaterialParams mat1)
{
    return 
    (sqrt(length(mat0.albedo.rgb - mat1.albedo.rgb)) < 0.1) &&
    (abs(mat0.metallic - mat1.metallic) < 0.1) &&
    (abs(mat0.roughness - mat1.roughness) < 0.1) &&
    (abs(mat0.specular - mat1.specular) < 0.1) &&
    (abs(mat0.transRatio - mat1.transRatio) < 0.1) &&
    (sqrt(length(mat0.transColor.rgb - mat1.transColor.rgb)) < 0.1) &&
    (sqrt(length(mat0.emission.rgb - mat1.emission.rgb)) < 0.1);
}

#endif//__COMMON_HLSLI__