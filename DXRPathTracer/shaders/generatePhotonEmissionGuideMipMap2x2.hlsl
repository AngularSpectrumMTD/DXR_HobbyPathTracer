RWTexture2D<float> gPhotonEmissionGuideMap0 : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMap1 : register(u1);

[numthreads(1, 1, 1)]
void generatePhotonEmissionGuideMipMap2x2(int3 dispatchThreadID : SV_DispatchThreadID)
{
    gPhotonEmissionGuideMap1[int2(0, 0)] = 
    gPhotonEmissionGuideMap0[int2(0, 0)] +
    gPhotonEmissionGuideMap0[int2(1, 0)] +
    gPhotonEmissionGuideMap0[int2(0, 1)] +
    gPhotonEmissionGuideMap0[int2(1, 1)];
}