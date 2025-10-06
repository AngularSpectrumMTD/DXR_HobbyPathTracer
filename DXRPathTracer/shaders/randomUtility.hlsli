uint generateHash(inout uint orig)
{
#if 0
    orig ^= 2747636419u;
    orig *= 2654435769u;
    orig ^= orig >> 16;
    orig *= 2654435769u;
    orig ^= orig >> 16;
    orig *= 2654435769u;
    return orig;
#else
    uint z = orig += 0x6D2B79F5;
    z = (z ^ z >> 15u) * (1u | z);
    z ^= z + (z^z >> 7u) * (61u | z);
    return z^z >> 14u;
#endif
}

float rand1D(inout uint seed)
{
#if 0
    return float(generateHash(seed)) / 4294967295.0;
#else
    return asfloat(0x3F800000 | (generateHash(seed) & 0x7FFFFF)) - 1;
#endif
}

float rand(inout uint randomState)//0-1
{
    return rand1D(randomState);
}

uint generateRandomSeed(uint2 xy, uint dimX)
{
    uint v = xy.x + xy.y * dimX + getLightRandomSeed();
    return generateHash(v);
}

float2 sample2DGaussianBoxMuller(in float rand0, in float rand1)
{
    const float r = sqrt(max(-2.0f * log(rand0), 0.0f));
    const float phi = 2.0f * PI * rand1;
    float2 sncs = 0.xx;
    sincos(phi, sncs.x, sncs.y);
    return r * float2(sncs.y, sncs.x);
}