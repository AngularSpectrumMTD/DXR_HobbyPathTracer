#include "weightsUtility.hlsli"

struct WaveletCB
{
    uint stepScale; //pow(2, iter)
    float sigmaScale; //pow(2, -iter)
    float clrSigma;
    float nmlSigma;
    float posSigma;
};

ConstantBuffer<WaveletCB> gWaveletParam : register(b0);

Texture2D<float4> colorBufferSrc : register(t0);
Texture2D<float> depthBuffer : register(t1);
Texture2D<float4> normalBuffer : register(t2);
Texture2D<float> varianceBufferSrc : register(t3);

RWTexture2D<float4> colorBufferDst : register(u0);
RWTexture2D<float> varianceBufferDst : register(u1);

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void A_TrousWaveletFilter(uint3 dtid : SV_DispatchThreadID)
{
    int2 computePix = dtid.xy;
    int2 bufferSize = 0.xx;
    colorBufferSrc.GetDimensions(bufferSize.x, bufferSize.y);
    
    const float variance = varianceBufferSrc[computePix];
    const float3 currentColor = colorBufferSrc[computePix].rgb;
    const float luminance = computeLuminance(currentColor);
    const float depth = depthBuffer[computePix];
    const float3 normal = normalBuffer[computePix].xyz;
    const float dx = computePix.x < bufferSize.x / 2 ? 1 : -1;
    const float dy = computePix.x < bufferSize.x / 2 ? 1 : -1;
    const float xDepth = depthBuffer[computePix + int2(dx, 0)];
    const float yDepth = depthBuffer[computePix + int2(0, dy)];
    const float dzdx = (xDepth - depth) * dx;
    const float dzdy = (yDepth - depth) * dy;
    
    float meanLocal = 0;
    //apply filter to variance for stabilization
    {
        float varSum = 0;
        float wSum = 0;
        int i = 0, j = 0;
        for (i = -1; i <= 1; i++)
        {
            float ky = gaussKernel[i + 1];
            for (j = -1; j <= 1; j++)
            {
                int2 neighborPix = computePix + int2(j, i);
                if (!isInsideBounds(neighborPix, bufferSize))
                {
                    continue;
                }
                float w = gaussKernel[j + 1] * ky;
                varSum += w * varianceBufferSrc[neighborPix];
                wSum += w;
            }
        }
        meanLocal = sqrt(varSum / wSum);
    }
    
    //a-trous wavelet
    {
        const float waveletW_center = waveletWeight[WAVELET_CENTER];
        float wSum = waveletW_center;
        float3 filteredColor = waveletW_center * currentColor;
        float filteredVar = waveletW_center * waveletW_center * variance;
        int i = 0;
        for (i = 0; i < WAVELET_FILTERELEMENTS; i++)
        {
            if (i == WAVELET_CENTER)
            {
                continue;
            }
            int2 ofs = int2(waveletOffset[i].x * gWaveletParam.stepScale, waveletOffset[i].y * gWaveletParam.stepScale);
            int2 neighborPix = computePix + ofs;
            if (!isInsideBounds(neighborPix, bufferSize))
            {
                continue;
            }
            float w = waveletWeight[i];
            float neighborDepth = depthBuffer[neighborPix];
            float3 neighborNormal = normalBuffer[neighborPix].rgb;
            float neighborLuminance = computeLuminance(colorBufferSrc[neighborPix].rgb);
            
            float wDepth = (depth == 0) ? 1 : depthWeight(depth, neighborDepth, dzdx, dzdy, ofs.x, ofs.y);
            float wNormal = normalWeight(normal, neighborNormal);
            float wLuminance = luminanceWeight(luminance, neighborLuminance, meanLocal);//if mean is small, weight reaches to 0
            float wComposite = w * wDepth * wNormal * wLuminance;//IBL vanished by DepthWeight

            filteredColor += wComposite * colorBufferSrc[neighborPix].rgb;
            filteredVar += wComposite * wComposite * varianceBufferSrc[neighborPix];
            wSum += wComposite;
        }
        filteredColor /= wSum;
        filteredVar /= wSum * wSum;
        
        colorBufferDst[computePix] = float4(filteredColor, 1);
        varianceBufferDst[computePix] = filteredVar;
    }
}