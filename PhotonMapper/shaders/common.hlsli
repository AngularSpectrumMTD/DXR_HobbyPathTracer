#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define LIGHT_BASE_LENGTH 1.0f

#define DEFAULT_RAY_ID 0
#define DEFAULT_MISS_ID 0
#define DEFAULT_GEOM_CONT_MUL 1

#include "sceneCBDefinition.hlsli"

#define PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED 1 << 0
#define PAYLOAD_BIT_MASK_IS_SHADOW_RAY 1 << 1
#define PAYLOAD_BIT_MASK_IS_SHADOW_MISS 1 << 2
#define PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE 1 << 3

#define RAY_MIN_T 0.001f
#define RAY_MAX_T 1000000.0f

struct Payload
{
    float3 throughput;
    float3 caustics;
    uint2 storeIndexXY;
    float3 eyeDir;
    int recursive;
    uint flags;
    float3 DI;
    float3 GI;
};

struct PhotonPayload
{
    float3 throughput;
    int recursive;
    int storeIndex;
    int stored;
    float lambdaNM;
};

struct TriangleIntersectionAttributes
{
    float2 barys;
};

struct LightGenerateParam
{
    float3 position;
    float3 emission;
    float3 U; //u vector for rectangle or spot light
    float3 V; //v vector for rectangle or spot light
    float sphereRadius; //radius for sphere lightf
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
RWTexture2D<float> gDepthBuffer : register(u1);
RWStructuredBuffer<uint2> gPhotonGridIdBuffer : register(u2);
RWTexture2D<float4> gDiffuseAlbedoBuffer : register(u3);
RWTexture2D<float4> gPositionBuffer : register(u4);
RWTexture2D<float4> gNormalBuffer : register(u5);
RWTexture2D<float2> gVelocityBuffer : register(u6);

RWTexture2D<float4> gDIBuffer : register(u7);
RWTexture2D<float4> gGIBuffer : register(u8);

RWTexture2D<float4> gCausticsBuffer : register(u9);

RWStructuredBuffer<DIReservoir> gDIReservoirBuffer : register(u10);

#include "sceneParamInterface.hlsli"

////////////////////////////////////
//Random
////////////////////////////////////
static uint randGenState;
static const float rnd01Converter = (1.0f / 4294967296.0f);
float randXorshift()
{
    randGenState ^= uint(randGenState << 13);
    randGenState ^= uint(randGenState >> 17);
    randGenState ^= uint(randGenState << 5);
    return randGenState * rnd01Converter;
}

uint wangHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uint pcgHash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u; 
    return (word >> 22u) ^ word;
}

float pcgHashState()
{
    randGenState = randGenState * 747796405u + 2891336453u;
    uint word = ((randGenState >> ((randGenState >> 28u) + 4u)) ^ randGenState) * 277803737u; 
    return ((word >> 22u) ^ word) * rnd01Converter;
}

////////////////////////////////////
// Common Function
////////////////////////////////////
inline int serialRaysIndex(int3 dispatchRaysIndex, int3 dispatchRaysDimensions)
{
    return (dispatchRaysIndex.x + dispatchRaysDimensions.x * dispatchRaysIndex.y) * dispatchRaysDimensions.z + dispatchRaysIndex.z;
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
    if (payload.recursive >= getMaxBounceNum() || length(payload.throughput) < 1e-2)
    {
        return true;
    }
    return false;
}

inline bool isShadowRay(inout Payload payload)
{
    return payload.flags & PAYLOAD_BIT_MASK_IS_SHADOW_RAY;
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

inline bool isReachedRecursiveLimitPhotonPayload(inout PhotonPayload payload)
{
    if (payload.recursive >= getMaxPhotonBounceNum())
    {
        payload.throughput = float3(0, 0, 0);
        return true;
    }
    payload.recursive++;
    return false;
}

float3x3 rodriguesRoatationFormula(float theta, float3 n)
{
    float cosT, sinT;
    sincos(theta, sinT, cosT);

    return float3x3(
        cosT + n.x * n.x * (1 - cosT), n.x * n.y * (1 - cosT) - n.z * sinT, n.x * n.z * (1 - cosT) + n.y * sinT,
        n.x * n.y * (1 - cosT) + n.z * sinT, cosT + n.y * n.y * (1 - cosT), n.y * n.z * (1 - cosT) - n.x * sinT,
        n.z * n.x * (1 - cosT) - n.y * sinT, n.y * n.z * (1 - cosT) + n.x * sinT, cosT + n.z * n.z * (1 - cosT)
        );
}

float3 getConeSample(float randSeed, float3 direction, float coneAngle)
{
    float cosAngle = cos(coneAngle);

    float z = pcgHashState() * (1.0f - cosAngle) + cosAngle;
    float phi = pcgHashState() * 2.0f * PI;

    float x = sqrt(1.0f - z * z) * cos(phi);
    float y = sqrt(1.0f - z * z) * sin(phi);
    float3 north = float3(0.f, 0.f, 1.f);

    float3 axis = normalize(cross(north, normalize(direction)));
    float angle = acos(dot(normalize(direction), north));

    float3x3 R = rodriguesRoatationFormula(angle, axis);

    return mul(R, float3(x, y, z));
}

////////////////////////////////////
//GBuffer
////////////////////////////////////
float compute01Depth(float3 wPos)
{
    matrix mtxViewProj = mul(gSceneParam.mtxProj, gSceneParam.mtxView);
    float4 svPosition = mul(float4(wPos, 1), mtxViewProj);
    float depth = (svPosition.z - getNearPlaneDistance())
     / (getFarPlaneDistance() - getNearPlaneDistance());
    float zeroOneDepth = 0.5 * (depth + 1); //near 1 to far 0
    return zeroOneDepth;
}

void storeGBuffer(inout Payload payload, in float3 albedo, in float3 normal)
{
    if (!(payload.flags & PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED) && (payload.recursive == 1))
    {
        float3 wPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        float2 writeIndex = payload.storeIndexXY;
        gDiffuseAlbedoBuffer[writeIndex] = float4(albedo.x, albedo.y, albedo.z, 0);
        gDepthBuffer[writeIndex] = (payload.recursive == 0) ? 0 : compute01Depth(wPos);
        gPositionBuffer[writeIndex] = float4(wPos.x, wPos.y, wPos.z, 0);
        gNormalBuffer[writeIndex] = float4(normal.x, normal.y, normal.z, 0);
        matrix mtxViewProj = mul(gSceneParam.mtxProj, gSceneParam.mtxView);
        matrix mtxViewProjPrev = mul(gSceneParam.mtxProjPrev, gSceneParam.mtxViewPrev);
        float4 currSvPosition = mul(float4(wPos, 1), mtxViewProj);
        float4 prevSvPosition = mul(float4(wPos, 1), mtxViewProjPrev);
        float2 velocity = currSvPosition.xy / currSvPosition.w - prevSvPosition.xy / prevSvPosition.w;//-1 to 1
        //gVelocityBuffer[writeIndex] = 0.5 * velocity + 0.5;
        gVelocityBuffer[writeIndex] = currSvPosition.xy / currSvPosition.w;
        payload.flags |= PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED;
    }
}

void storeDIReservoir(in DIReservoir reservoir, in Payload payload)
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    gDIReservoirBuffer[serialIndex] = reservoir;
}

float depthLoad(uint2 index)
{
    return gDepthBuffer[index];
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

#endif//__COMMON_HLSLI__