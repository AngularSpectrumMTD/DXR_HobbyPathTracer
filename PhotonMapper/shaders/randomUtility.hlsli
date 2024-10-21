uint pcgHash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float hash13(float3 f3val)
{
    f3val = frac(f3val * .1031);
    f3val += dot(f3val, f3val.zyx + 31.32);
    return frac((f3val.x + f3val.y) * f3val.z);
}

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

uint generateRandomInitialRandomSeed(uint2 xy)
{
    return 10000000 * hash13(float3(xy.x, xy.y, getLightRandomSeed()));
}