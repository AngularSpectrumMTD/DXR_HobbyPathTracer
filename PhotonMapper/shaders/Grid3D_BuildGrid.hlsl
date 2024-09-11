#include "Grid3D_Header.hlsli"

RWStructuredBuffer<PhotonInfo> gPhotonMapRead : register(u0);
RWStructuredBuffer<uint2> gPhotonGridBufferWrite : register(u1);

//compute cell has photon, and bind
[numthreads(GRID_SORT_THREAD_NUM, 1, 1)]
void buildGrid( uint3 dtid : SV_DispatchThreadID )
{
    const unsigned int photonID = dtid.x;
    
    PhotonInfo photon = gPhotonMapRead[photonID];
    float3 photonPosition = photon.position;
    bool isZero = (photon.compressedThroughput == 0);
    bool isOutside = false;
    float3 gridIJK = ComputeGridCell(photonPosition, isOutside);
    
    if(isOutside || isZero)
    {
        gPhotonGridBufferWrite[photonID] = uint2(UINT32_MAX, photonID);
    }
    else
    {
        gPhotonGridBufferWrite[photonID] = MakeHashPair((uint3)gridIJK, photonID);
    }
}