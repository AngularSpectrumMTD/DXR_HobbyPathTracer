struct DenoiseCB
{
    uint stepScale;//pow(2, iter)
    float sigmaScale;//pow(2, -iter)
    float clrSigma;
    float nmlSigma;
    float posSigma;
};

    static float kernel[9] = {
        1,1,1,
        1,1,1,
        1,1,1
    };

    static int2 offset[9] = {
        int2(-1, -1),int2(+0, -1),int2(+1, -1),
        int2(-1, +0),int2(+0, +0),int2(+1, +0),
        int2(-1, +1),int2(+0, +1),int2(+1, +1),
    };

ConstantBuffer<DenoiseCB> gDenoiseParam : register(b0);
RWTexture2D<float4> gSrc: register(u0);
RWTexture2D<float4> gDst: register(u1);
RWTexture2D<float4> gPos: register(u2);
RWTexture2D<float4> gNml: register(u3);

[numthreads(16, 16, 1)]
void Denoise(uint2 threadid : SV_DispatchThreadID)
{
    float4 sum = float4(0, 0, 0, 0);
    float weightSum = 0;

    int2 centerUV = threadid.xy;

    float4 centerClr = gSrc[centerUV];
    float4 centerNml = gNml[centerUV];
    float4 centerPos = gPos[centerUV];

    float2 SIZE;
    gSrc.GetDimensions(SIZE.x, SIZE.y);

    int i = 0;
    for(i = 0; i < 9; i++)
    {
        int2 uv = centerUV + offset[i] * gDenoiseParam.stepScale;

        if(uv.x < 0 || uv.x > SIZE.x - 1 || uv.y < 0 || uv.y > SIZE.y - 1)
        {
            continue;
        }

        float4 clr = gSrc[uv];
        float4 nml = gNml[uv];
        float4 pos = gPos[uv];
        pos.w = 1;
        nml.w = 0;

        float4 delta = clr - centerClr;
        float dist2 = dot(delta, delta);
        float w_rt = min(exp(-dist2 / (gDenoiseParam.clrSigma * gDenoiseParam.sigmaScale)), 1.0);

        delta = nml - centerNml;
        dist2 = dot(delta, delta);
        float w_n = min(exp(-dist2 / (gDenoiseParam.nmlSigma * gDenoiseParam.sigmaScale)), 1.0);

        delta = pos - centerPos;
        dist2 = dot(delta, delta);
        float w_p = min(exp(-dist2 / (gDenoiseParam.posSigma * gDenoiseParam.sigmaScale)), 1.0);

        float weight = w_rt * w_n * w_p;

        sum += clr * weight * kernel[i];
        weightSum += weight * kernel[i];
    }

    gDst[centerUV] = sum / weightSum;
}