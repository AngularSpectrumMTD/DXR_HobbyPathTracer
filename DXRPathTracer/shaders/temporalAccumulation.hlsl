#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "spectralRenderingHelper.hlsli"
#include "reservoir.hlsli"

#define THREAD_NUM 16
#define MAX_ACCUMULATION_RANGE 10000

Texture2D<float4> HistoryDIBuffer : register(t0);
Texture2D<float4> HistoryGIBuffer : register(t1);
Texture2D<float4> HistoryCausticsBuffer : register(t2);
Texture2D<float4> NormalDepthBuffer : register(t3);
Texture2D<float4> PrevNormalDepthBuffer : register(t4);
Texture2D<float2> PrevIDBuffer : register(t5);
Texture2D<float2> LuminanceMomentBufferSrc : register(t6);
StructuredBuffer<DIReservoir> DIReservoirBufferSrc : register(t7);
Texture2D<float4> PositionBuffer : register(t8);
Texture2D<float4> PrevPositionBuffer : register(t9);
StructuredBuffer<GIReservoir> GIReservoirBufferSrc : register(t10);

RWTexture2D<float4> CurrentDIBuffer : register(u0);
RWTexture2D<float4> CurrentGIBuffer : register(u1);
RWTexture2D<float4> CurrentCausticsBuffer : register(u2);
RWTexture2D<float4> DIGIBuffer : register(u3);
RWTexture2D<uint> AccumulationCountBuffer : register(u4);
RWTexture2D<float2> LuminanceMomentBufferDst : register(u5);
RWTexture2D<uint> PrevAccumulationCountBuffer : register(u6);
RWStructuredBuffer<CompressedMaterialParams> ScreenSpaceMaterial : register(u7);

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

float computeLuminance(const float3 linearRGB)
{
    return dot(float3(0.2126, 0.7152, 0.0722), linearRGB);
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void temporalAccumulation(uint3 dtid : SV_DispatchThreadID)
{
    float2 dims;
    CurrentDIBuffer.GetDimensions(dims.x, dims.y);

    uint2 currID = dtid.xy;

    float currDepth = NormalDepthBuffer[currID].w;
    float3 currNormal = NormalDepthBuffer[currID].xyz;

    float3 currObjectWorldPos = PositionBuffer[currID].xyz;

    int2 prevID = PrevIDBuffer[currID];

    float3 currDI = 0.xxx;
    float3 currGI = 0.xxx;

    MaterialParams material = decompressMaterialParams(ScreenSpaceMaterial[currID.x + dims.x * currID.y]);
    if(isUseNEE() && isUseStreamingRIS())
    {
        const uint serialCurrID = currID.y * dims.x + currID.x;
        DIReservoir currDIReservoir = DIReservoirBufferSrc[serialCurrID];
        GIReservoir currGIReservoir = GIReservoirBufferSrc[serialCurrID];

        float3 reservoirElementRemovedDI = CurrentDIBuffer[currID].rgb;
        currDI = (isIndirectOnly() ? 0.xxx : resolveDIReservoir(currDIReservoir)) + reservoirElementRemovedDI;

        const float w = material.roughness * material.roughness;
        float3 reservoirElementRemovedGI = CurrentGIBuffer[currID].rgb;
        currGI = w * resolveGIReservoir(currGIReservoir) + (1 - w) * reservoirElementRemovedGI;
    }
    else
    {
        currDI = CurrentDIBuffer[currID].rgb;
    }
    
    const float3 modAlbedo = modulatedAlbedo(material);

    float3 currCaustics = CurrentCausticsBuffer[currID].rgb;

    //modulate
    currDI /= modAlbedo;
    currGI /= modAlbedo;
    currCaustics /= modAlbedo;

    if(isWithinBounds(prevID, dims) && isAccumulationApply())
    {
        float3 prevDI = HistoryDIBuffer[prevID].rgb;
        float3 prevGI = HistoryGIBuffer[prevID].rgb;
        float3 prevCaustics = HistoryCausticsBuffer[prevID].rgb;
        float3 currDIGI = currDI + currGI;

        float prevDepth = PrevNormalDepthBuffer[prevID].w;
        float3 prevNormal = PrevNormalDepthBuffer[prevID].xyz;
        float3 prevWorldPos = PrevPositionBuffer[prevID].xyz;
        float2 prevLuminanceMoment = LuminanceMomentBufferSrc[prevID];

        uint accCount = PrevAccumulationCountBuffer[prevID];
        const bool isTemporalReuseEnable = isTemporalReprojectionSuccessed(currDepth, prevDepth, currNormal, prevNormal, currObjectWorldPos, prevWorldPos);
        if (isTemporalReuseEnable)
        {
            accCount++;
        }
        else
        {
            accCount = 1;
        }
        
        accCount = min(accCount, MAX_ACCUMULATION_RANGE);
        AccumulationCountBuffer[currID] = accCount;
        const float tmpAccmuRatio = 1.f / accCount;

        float3 accumulatedDI = lerp(prevDI, currDI, tmpAccmuRatio);
        float3 accumulatedGI = lerp(prevGI, currGI, tmpAccmuRatio);
        float3 accumulatedDIGI = accumulatedDI + accumulatedGI;
        float3 accumulatedCaustics = lerp(prevCaustics, currCaustics, tmpAccmuRatio);

        float luminance = computeLuminance(currDIGI);
        float2 currLuminanceMoment = float2(luminance, luminance * luminance);
        currLuminanceMoment.x = lerp(prevLuminanceMoment.x, currLuminanceMoment.x, tmpAccmuRatio);
        currLuminanceMoment.y = lerp(prevLuminanceMoment.y, currLuminanceMoment.y, tmpAccmuRatio);

        float3 renderCaustics = accumulatedCaustics;
// #ifdef USE_SPECTRAL_RENDERED_CAUSTICS
//         renderCaustics = max(0.xxx, mul(renderCaustics, XYZtoRGB2));
// #endif

        DIGIBuffer[currID].rgb = accumulatedDIGI + getCausticsBoost() * renderCaustics;
        CurrentDIBuffer[currID].rgb = accumulatedDI;
        CurrentGIBuffer[currID].rgb = accumulatedGI;
        CurrentCausticsBuffer[currID].rgb = accumulatedCaustics;
        LuminanceMomentBufferDst[currID] = currLuminanceMoment;
    }
    else
    {
        AccumulationCountBuffer[currID] = 1;
        float3 currDIGI = currDI + currGI;

        float3 renderCaustics = currCaustics;
// #ifdef USE_SPECTRAL_RENDERED_CAUSTICS
//         renderCaustics = max(0.xxx, mul(renderCaustics, XYZtoRGB2));
// #endif

        DIGIBuffer[currID].rgb = currDIGI + getCausticsBoost() * renderCaustics;
        CurrentDIBuffer[currID].rgb = currDI;
        CurrentGIBuffer[currID].rgb = currGI;
        CurrentCausticsBuffer[currID].rgb = currCaustics;
        float luminance = computeLuminance(currDIGI);
        float2 currLuminanceMoment = float2(luminance, luminance * luminance);
        LuminanceMomentBufferDst[currID] = currLuminanceMoment;
    }
}