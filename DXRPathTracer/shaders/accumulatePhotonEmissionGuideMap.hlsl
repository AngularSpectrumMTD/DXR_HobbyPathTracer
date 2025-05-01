#define THREAD_NUM 16

RWTexture2D<float> gPhotonEmissionGuideMapPrev : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMapCurr : register(u1);

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void accumulatePhotonEmissionGuideMap(uint3 dtid : SV_DispatchThreadID)
{
    int2 dims;
    gPhotonEmissionGuideMapCurr.GetDimensions(dims.x, dims.y);

    if(!isWithinBounds(dtid.xy, dims))
    {
        return;
    }

    const float blendRatio = 0.1;
    const float curr = gPhotonEmissionGuideMapCurr[dtid.xy];
    const float prev = gPhotonEmissionGuideMapPrev[dtid.xy];

    gPhotonEmissionGuideMapCurr[dtid.xy] = lerp(curr, prev, blendRatio);
}