#define THREAD_NUM 16

RWTexture2D<float4> diffuseAlbedoBuffer : register(u0);
RWTexture2D<float4> normalDepthBuffer : register(u1);
RWTexture2D<float4> idRoughnessBuffer : register(u2);
RWTexture2D<float4> velocityBuffer : register(u3);

RWTexture2D<float4> finalColor : register(u4);

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void debugView(uint3 dtid : SV_DispatchThreadID)
{
    int2 computePix = dtid.xy;
    int2 bufferSize = 0.xx;
    finalColor.GetDimensions(bufferSize.x, bufferSize.y);
    
    float3 finalColorSrc = finalColor[computePix].xyz;
    float3 diffuseAlbedo = diffuseAlbedoBuffer[computePix].xyz;

    const float sizeRatio = 1.0f / 4;

    const uint offsetX = (bufferSize.x * (1 - sizeRatio));
    const uint offsetY = (bufferSize.x * sizeRatio) * (bufferSize.y * 1.0f / bufferSize.x);
    
    if (offsetX < computePix.x)
    {
        if (0 < computePix.y && computePix.y < 1 * offsetY)
        {
            finalColorSrc = diffuseAlbedoBuffer[(computePix - int2(offsetX, 0)) / sizeRatio].xyz;
        }
        else if (1 * offsetY <= computePix.y && computePix.y < 2 * offsetY)
        {
            finalColorSrc = float3(1 - normalDepthBuffer[(computePix - int2(offsetX, offsetY)) / sizeRatio].w, 0, 0);
        }
        else if (2 * offsetY <= computePix.y && computePix.y < 3 * offsetY)
        {
            finalColorSrc = (normalDepthBuffer[(computePix - int2(offsetX, 2 * offsetY)) / sizeRatio].xyz + 1.xxx) * 0.5;
        }
        else if (3 * offsetY <= computePix.y && computePix.y < 4 * offsetY)
        {
            float2 srcVelocity = velocityBuffer[(computePix - int2(offsetX, 3 * offsetY)) / sizeRatio].xy;//-1 to 1
            finalColorSrc = float3((srcVelocity.xy + 1.xx) * 0.5.xx, 0);//0 to 1
        }
    }
    finalColor[computePix].xyz = finalColorSrc;
}