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

#include "materialParams.hlsli"

struct Payload
{
    uint throughputU32;
    int recursive;
    uint flags;
    float T;//for SSS
    uint hittedCount;//for SSS
    float3 SSSnormal;//for SSS
    uint primaryBSDFU32;//for ReSTIR GI
    float primaryPDF;//for ReSTIR GI
    uint bsdfRandomSeed;//for ReSTIR GI
    uint randomSeed;
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
uint getReservoirSpatialReuseNum()
{
    return gReSTIRParam.data.x;
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
    if (payload.recursive >= getMaxBounceNum() || length(decompressU32asRGB(payload.throughputU32)) < 1e-2)
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

inline bool isSSSRayHitted(in Payload payload)
{
    return payload.hittedCount > 0;
}

inline bool isReachedRecursiveLimitPhotonPayload(inout PhotonPayload payload)
{
    if (payload.recursive >= getMaxPhotonBounceNum())
    {
        payload.throughputU32 = 0u;
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

float computeLuminance(const float3 linearRGB)
{
    return dot(float3(0.2126, 0.7152, 0.0722), linearRGB);
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

#endif//__COMMON_HLSLI__