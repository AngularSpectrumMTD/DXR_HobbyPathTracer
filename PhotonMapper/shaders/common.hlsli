#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define LIGHT_BASE_LENGTH 1.0f

#define DEFAULT_RAY_ID 0
#define DEFAULT_MISS_ID 0
#define DEFAULT_GEOM_CONT_MUL 1

struct SceneCB
{
    matrix mtxView;
    matrix mtxProj;
    matrix mtxViewInv;
    matrix mtxProjInv;
    uint4 flags;
    float4 photonParams;
    float4 cameraParams;
    float4 gatherParams;
    float4 gatherParams2;
    float4 spotLightParams;
    float4 viewVec;
    uint4 additional;//x light num
};

#define PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED 1 << 0
#define PAYLOAD_BIT_MASK_IS_SHADOW_RAY 1 << 1
#define PAYLOAD_BIT_MASK_IS_SHADOW_MISS 1 << 2
#define PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE 1 << 3

struct Payload
{
    float3 throughput;
    float3 color;
    uint2 storeIndexXY;
    float3 eyeDir;
    int recursive;
    uint flags;
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

// Global Root Signature
RaytracingAccelerationStructure gRtScene : register(t0);
Texture2D<float4> gEquiRecEnvMap : register(t1);
Texture2D<float2> gLuminanceMomentBufferSrc : register(t2);
StructuredBuffer<LightGenerateParam> gLightGenerateParams : register(t3);
ConstantBuffer<SceneCB> gSceneParam : register(b1);
SamplerState gSampler : register(s0);

RWStructuredBuffer<PhotonInfo> gPhotonMap : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);
RWTexture2D<float> gPrevDepthBuffer : register(u2);
RWStructuredBuffer<uint2> gPhotonGridIdBuffer : register(u3);
RWTexture2D<float4> gDiffuseAlbedoBuffer : register(u4);
RWTexture2D<float4> gPositionBuffer : register(u5);
RWTexture2D<float4> gNormalBuffer : register(u6);
RWTexture2D<float4> gOutput : register(u7);
RWTexture2D<float4> gAccumulationBuffer : register(u8);
RWTexture2D<float2> gLuminanceMomentBufferDst : register(u9);
RWTexture2D<uint> gAccumulationCountBuffer : register(u10);

////////////////////////////////////
//Interpret Scene Param
////////////////////////////////////
bool isDirectionalLight()
{
    return gSceneParam.flags.x == 0;
}

bool isUseTexture()
{
    return gSceneParam.flags.y == 1;
}

bool isVisualizePhotonDebugDraw()
{
    return gSceneParam.flags.z == 1;
}

bool isVisualizeLightRange()
{
    return gSceneParam.flags.w == 1;
}

uint getLightNum()
{
    return gSceneParam.additional.x;
}

bool isIndirectOnly()
{
    return gSceneParam.additional.y == 1;
}

bool isUseNEE()
{
    return gSceneParam.additional.z == 1;
}

bool isUseWRS_RIS()
{
    return gSceneParam.additional.w == 1;
}

bool isApplyCaustics()
{
    return gSceneParam.photonParams.x == 1;
}

float getSpectrumMode()
{
    return gSceneParam.photonParams.z;
}

float getMaxPhotonBounceNum()
{
    return gSceneParam.photonParams.w;
}

float getNearPlaneDistance()
{
    return gSceneParam.cameraParams.x;
}

float getFarPlaneDistance()
{
    return gSceneParam.cameraParams.y;
}

float getMaxBounceNum()
{
    return gSceneParam.cameraParams.z;
}

float getGatherRadius()
{
    return gSceneParam.gatherParams.x;
}

float getGatherSharpness()
{
    return gSceneParam.gatherParams.y;
}

float getGatherBoost()
{
    return gSceneParam.gatherParams.z;
}

float getGatherBlockRange()
{
    return gSceneParam.gatherParams.w;
}

uint getPhotonUnitNum()
{
    return gSceneParam.gatherParams2.x;
}

bool isAccumulationApply()
{
    return (gSceneParam.gatherParams2.y == 1);
}

float getLightRange()
{
    return gSceneParam.spotLightParams.x;
}

float getLightRandomSeed()
{
    return gSceneParam.spotLightParams.y;
}

float getLightLambdaNum()
{
    return gSceneParam.spotLightParams.z;
}

float getCausticsBoost()
{
    return gSceneParam.spotLightParams.w;
}

float3 getViewVec()
{
    return gSceneParam.viewVec.xyz;
}

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
        payload.color = float3(0, 0, 0);
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

void storeAlbedoDepthPositionNormal(inout Payload payload, in float3 albedo, in float3 normal)
{
    if (!(payload.flags & PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED) && (payload.recursive == 1))
    {
        float3 wPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        float2 writeIndex = payload.storeIndexXY;
        gDiffuseAlbedoBuffer[writeIndex] = float4(albedo.x, albedo.y, albedo.z, 0);
        gDepthBuffer[writeIndex] = (payload.recursive == 0) ? 0 : compute01Depth(wPos);
        gPositionBuffer[writeIndex] = float4(wPos.x, wPos.y, wPos.z, 0);
        gNormalBuffer[writeIndex] = float4(normal.x, normal.y, normal.z, 0);
        payload.flags |= PAYLOAD_BIT_MASK_IS_DENOISE_HINT_STORED;
    }
}

float depthLoad(uint2 index)
{
    return gDepthBuffer[index];
}

float lengthSqr(float3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

#endif//__COMMON_HLSLI__