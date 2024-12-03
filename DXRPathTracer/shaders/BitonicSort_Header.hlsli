#ifndef __BITONICSORT_HEADER_HLSLI__
#define __BITONICSORT_HEADER_HLSLI__

#define BITONIC_BLOCK_SIZE 1024
#define TRANSPOSE_BLOCK_SIZE 16

struct BitonicSortCB
{
    uint level;
    uint levelMask;
    uint width;
    uint height;
};

ConstantBuffer<BitonicSortCB> gBitonicParam : register(b0);

bool Compare(uint2 left, uint2 right)
{
    return (left.x == right.x) ? (left.y <= right.y) : (left.x <= right.x);
}

#endif//__BITONICSORT_HEADER_HLSLI__