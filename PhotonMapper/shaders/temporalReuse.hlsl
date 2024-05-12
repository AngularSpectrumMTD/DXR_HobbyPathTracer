#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "spectralRenderingHelper.hlsli"
#include "reservoir.hlsli"

#define THREAD_NUM 16
#define REINHARD_L 1000
#define MAX_ACCUMULATION_RANGE 1000

Texture2D<float4> HistoryDIBuffer : register(t0);
Texture2D<float4> HistoryGIBuffer : register(t1);
Texture2D<float4> HistoryCausticsBuffer : register(t2);
Texture2D<float> DepthBuffer : register(t3);
Texture2D<float> PrevDepthBuffer : register(t4);
Texture2D<float2> VelocityBuffer : register(t5);
Texture2D<float2> LuminanceMomentBufferSrc : register(t6);
StructuredBuffer<DIReservoir> DIReservoirBufferSrc : register(t7);

RWTexture2D<float4> CurrentDIBuffer : register(u0);
RWTexture2D<float4> CurrentGIBuffer : register(u1);
RWTexture2D<float4> CurrentCausticsBuffer : register(u2);
RWTexture2D<float4> DIGIBuffer : register(u3);
RWTexture2D<uint> AccumulationCountBuffer : register(u4);
RWTexture2D<float2> LuminanceMomentBufferDst : register(u5);
RWStructuredBuffer<DIReservoir> DIReservoirBufferDst : register(u6);

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

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void temporalReuse(uint3 dtid : SV_DispatchThreadID)
{
    float2 dims;
    CurrentDIBuffer.GetDimensions(dims.x, dims.y);

    uint2 currID = dtid.xy;

    float currDepth = DepthBuffer[currID];
    uint accCount = AccumulationCountBuffer[currID];

    float2 velocity = VelocityBuffer[currID] * 2.0 - 1.0;
    uint2 prevID = currID;//(ID / dims - velocity) * dims;

    float3 currDI = 0.xxx;
    if(isUseNEE() && isUseWRS_RIS())
    {
        DIReservoir currDIReservoir = DIReservoirBufferDst[currID.y * dims.x + currID.x];
        float3 reservoirElementRemovedDI = CurrentDIBuffer[currID].rgb;
        currDI = shadeDIReservoir(currDIReservoir) + reservoirElementRemovedDI;
    }
    else
    {
        currDI = CurrentDIBuffer[currID].rgb;
    }
    float3 currGI = CurrentGIBuffer[currID].rgb;
    float3 currCaustics = CurrentCausticsBuffer[currID].rgb;
    float3 prevDI = HistoryDIBuffer[prevID].rgb;
    float3 prevGI = HistoryGIBuffer[prevID].rgb;
    float3 prevCaustics = HistoryCausticsBuffer[prevID].rgb;
    float3 currDIGI = currDI + currGI;

    float prevDepth = PrevDepthBuffer[prevID];
    float2 prevLuminanceMoment = LuminanceMomentBufferSrc[prevID];

    float luminance = computeLuminance(currDIGI);
    float2 curremtLuminanceMoment = float2(luminance, luminance * luminance);
    
    if (isAccumulationApply())
    {
        accCount++;
    }
    else
    {
        accCount = 1;
    }
    AccumulationCountBuffer[currID] = accCount;

    if (accCount < MAX_ACCUMULATION_RANGE)
    {
        const float tmpAccmuRatio = 1.f / accCount;

        float3 accumulatedDI = lerp(prevDI, currDI, tmpAccmuRatio);
        float3 accumulatedGI = lerp(prevGI, currGI, tmpAccmuRatio);
        float3 accumulatedDIGI = accumulatedDI + accumulatedGI;
        float3 accumulatedCaustics = lerp(prevCaustics, currCaustics, tmpAccmuRatio);

        curremtLuminanceMoment.x = lerp(prevLuminanceMoment.x, curremtLuminanceMoment.x, tmpAccmuRatio);
        curremtLuminanceMoment.y = lerp(prevLuminanceMoment.y, curremtLuminanceMoment.y, tmpAccmuRatio);

        float3 toneMappedDIGI = float3(accumulatedDIGI * reinhard(computeLuminance(accumulatedDIGI), REINHARD_L) / computeLuminance(accumulatedDIGI));//luminance based tone mapping
        DIGIBuffer[currID].rgb = toneMappedDIGI + getCausticsBoost() * mul(accumulatedCaustics, XYZtoRGB2);
        CurrentDIBuffer[currID].rgb = accumulatedDI;
        CurrentGIBuffer[currID].rgb = accumulatedGI;
        CurrentCausticsBuffer[currID].rgb = accumulatedCaustics;
        LuminanceMomentBufferDst[currID] = curremtLuminanceMoment;
    }
}