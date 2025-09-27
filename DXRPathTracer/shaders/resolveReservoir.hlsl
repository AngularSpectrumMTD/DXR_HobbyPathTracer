#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "spectralRenderingHelper.hlsli"
#include "reservoir.hlsli"

#define THREAD_NUM 16
#define MAX_ACCUMULATION_RANGE 10000

StructuredBuffer<DIReservoir> DIReservoirBufferSrc : register(t0);
StructuredBuffer<GIReservoir> GIReservoirBufferSrc : register(t1);

RWTexture2D<float4> CurrentDIBuffer : register(u0);
RWTexture2D<float4> CurrentGIBuffer : register(u1);
RWStructuredBuffer<CompressedMaterialParams> ScreenSpaceMaterial : register(u2);

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void resolveReservoir(uint3 dtid : SV_DispatchThreadID)
{
    float2 dims;
    CurrentDIBuffer.GetDimensions(dims.x, dims.y);

    uint2 currID = dtid.xy;

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

        const float wBase = (material.roughness < 0.4) ? material.roughness : 0.7;
        float w = isTransparentMaterial(material) ? 0 : wBase;
        float3 reservoirElementRemovedGI = CurrentGIBuffer[currID].rgb;
        if(!isUseReservoirSpatialReuse() && !isTransparentMaterial(material))
        {
            w = 1;
        }
        currGI = w * resolveGIReservoir(currGIReservoir) + (1 - w) * reservoirElementRemovedGI;
    }
    else
    {
        currDI = CurrentDIBuffer[currID].rgb;
    }
    
    CurrentDIBuffer[currID].rgb = currDI;
    CurrentGIBuffer[currID].rgb = currGI;
}