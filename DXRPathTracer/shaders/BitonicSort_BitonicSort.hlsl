#include "BitonicSort_Header.hlsli"

RWStructuredBuffer<uint2> gOutput : register(u0);

groupshared uint2 sharedData[BITONIC_BLOCK_SIZE];

[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void bitonicSort(uint3 dtid : SV_DispatchThreadID, uint gid : SV_GroupIndex)
{
    sharedData[gid] = gOutput[dtid.x];
    GroupMemoryBarrierWithGroupSync();

    //sort shared data
    for (uint j = gBitonicParam.level >> 1; j > 0; j >>= 1)
    {
        uint2 result = 
        //((sharedData[gid & ~j].x <= sharedData[gid | j].x) == (bool) (gBitonicParam.levelMask & dtid.x))
        (Compare(sharedData[gid & ~j], sharedData[gid | j]) == (bool) (gBitonicParam.levelMask & dtid.x))
         ? sharedData[gid ^ j] : sharedData[gid];
        GroupMemoryBarrierWithGroupSync();
        sharedData[gid] = result;
        GroupMemoryBarrierWithGroupSync();
    }
    
    gOutput[dtid.x] = sharedData[gid];
}