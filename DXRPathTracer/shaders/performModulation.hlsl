#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "spectralRenderingHelper.hlsli"
#include "reservoir.hlsli"

#define THREAD_NUM 16

RWTexture2D<float4> CurrentDIBuffer : register(u0);
RWTexture2D<float4> CurrentGIBuffer : register(u1);
RWTexture2D<float4> CurrentCausticsBuffer : register(u2);
RWStructuredBuffer<CompressedMaterialParams> ScreenSpaceMaterial : register(u3);

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void performModulation(uint2 dtid : SV_DispatchThreadID)
{
    float2 dims;
    CurrentDIBuffer.GetDimensions(dims.x, dims.y);

    MaterialParams material = decompressMaterialParams(ScreenSpaceMaterial[dtid.x + dims.x * dtid.y]);
    
    const float3 modAlbedo = modulatedAlbedo(material);

    float3 currDI = CurrentDIBuffer[dtid].rgb;
    float3 currGI = CurrentGIBuffer[dtid].rgb;
    float3 currCaustics = CurrentCausticsBuffer[dtid].rgb;

    //modulate
    currDI /= modAlbedo;
    currGI /= modAlbedo;
    currCaustics /= modAlbedo;

    CurrentDIBuffer[dtid].rgb = currDI;
    CurrentGIBuffer[dtid].rgb = currGI;
    CurrentCausticsBuffer[dtid].rgb = currCaustics;
}