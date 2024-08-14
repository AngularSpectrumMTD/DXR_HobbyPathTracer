#ifndef __COMPRESSION_UTILITY_HLSLI__
#define __COMPRESSION_UTILITY_HLSLI__

uint convertF32x3toU32_R11G11B10(in float3 f3_rgb)
{
    return ((f32tof16(f3_rgb.x) << 17) & 0xFFE00000) | ((f32tof16(f3_rgb.y) << 6) & 0x001FFC00) | ((f32tof16(f3_rgb.z) >> 5) & 0x000003FF);
}

float3 convertU32toF32x3_R11G11B10(in uint u_rgb)
{
    return float3(f16tof32((u_rgb >> 17) & 0x7FF0), f16tof32((u_rgb >> 6) & 0x7FF0), f16tof32((u_rgb << 5) & 0x7FE0));
}

#endif//__COMPRESSION_UTILITY_HLSLI__