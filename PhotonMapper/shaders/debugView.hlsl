#define THREAD_NUM 16

RWTexture2D<float4> diffuseAlbedoBuffer : register(u0);
RWTexture2D<float> depthBuffer : register(u1);
RWTexture2D<float4> normalBuffer : register(u2);

RWTexture2D<float4> finalColor : register(u3);

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void debugView(uint3 dtid : SV_DispatchThreadID)
{
    int2 computePix = dtid.xy;
    int2 bufferSize = 0.xx;
    finalColor.GetDimensions(bufferSize.x, bufferSize.y);
    
    float3 finalColorSrc = finalColor[computePix].xyz;
    float3 diffuseAlbedo = diffuseAlbedoBuffer[computePix].xyz;
    float3 depth = float3(depthBuffer[computePix], 0, 0);
    float3 normal = (normalBuffer[computePix].xyz + 1.xxx) * 0.5;

    const uint offsetX = (bufferSize.x * 2.0f / 3);
    const uint offsetY = (bufferSize.x * 1.0f / 3) * (bufferSize.y * 1.0f / bufferSize.x);
    
    if (offsetX < computePix.x)
    {
        if (0 < computePix.y && computePix.y < 1 * offsetY)
        {
            finalColorSrc = diffuseAlbedoBuffer[3 * (computePix - int2(offsetX, 0))].xyz;
        }
        else if (1 * offsetY <= computePix.y && computePix.y < 2 * offsetY)
        {
            finalColorSrc = float3(1 - depthBuffer[3 * (computePix - int2(offsetX, offsetY))], 0, 0);
        }
        else if (2 * offsetY <= computePix.y && computePix.y < 3 * offsetY)
        {
            finalColorSrc = (normalBuffer[3 * (computePix - int2(offsetX, 2 * offsetY))].xyz + 1.xxx) * 0.5;
        }
    }
    finalColor[computePix].xyz = finalColorSrc;
}