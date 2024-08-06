#define THREAD_NUM 16

RWTexture2D<uint> gPhotonRandomCounterMap : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMap : register(u1);

static const float gaussKernel[3] = { 1 / 4.0f, 1 / 2.0f, 1 / 4.0f };

//restrict
bool isWithinBounds(int2 id, int2 size)
{
    return ((0 <= id.x) && (id.x <= (size.x - 1))) && ((0 <= id.y) && (id.y <= (size.y - 1)));
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void generateEmissionGuideMap(uint3 dtid : SV_DispatchThreadID)
{
    int2 dims;
    gPhotonRandomCounterMap.GetDimensions(dims.x, dims.y);

    float sum = 0;
    float sumW = 0;

    int i = 0;
    int j = 0;

    for(i = -1; i <= 1; i++)
    {
        for(j = -1; j <= 1; j++)
        {
            int2 pos = dtid.xy + int2(j, i);
            float w = gaussKernel[i + 1] * gaussKernel[j + 1];

            if(!isWithinBounds(pos, dims))
            {
                continue;
            }

            sum += w * gPhotonRandomCounterMap[pos];
            sumW += w;
        }
    }

    gPhotonEmissionGuideMap[dtid.xy] = sum / sumW;
}