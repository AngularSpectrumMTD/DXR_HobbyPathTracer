#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "spectralRenderingHelper.hlsli"
#include "reservoir.hlsli"

#define THREAD_NUM 16
#define REINHARD_L 1000
#define MAX_ACCUMULATION_RANGE 1000

#define MAX_TEMPORAL_RESERVOIR_M_RATIO 10

StructuredBuffer<DIReservoir> DIReservoirBufferSrc : register(t0);
Texture2D<float> DepthBuffer : register(t1);
Texture2D<float> PrevDepthBuffer : register(t2);
Texture2D<float2> VelocityBuffer : register(t3);
RWStructuredBuffer<DIReservoir> DIReservoirBufferDst : register(u0);

static uint rseed;

float rand(in int2 indexXY)//0-1
{
    rseed += 1.0;
    return frac(sin(dot(indexXY.xy, float2(12.9898, 78.233)) * (getLightRandomSeed() + 1) * 0.001 + rseed) * 43758.5453);
}

float computeLuminance(const float3 linearRGB)
{
    return dot(float3(0.2126, 0.7152, 0.0722), linearRGB);
}

float reinhard(float x, float L)
{
    return (x / (1 + x)) * (1 + x / L / L);
}

float3 reinhard3f(float3 v, float L)
{
    return float3(reinhard(v.x, L), reinhard(v.y, L), reinhard(v.z, L));
}

float ACESFilmicTonemapping(float x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate(x * (a * x + b) / (x * (c * x + d) + c));
}

float3 ACESFilmicTonemapping3f(float3 v)
{
    return float3(ACESFilmicTonemapping(v.x), ACESFilmicTonemapping(v.y), ACESFilmicTonemapping(v.z));
}

void DIReservoirTemporalReuse(inout DIReservoir currDIReservoir, in DIReservoir prevDIReservoir, in uint2 currID)
{
    //Limitting
    if(prevDIReservoir.M > MAX_TEMPORAL_REUSE_M)
    {
        float r = max(0, ((float)MAX_TEMPORAL_REUSE_M / prevDIReservoir.M));
        prevDIReservoir.W_sum *= r;
        prevDIReservoir.M = MAX_TEMPORAL_REUSE_M;
    }

    DIReservoir tempDIReservoir;
    tempDIReservoir.initialize();
    //combine reservoirs
    {
        const float currUpdateW = currDIReservoir.W_sum;
        combineDIReservoirs(tempDIReservoir, currDIReservoir, currUpdateW, rand(currID));
        const float prevUpdateW = prevDIReservoir.W_sum;// * (prevDIReservoir.targetPDF / currDIReservoir.targetPDF);
        combineDIReservoirs(tempDIReservoir, prevDIReservoir, prevUpdateW, rand(currID));
    }
    currDIReservoir = tempDIReservoir;
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void temporalReuse(uint3 dtid : SV_DispatchThreadID)
{
    rseed = getLightRandomSeed();
    float2 dims;
    DepthBuffer.GetDimensions(dims.x, dims.y);

    uint2 currID = dtid.xy;

    float currDepth = DepthBuffer[currID];

    float2 velocity = VelocityBuffer[currID] * 2.0 - 1.0;
    uint2 prevID = currID;//(ID / dims - velocity) * dims;

    float3 currDI = 0.xxx;
    if(isUseNEE() && isUseWRS_RIS())
    {
        const uint serialCurrID = currID.y * dims.x + currID.x;
        const uint serialPrevID = prevID.y * dims.x + prevID.x;
        DIReservoir currDIReservoir = DIReservoirBufferDst[serialCurrID];

        if (isAccumulationApply() && isUseReservoirTemporalReuse())
        {
            DIReservoir prevDIReservoir = DIReservoirBufferSrc[serialPrevID];
            DIReservoirTemporalReuse(currDIReservoir, prevDIReservoir, currID);
        }

        DIReservoirBufferDst[serialCurrID] = currDIReservoir;
    }
}