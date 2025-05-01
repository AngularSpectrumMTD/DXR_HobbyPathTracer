#define THREAD_NUM 16

RWTexture2D<float> gPhotonEmissionGuideMapSrc : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMapDst : register(u1);

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void copyPhotonEmissionGuideMap(uint3 dtid : SV_DispatchThreadID)
{
    int2 dims;
    gPhotonEmissionGuideMapDst.GetDimensions(dims.x, dims.y);

    if(!isWithinBounds(dtid.xy, dims))
    {
        return;
    }

    gPhotonEmissionGuideMapDst[dtid.xy] = gPhotonEmissionGuideMapSrc[dtid.xy];
}