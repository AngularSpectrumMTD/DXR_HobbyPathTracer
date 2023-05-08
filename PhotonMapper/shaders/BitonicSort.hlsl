struct BitonicSortCB
{
    int inc;
    int dir;
};

#define BITONIC2_THREAD_NUM 64
ConstantBuffer<BitonicSortCB> gBitonicParam : register(b0);
RWStructuredBuffer<uint2> data : register(u0);

//Key Order
//Ascending Order
#define COMPARISON(a,b) ( a.x < b.x )
//Descending Order
//#define COMPARISON(a,b) ( a.x > b.x )

#define ORDER(a,b) { bool swap = reverse ^ COMPARISON(a,b); uint2 aPrev = a; uint2 bPrev = b; if (swap) { a = bPrev; b = aPrev; } }

[numthreads(BITONIC2_THREAD_NUM, 1, 1)]
void BitonicSort(uint threadid : SV_DispatchThreadID)
{
    int t = threadid;
    int low = t & (gBitonicParam.inc - 1);
    int i = (t << 1) - low;
    bool reverse = ((gBitonicParam.dir & i) == 0); // asc/desc order

    uint2 x0 = data[i];
    uint2 x1 = data[gBitonicParam.inc + i];

    ORDER(x0, x1)

    data[i] = x0;
    data[gBitonicParam.inc + i] = x1;
}