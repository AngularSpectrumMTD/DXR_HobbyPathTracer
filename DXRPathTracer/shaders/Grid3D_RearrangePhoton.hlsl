#include "Grid3D_Header.hlsli"

RWStructuredBuffer<PhotonInfo> gPhotonMapRead : register(u0);
RWStructuredBuffer<PhotonInfo> gPhotonMapWrite : register(u1);
RWStructuredBuffer<uint2> gPhotonGridBufferRead : register(u2);

[numthreads(GRID_SORT_THREAD_NUM, 1, 1)]
void rearrangePhoton(uint3 dtid : SV_DispatchThreadID)
{
    const unsigned int id = dtid.x;
    const unsigned int photonID = GetGridValue(gPhotonGridBufferRead[id]);
    gPhotonMapWrite[id] = gPhotonMapRead[photonID];//sorted
}