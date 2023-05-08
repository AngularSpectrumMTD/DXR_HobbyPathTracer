#include "Grid3D_Header.hlsli"

RWStructuredBuffer<PhotonInfo> gPhotonMapRead : register(u0);
RWStructuredBuffer<PhotonInfo> gPhotonMapWrite : register(u1);

[numthreads(GRID_SORT_THREAD_NUM, 1, 1)]
void copyPhotonBuffer(uint3 dtid : SV_DispatchThreadID)
{
    gPhotonMapWrite[dtid.x] = gPhotonMapRead[dtid.x]; //sorted
}