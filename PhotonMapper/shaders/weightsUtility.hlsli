#define THREAD_NUM 16
#define FILTER_FOOTPRINT 3
#define WAVELET_FILTERSIZE 3
#define WAVELET_FILTERELEMENTS WAVELET_FILTERSIZE * WAVELET_FILTERSIZE
#define WAVELET_CENTER (WAVELET_FILTERSIZE / 2) * WAVELET_FILTERSIZE + (WAVELET_FILTERSIZE / 2)

static const float cKernel[7] = { 0.006, 0.06, 0.24, 0.38, 0.24, 0.06, 0.006 };
static const float gaussKernel[3] = { 1 / 4.0f, 1 / 2.0f, 1 / 4.0f };
static const float waveletWeight[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static const int2 waveletOffset[] =
{
    int2(-1, -1), int2(0, -1), int2(+1, -1),
    int2(-1, 0), int2(0, 0), int2(+1, 0),
    int2(+1, +1), int2(0, +1), int2(+1, +1)
};

bool isInsideBounds(int2 ij, int2 size)
{
    return !(ij.x < 0 || ij.y < 0 || ij.x > size.x - 1 || ij.y > size.y - 1);
}

//https://research.nvidia.com/publication/2017-07_spatiotemporal-variance-guided-filtering-real-time-reconstruction-path-traced
float depthWeight(const float depth, const float neighborDepth, const float dzdx, const float dzdy, const int dx, const int dy)
{
    const float sig = 1.f;
    const float eps = 1e-5f;
    return saturate(exp(-abs(depth - neighborDepth + eps) / (sig * abs(dzdx * dx + dzdy * dy)) + eps));
}

float normalWeight(const float3 normal, const float3 neighborNormal)
{
    float normalInnerProduct = max(0, dot(normal, neighborNormal));
    if (normalInnerProduct == 0)
    {
        return 0;
    }
    
    float nip2 = normalInnerProduct * normalInnerProduct;
    float nip4 = nip2 * nip2;
    float nip8 = nip4 * nip4;
    float nip16 = nip8 * nip8;
    float nip32 = nip16 * nip16;
    float nip64 = nip32 * nip32;
    return nip64 * nip64;
}

float luminanceWeight(const float luminance, const float neighborLuminance, const float meanLocal)
{
    const float sig = 1.f;
    const float eps = 1e-5f;
    return exp(-abs(neighborLuminance - luminance) / (sig * meanLocal + eps));
}

float computeLuminance(const float3 linearRGB)
{
    return dot(float3(0.2126, 0.7152, 0.0722), linearRGB);
}