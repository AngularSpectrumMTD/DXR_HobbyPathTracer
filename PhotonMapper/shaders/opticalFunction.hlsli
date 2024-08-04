#ifndef __OPTICALFUNCTION_HLSLI__
#define __OPTICALFUNCTION_HLSLI__

#include "common.hlsli"

#define Z_AXIS float3(0, 0, 1)

struct MaterialParams
{
    float4 albedo;
    float metallic;
    float roughness;
    float specular;
    float transRatio;
    float4 transColor;
    float4 emission;
};

bool isPhotonStoreRequired(in MaterialParams params)
{
    return (rand() < params.roughness) && (params.transRatio == 0);
}

void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
{
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    tangent = normalize(cross(up, normal));
    bitangent = cross(normal, tangent);
}

uint getRandomLightID()
{
    return min(max(0, (uint) (rand() * (getLightNum()) + 0.5)), getLightNum() - 1);
}

#include "reservoir.hlsli"
#include "geometryIntersection.hlsli"
#include "samplingBSDF.hlsli"
#include "samplingLight.hlsli"
#include "shading.hlsli"
#include "spectralRenderingHelper.hlsli"

#endif//__OPTICALFUNCTION_HLSLI__