#ifndef __GRID3D_HEADER_HLSLI__
#define __GRID3D_HEADER_HLSLI__

struct GridCB
{
    int numPhotons;
    float3 gridDimensions;
    float gridH;
    float photonExistRange;
};


struct PhotonInfo
{
    float3 throughput;
    float3 position;
    //float3 inDir;
};

#define GRID_SORT_THREAD_NUM 1024
#define UINT32_MAX 0xffffffff

ConstantBuffer<GridCB> gGridParam : register(b0);

bool isOutSideGridRange(float3 pos)
{
    return  (
    (pos.x < -gGridParam.photonExistRange / 2 || pos.x > gGridParam.photonExistRange / 2)//x
    ||(pos.y < -gGridParam.photonExistRange / 2 || pos.y > gGridParam.photonExistRange / 2)//y
    ||(pos.z < -gGridParam.photonExistRange / 2 || pos.z > gGridParam.photonExistRange / 2)//z
    )
    ;
}

uint3 ComputeGridCell(float3 pos, inout bool isOutside)
{
    isOutside = isOutSideGridRange(pos);
    if(isOutside)
    {
        return uint3(UINT32_MAX,UINT32_MAX,UINT32_MAX);
    }
    else
    {
        int3 ijk;
        ijk.x = gGridParam.gridDimensions.x * pos.x / (gGridParam.photonExistRange) + 0.5 * gGridParam.gridDimensions.x;
        ijk.y = gGridParam.gridDimensions.y * pos.y / (gGridParam.photonExistRange) + 0.5 * gGridParam.gridDimensions.y;
        ijk.z = gGridParam.gridDimensions.z * pos.z / (gGridParam.photonExistRange) + 0.5 * gGridParam.gridDimensions.z;
        return ijk;
    }
}

float3 CellCenterPos(uint3 ijk)
{
    float3 IJK = (float3)ijk - (gGridParam.gridH / 2).xxx - 1.xxx;
    float dx = gGridParam.photonExistRange / gGridParam.gridDimensions.x, 
    dy = gGridParam.photonExistRange / gGridParam.gridDimensions.y, 
    dz = gGridParam.photonExistRange / gGridParam.gridDimensions.z;
    return float3(dx, dy, dz) * (IJK + 0.5.xxx);
}

float DistanceBetweenCell(uint3 ijk, float3 pos)
{
    float3 cellCenter = CellCenterPos(ijk);
    return length(cellCenter - pos);
}

uint GridHash(uint3 ijk)
{
    return ijk.x + (ijk.y * gGridParam.gridDimensions.x) + (ijk.z * gGridParam.gridDimensions.x * gGridParam.gridDimensions.y);
}

uint2 MakeHashPair(uint3 ijk, uint id)
{
    return uint2(GridHash(ijk), id);
}

uint GetGridHash(uint2 hashPair)
{
    return hashPair.x;
}

uint GetGridValue(uint2 hashPair)
{
    return hashPair.y;
}

#endif//__GRID3D_HEADER_HLSLI__