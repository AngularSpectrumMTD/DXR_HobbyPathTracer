#include "sceneCBDefinition.hlsli"
ConstantBuffer<SceneCB> gSceneParam : register(b0);
#include "sceneParamInterface.hlsli"

#include "compressionUtility.hlsli"
#include "materialParams.hlsli"

#define THREAD_NUM 16

Texture2D<float4> NormalDepthBuffer : register(t0);

RWStructuredBuffer<CompressedMaterialParams> ScreenSpaceMaterial : register(u0);
RWTexture2D<uint> AccumulationCountBuffer : register(u1);
RWTexture2D<float4> NativePathtracedResult : register(u2);
RWTexture2D<float4> FinalizedPathtracedResult : register(u3);

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
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

#define minMaxSwap(a, b)				temp = a; a = min(a, b); b = max(temp, b);
#define mn3(a, b, c)			minMaxSwap(a, b); minMaxSwap(a, c);
#define mx3(a, b, c)			minMaxSwap(b, c); minMaxSwap(a, c);

#define minMax3(a, b, c)			mx3(a, b, c); minMaxSwap(a, b);
#define minMax4(a, b, c, d)		minMaxSwap(a, b); minMaxSwap(c, d); minMaxSwap(a, c); minMaxSwap(b, d);
#define minMax5(a, b, c, d, e)	minMaxSwap(a, b); minMaxSwap(c, d); mn3(a, c, e); mx3(b, d, e);
#define minMax6(a, b, c, d, e, f) minMaxSwap(a, d); minMaxSwap(b, e); minMaxSwap(c, f); mn3(a, b, c); mx3(d, e, f);

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void finalizePathtracedResult(uint3 dtid : SV_DispatchThreadID)
{
    float2 dims;
    FinalizedPathtracedResult.GetDimensions(dims.x, dims.y);

    uint2 currID = dtid.xy;

    if(!isWithinBounds(currID, dims))
    {
        return;
    }

    MaterialParams material = decompressMaterialParams(ScreenSpaceMaterial[currID.x + dims.x * currID.y]);
    const bool isTransparent = isTransparentMaterial(material);

    float4 finalCol = NativePathtracedResult[currID];
    float4 origCol = finalCol;

    const uint accumulationCpunt = AccumulationCountBuffer[currID];

    if(isTransparent && (accumulationCpunt > 1))
    {
        float3 v[9];
        const float range = 1;

        for(int dX = -1; dX <= 1; ++dX) {
            for(int dY = -1; dY <= 1; ++dY) {		
                float2 offset = float2(float(dX), float(dY));
                v[(dX + 1) * 3 + (dY + 1)] = NativePathtracedResult[currID + range * offset].rgb;
            }
        }

        float3 temp;

        if(isUseMedianFiltering())
        {
            minMax6(v[0], v[1], v[2], v[3], v[4], v[5]);
            minMax5(v[1], v[2], v[3], v[4], v[6]);
            minMax4(v[2], v[3], v[4], v[7]);
            minMax3(v[3], v[4], v[8]);
        }
        
        finalCol = float4(v[4],1);

        if(computeLuminance(origCol.rgb) <= computeLuminance(finalCol.rgb))
        {
            finalCol = origCol;
        }
    }

    FinalizedPathtracedResult[currID] = finalCol;
}