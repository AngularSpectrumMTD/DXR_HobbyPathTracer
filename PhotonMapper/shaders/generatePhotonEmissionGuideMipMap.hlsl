RWTexture2D<float> gPhotonEmissionGuideMap0 : register(u0);
RWTexture2D<float> gPhotonEmissionGuideMap1 : register(u1);
RWTexture2D<float> gPhotonEmissionGuideMap2 : register(u2);
RWTexture2D<float> gPhotonEmissionGuideMap3 : register(u3);

groupshared float sharedValueTbl0[8 * 8];
groupshared float sharedValueTbl1[8 * 8];

[numthreads(8, 8, 1)]
void generatePhotonEmissionGuideMipMap(int groupIndex : SV_GroupIndex, int3 groupId : SV_GroupID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    sharedValueTbl0[groupIndex] = gPhotonEmissionGuideMap0.Load(dispatchThreadID.xy);
    AllMemoryBarrierWithGroupSync();
    if(groupIndex >= 16)
    {
        return;
    }

    int y = groupIndex / 4;
    int x = groupIndex - 4 * y;
    int shared_y = y * 2;
    int shared_x = x * 2;

    sharedValueTbl1[groupIndex] = 
    sharedValueTbl0[(shared_x + 0) + 8 * (shared_y + 0)] + 
    sharedValueTbl0[(shared_x + 1) + 8 * (shared_y + 0)] + 
    sharedValueTbl0[(shared_x + 0) + 8 * (shared_y + 1)] + 
    sharedValueTbl0[(shared_x + 1) + 8 * (shared_y + 1)];

    gPhotonEmissionGuideMap1[groupId.xy * 4 + int2(x, y)] = sharedValueTbl1[groupIndex];

    if(groupIndex >= 4)
    {
        return;
    }

    y = groupIndex / 2;
    x = groupIndex - 2 * y;
    shared_y = y * 2;
    shared_x = x * 2;

    sharedValueTbl0[groupIndex] = 
    sharedValueTbl1[(shared_x + 0) + 4 * (shared_y + 0)] + 
    sharedValueTbl1[(shared_x + 1) + 4 * (shared_y + 0)] + 
    sharedValueTbl1[(shared_x + 0) + 4 * (shared_y + 1)] + 
    sharedValueTbl1[(shared_x + 1) + 4 * (shared_y + 1)];

    gPhotonEmissionGuideMap2[groupId.xy * 2 + int2(x, y)] = sharedValueTbl0[groupIndex];

    if(groupIndex == 0)
    {
        gPhotonEmissionGuideMap3[groupId.xy] = 
        sharedValueTbl0[0] + 
        sharedValueTbl0[1] + 
        sharedValueTbl0[2] + 
        sharedValueTbl0[3];
    }
}