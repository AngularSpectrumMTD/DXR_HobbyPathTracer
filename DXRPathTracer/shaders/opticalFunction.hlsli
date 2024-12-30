#ifndef __OPTICALFUNCTION_HLSLI__
#define __OPTICALFUNCTION_HLSLI__

#include "common.hlsli"

#define Z_AXIS float3(0, 0, 1)

void primarySurfaceHasHighPossibilityCausticsGenerate(in MaterialParams params, inout PhotonPayload payload)
{
    if((payload.recursive == 1) && ((params.transRatio > 0) || (params.metallic > 0.5)))
    {
        payload.flags |= PHOTON_PAYLOAD_BIT_MASK_IS_PRIMARY_SURFACE_HAS_HIGH_POSSIBILITY_GENERATE_CAUSTICS;
    }
}

bool isPrimarySurfaceHasHighPossibilityCausticsGenerate(in PhotonPayload payload)
{
    return (payload.flags & PHOTON_PAYLOAD_BIT_MASK_IS_PRIMARY_SURFACE_HAS_HIGH_POSSIBILITY_GENERATE_CAUSTICS);
}

bool isPhotonStoreRequired(in MaterialParams params, inout PhotonPayload payload)
{
    return (rand(payload.randomSeed) < params.roughness) && (params.transRatio == 0);
}

// void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
// {
//     float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
//     tangent = normalize(cross(up, normal));
//     bitangent = cross(normal, tangent);
// }

float copysignf(float x, float y)
{
    uint ux = asuint(x);
    uint uy = asuint(y);
	ux &= 0x7fffffff;
	ux |= uy & 0x80000000;
	return asfloat(ux);
}

//Paper : "Building an Orthonormal Basis, Revisited"
void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
{
  float sign = copysignf(1.0f, normal.z);
  const float A = -1.0f / (sign + normal.z);
  const float B = normal.x * normal.y * A;
  tangent = float3(1.0f + sign * normal.x * normal.x * A, sign * B, -sign * normal.x);
  bitangent = float3(B, sign + normal.y * normal.y * A, -normal.y);
}

uint getRandomLightID(inout uint randomSeed)
{
    return min(max(0, (uint) (rand(randomSeed) * (getLightNum()) + 0.5)), getLightNum() - 1);
}

#include "reservoir.hlsli"
#include "geometryIntersection.hlsli"
#include "samplingBSDF.hlsli"
#include "samplingLight.hlsli"
#include "shading.hlsli"
#include "spectralRenderingHelper.hlsli"

#endif//__OPTICALFUNCTION_HLSLI__