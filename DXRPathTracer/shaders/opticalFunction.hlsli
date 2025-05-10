#ifndef __OPTICALFUNCTION_HLSLI__
#define __OPTICALFUNCTION_HLSLI__

#include "common.hlsli"

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
    return (rand(payload.randomSeed) < params.roughness) && !isTransparentMaterial(params);
}

// void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
// {
//     float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
//     tangent = normalize(cross(up, normal));
//     bitangent = cross(normal, tangent);
// }

#include "reservoir.hlsli"
#include "geometryIntersection.hlsli"
#include "samplingBSDF.hlsli"
#include "samplingLight.hlsli"
#include "shading.hlsli"
#include "spectralRenderingHelper.hlsli"

#endif//__OPTICALFUNCTION_HLSLI__