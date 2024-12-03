#include "Grid3D_Header.hlsli"

RWStructuredBuffer<uint2> gPhotonGridIdBufferWrite : register(u0);

[numthreads(GRID_SORT_THREAD_NUM, 1, 1)]
void clearGridIndices(uint3 dtid : SV_DispatchThreadID)
{
    gPhotonGridIdBufferWrite[dtid.x] = uint2(UINT32_MAX, UINT32_MAX);
}