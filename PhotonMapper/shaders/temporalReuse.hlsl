#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "spectralRenderingHelper.hlsli"
#include "reservoir.hlsli"

#define THREAD_NUM 16
#define REINHARD_L 1000
#define MAX_ACCUMULATION_RANGE 1000

StructuredBuffer<DIReservoir> DIReservoirBufferSrc : register(t0);
Texture2D<float4> NormalDepthBuffer : register(t1);
Texture2D<float4> PrevNormalDepthBuffer : register(t2);
Texture2D<float2> PrevIDBuffer : register(t3);
Texture2D<float4> PositionBuffer : register(t4);
Texture2D<float4> PrevPositionBuffer : register(t5);
StructuredBuffer<GIReservoir> GIReservoirBufferSrc : register(t6);
RWStructuredBuffer<DIReservoir> DIReservoirBufferDst : register(u0);
RWStructuredBuffer<GIReservoir> GIReservoirBufferDst : register(u1);

static uint rseed;

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

float rand(in int2 indexXY)//0-1
{
    rseed += 1.0;
    return frac(sin(dot(indexXY.xy, float2(12.9898, 78.233)) * (getLightRandomSeed() + 1) * 0.001 + rseed) * 43758.5453);
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
    int2 dims;
    NormalDepthBuffer.GetDimensions(dims.x, dims.y);

    uint2 currID = dtid.xy;

    float currDepth = NormalDepthBuffer[currID].w;
    float3 currNormal = NormalDepthBuffer[currID].xyz;

    float3 currObjectWorldPos = PositionBuffer[currID].xyz;

    int2 prevID = PrevIDBuffer[currID];

    float3 currDI = 0.xxx;
    if(isUseNEE() && isUseStreamingRIS())
    {
        const uint serialCurrID = currID.y * dims.x + currID.x;
        const uint serialPrevID = clamp(prevID.y * dims.x + prevID.x, 0, dims.x * dims.y - 1);
        DIReservoir currDIReservoir = DIReservoirBufferDst[serialCurrID];

        if (isUseReservoirTemporalReuse() && isWithinBounds(prevID, dims))
        {
            float prevDepth = PrevNormalDepthBuffer[prevID].w;
            float3 prevNormal = PrevNormalDepthBuffer[prevID].xyz;
            float3 prevObjectWorldPos = PrevPositionBuffer[prevID].xyz;
            const bool isTemporalReuseEnable = isTemporalReprojectionEnable(currDepth, prevDepth, currNormal, prevNormal, currObjectWorldPos, prevObjectWorldPos);
            if(isTemporalReuseEnable)
            {
                DIReservoir prevDIReservoir = DIReservoirBufferSrc[serialPrevID];
                DIReservoirTemporalReuse(currDIReservoir, prevDIReservoir, currID);
            }
        }

        DIReservoirBufferDst[serialCurrID] = currDIReservoir;
    }
}