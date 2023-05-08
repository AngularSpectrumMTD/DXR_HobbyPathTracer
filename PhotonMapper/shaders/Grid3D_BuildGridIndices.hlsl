#include "Grid3D_Header.hlsli"

RWStructuredBuffer<uint2> gPhotonGridBufferRead : register(u0);
RWStructuredBuffer<uint2> gPhotonGridIdBufferWrite : register(u1);

[numthreads(GRID_SORT_THREAD_NUM, 1, 1)]
void buildGridIndices(uint3 dtid : SV_DispatchThreadID)
{
    const unsigned int photonID = dtid.x;
    
    //prev ID
    uint photonID_prev = (photonID == 0) ? gGridParam.numPhotons : photonID;
    photonID_prev--;

    //next ID
    uint photonID_next = photonID + 1;
    if (photonID_next == (uint) gGridParam.numPhotons)
    {
        photonID_next = 0;
    }
    
    uint cell = GetGridHash(gPhotonGridBufferRead[photonID]);
    uint cell_prev = GetGridHash(gPhotonGridBufferRead[photonID_prev]);
    uint cell_next = GetGridHash(gPhotonGridBufferRead[photonID_next]);
    
    if (cell != cell_prev)
    {
        gPhotonGridIdBufferWrite[cell].x = photonID;
    }
    
    if (cell != cell_next)
    {
        gPhotonGridIdBufferWrite[cell].y = photonID + 1;
    }
}