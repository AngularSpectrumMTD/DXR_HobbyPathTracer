RWTexture2D<float> gPhotonEmissionGuideMap0 : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMap1 : register(u1);
RWTexture2D<float> gPhotonEmissionGuideMap2 : register(u2);

groupshared float sharedValueTbl0[8 * 8];
groupshared float sharedValueTbl1[8 * 8];

[numthreads(4, 4, 1)]
void generatePhotonEmissionGuideMipMap4x4(int groupIndex : SV_GroupIndex, int3 dispatchThreadID : SV_DispatchThreadID)
{
    sharedValueTbl0[groupIndex] = gPhotonEmissionGuideMap0.Load(dispatchThreadID.xy);
    AllMemoryBarrierWithGroupSync();
    if(groupIndex >= 4)
    {
        return;
    }

    int y = groupIndex / 2;
    int x = groupIndex - 2 * y;
    int shared_y = y * 2;
    int shared_x = x * 2;

    sharedValueTbl1[groupIndex] = 
    sharedValueTbl0[(shared_x + 0) + 4 * (shared_y + 0)] + 
    sharedValueTbl0[(shared_x + 1) + 4 * (shared_y + 0)] + 
    sharedValueTbl0[(shared_x + 0) + 4 * (shared_y + 1)] + 
    sharedValueTbl0[(shared_x + 1) + 4 * (shared_y + 1)];

    gPhotonEmissionGuideMap1[int2(x, y)] = sharedValueTbl1[groupIndex];

    if(groupIndex == 0)
    {
        gPhotonEmissionGuideMap2[int2(0, 0)] = 
        sharedValueTbl1[0] + 
        sharedValueTbl1[1] + 
        sharedValueTbl1[2] + 
        sharedValueTbl1[3];
    }
}