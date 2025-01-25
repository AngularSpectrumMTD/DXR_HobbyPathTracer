uint generateHash(uint orig)
{
    orig ^= 2747636419u;
    orig *= 2654435769u;
    orig ^= orig >> 16;
    orig *= 2654435769u;
    orig ^= orig >> 16;
    orig *= 2654435769u;
    return orig;
}

float rand1D(uint seed)
{
    return float(generateHash(seed)) / 4294967295.0;
}

float rand(inout uint randomState)//0-1
{
    randomState += 1.0;
    return rand1D(randomState);
}

uint generateRandomSeed(uint2 xy, uint dimX)
{
    return generateHash(xy.x) % 10000 + generateHash(xy.y) * dimX % 10000 + getLightRandomSeed();
}

float2 sample2DGaussianBoxMuller(in float rand0, in float rand1)
{
    const float r = sqrt(max(-2.0f * log(rand0), 0.0f));
    const float phi = 2.0f * PI * rand1;
    float2 sncs = 0.xx;
    sincos(phi, sncs.x, sncs.y);
    return r * float2(sncs.y, sncs.x);
}