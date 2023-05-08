#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

struct SceneCB
{
    matrix mtxView;
    matrix mtxProj;
    matrix mtxViewInv;
    matrix mtxProjInv;
    float4 lightColor;
    float4 backgroundColor;
    float4 spotLightPosition;
    float4 spotLightDirection;
    uint4 flags;
    float4 photonParams;
    float4 cameraParams;
    float4 gatherParams;
    float4 gatherParams2;
    float4 spotLightParams;
};

struct Payload
{
    float3 color;
    float3 photonColor;
    int recursive;
    int2 storeIndexXY;
    float3 eyeDir;
    int stored;
};

struct PhotonPayload
{
    float3 throughput;
    int recursive;
    int storeIndex;
    int stored;
    int reThroughRequired;
    float lambdaNM;
};

struct TriangleIntersectionAttributes
{
    float2 barys;
};

#define PI 3.1415926535
#define LIGHT_INSTANCE_MASK 0x08

#include "Grid3D_Header.hlsli"

// Global Root Signature
RaytracingAccelerationStructure gRtScene : register(t0);
Texture2D<float4> gEquiRecEnvMap : register(t1);
ConstantBuffer<SceneCB> gSceneParam : register(b1);
SamplerState gSampler : register(s0);

RWStructuredBuffer<PhotonInfo> gPhotonMap : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);
RWTexture2D<float> gPrevDepthBuffer : register(u2);
RWStructuredBuffer<uint2> gPhotonGridIdBuffer : register(u3);
RWTexture2D<float4> gPositionBuffer : register(u4);
RWTexture2D<float4> gNormalBuffer : register(u5);
RWTexture2D<float4> gOutput : register(u6);
RWTexture2D<float4> gOutput1 : register(u7);

////////////////////////////////////
//Interpret Scene Param
////////////////////////////////////
bool isDirectionalLight()
{
    return gSceneParam.flags.x == 0;
}

bool isUseTextureForStage()
{
    return gSceneParam.flags.y == 0;
}

bool isVisualizePhotonDebugDraw()
{
    return gSceneParam.flags.z == 1;
}

bool isVisualizeLightRange()
{
    return gSceneParam.flags.w == 1;
}

bool isApplyCaustics()
{
    return gSceneParam.photonParams.x == 1;
}

float getTempAccumuRatio()
{
    return gSceneParam.photonParams.y;
}

float getSpectrumMode()
{
    return gSceneParam.photonParams.z;
}

float getMaxRecursionDepth()
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
inline int SerialRaysIndex(int3 dispatchRaysIndex, int3 dispatchRaysDimensions)
{
    return (dispatchRaysIndex.x + dispatchRaysDimensions.x * dispatchRaysIndex.y) * dispatchRaysDimensions.z + dispatchRaysIndex.z;
}

inline float2 ComputeInterpolatedAttributeF2(float2 vertexAttributeTbl[3], float2 barycentrics)
{
    float2 ret;
    ret = vertexAttributeTbl[0];
    ret += barycentrics.x * (vertexAttributeTbl[1] - vertexAttributeTbl[0]);
    ret += barycentrics.y * (vertexAttributeTbl[2] - vertexAttributeTbl[0]);
    return ret;
}

float3 ComputeInterpolatedAttributeF3(float3 vertexAttributeTbl[3], float2 barycentrics)
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
    if (payload.recursive >= getMaxRecursionDepth())
    {
        payload.color = float3(0, 0, 0);
        return true;
    }
    return false;
}

inline bool isReachedRecursiveLimitPhotonPayload(inout PhotonPayload payload)
{
    payload.recursive++;
    if (payload.recursive >= getMaxRecursionDepth())
    {
        payload.throughput = float3(0, 0, 0);
        return true;
    }
    return false;
}

float3x3 RodriguesRoatationFormula(float theta, float3 n)
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

    float z = (pcgHashState() + randSeed) * (1.0f - cosAngle) + cosAngle;
    float phi = (pcgHashState() + randSeed) * 2.0f * PI;

    float x = sqrt(1.0f - z * z) * cos(phi);
    float y = sqrt(1.0f - z * z) * sin(phi);
    float3 north = float3(0.f, 0.f, 1.f);

    float3 axis = normalize(cross(north, normalize(direction)));
    float angle = acos(dot(normalize(direction), north));

    float3x3 R = RodriguesRoatationFormula(angle, axis);

    return mul(R, float3(x, y, z));
}

float3 getUniformSample(in uint2 id, float3 direction, float coneAngle)
{
    float cosAngle = cos(coneAngle);

    float z = cosAngle;
    float phi = 0;

    float x = sqrt(1.0f - z * z) * cos(phi);
    float y = sqrt(1.0f - z * z) * sin(phi);
    float3 north = float3(0.f, 0.f, 1.f);

    float3 axis = normalize(cross(north, normalize(direction)));
    float angle = acos(dot(normalize(direction), north));

    float3x3 R = RodriguesRoatationFormula(angle, axis);

    return mul(R, float3(x, y, z));
}

////////////////////////////////////
//GBuffer
////////////////////////////////////
float compute01Depth(float3 wPos)
{
    matrix mtxViewProj = mul(gSceneParam.mtxView, gSceneParam.mtxProj);
    float4 svPosition = mul(mtxViewProj, float4(wPos, 1));
    float depth = (svPosition.z - getNearPlaneDistance())
     / (getFarPlaneDistance() - getNearPlaneDistance());
    float zeroOneDepth = 0.5 * (depth + 1); //near 1 to far 0
    return zeroOneDepth;
}

void depthPositionNormalStore(inout Payload payload, in float3 normal, bool isMiss = false)
{
    if (payload.stored == 0)
    {
        float3 wPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        float depth = compute01Depth(wPos);
        float2 writeIndex = payload.storeIndexXY;
        gDepthBuffer[writeIndex] = isMiss ? 0 : depth;
        gPositionBuffer[writeIndex] = float4(wPos.x, wPos.y, wPos.z, 0);
        gNormalBuffer[writeIndex] = float4(normal.x, normal.y, normal.z, 0);
        payload.stored = 1;
    }
}

float depthLoad(uint2 index)
{
    return gDepthBuffer[index];
}

#endif//__COMMON_HLSLI__