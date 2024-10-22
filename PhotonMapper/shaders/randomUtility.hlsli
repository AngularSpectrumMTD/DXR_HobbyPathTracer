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

uint generateRandomInitialRandomSeed(uint2 xy, uint dimX)
{
    return generateHash(xy.x) % 10000 + generateHash(xy.y) * dimX % 10000 + getLightRandomSeed();
}