#include "weightsUtility.hlsli"

Texture2D<float> depthBuffer : register(t0);
Texture2D<float4> normalBuffer : register(t1);
Texture2D<float2> luminanceMomentBuffer : register(t2);

RWTexture2D<float> varianceBuffer : register(u0);

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void computeVariance(uint3 dtid : SV_DispatchThreadID)
{
    int2 computePix = dtid.xy;
    int2 bufferSize = 0.xx;
    depthBuffer.GetDimensions(bufferSize.x, bufferSize.y);

    const float depth = depthBuffer[computePix];
    const float3 normal = normalBuffer[computePix].xyz;
    const float dx = computePix.x < bufferSize.x / 2 ? 1 : -1;
    const float dy = computePix.y < bufferSize.y / 2 ? 1 : -1;
    const float xDepth = depthBuffer[computePix + int2(dx, 0)];
    const float yDepth = depthBuffer[computePix + int2(0, dy)];
    const float dzdx = (xDepth - depth) * dx;
    const float dzdy = (yDepth - depth) * dy;

    float momentFirst = 0;
    float momentSecond = 0;
    float wSum = 0;
    
    int i = 0, j = 0;
    for (i = -FILTER_FOOTPRINT; i < FILTER_FOOTPRINT; i++)
    {
        float ky = cKernel[i + FILTER_FOOTPRINT];
        for (j = -FILTER_FOOTPRINT; j < FILTER_FOOTPRINT; j++)
        {
            if (i == 0 && j == 0)
            {
                continue;
            }
            
            int2 neighborPix = computePix + int2(j, i);
            if (!isInsideBounds(neighborPix, bufferSize))
            {
                continue;
            }
            float neighborDepth = depthBuffer[neighborPix];
            float3 neighborNormal = normalBuffer[neighborPix].xyz;

            float wDepth = depthWeight(depth, neighborDepth, dzdx, dzdy, j, i);
            float wNormal = normalWeight(normal, neighborNormal);
            float wComposite = cKernel[j + FILTER_FOOTPRINT] * ky * wDepth * wNormal;
            
            float2 moments = luminanceMomentBuffer[neighborPix];
            momentFirst += wComposite * moments.x;
            momentSecond += wComposite * moments.y;
            wSum += wComposite;
        }
    }
    
    momentFirst /= wSum;
    momentSecond /= wSum;

    varianceBuffer[computePix] = max(momentSecond - momentFirst * momentFirst, 0);
}