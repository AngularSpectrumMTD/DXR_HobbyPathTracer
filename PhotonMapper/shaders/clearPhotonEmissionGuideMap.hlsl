#define THREAD_NUM 16

RWTexture2D<float> gPhotonEmissionGuideMap0 : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMap1 : register(u1);
RWTexture2D<float> gPhotonEmissionGuideMap2 : register(u2);
RWTexture2D<float> gPhotonEmissionGuideMap3 : register(u3);
RWTexture2D<float> gPhotonEmissionGuideMap4 : register(u4);
RWTexture2D<float> gPhotonEmissionGuideMap5 : register(u5);
RWTexture2D<float> gPhotonEmissionGuideMap6 : register(u6);

void clearEmissionGuideMap(int3 launchIndex)
{
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap0.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap0[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap1.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap1[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap2.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap2[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap3.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap3[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap4.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap4[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap5.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap5[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap6.GetDimensions(size.x, size.y);
        if ((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap6[launchIndex.xy] = 0;
        }
    }
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void clearPhotonEmissionGuideMap(uint3 dtid : SV_DispatchThreadID)
{
    clearEmissionGuideMap(dtid);
}