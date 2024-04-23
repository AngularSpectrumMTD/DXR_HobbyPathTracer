#include "common.hlsli"
#include "opticalFunction.hlsli"

#define ENABLE_IBL

float2 EquirecFetchUV(float3 dir)
{
    float2 uv = float2(atan2(dir.z , dir.x) / 2.0 / PI + 0.5, acos(dir.y) / PI);
    return uv;
}

[shader("miss")]
void miss(inout Payload payload) {
    if (isShadowRay(payload))
    {
        setVisibility(payload, true);
        return;
    }

    float3 hittedEmission = 0.xxx;
    if (!isIndirectOnly() && payload.recursive == 0 && intersectLightWithCurrentRay(hittedEmission))
    {
        payload.color = hittedEmission;
        payload.throughput = 0.xxx;
        return;
    }

    const bool isNEE_Prev_Executable = payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
    const bool isHitLightingRequired = isUseNEE() ? !isNEE_Prev_Executable : (isIndirectOnly() ? (payload.recursive > 1) : true);

    if (isHitLightingRequired)
    {
        payload.color += payload.throughput * directionalLightingOnMissShader(payload);
    }

    storeAlbedoDepthPositionNormal(payload, gSceneParam.backgroundColor.rgb, 0.xxx);

#ifdef ENABLE_IBL
    float4 cubemap = gEquiRecEnvMap.SampleLevel(gSampler, EquirecFetchUV(WorldRayDirection()), 0.0);
    payload.color += payload.throughput * cubemap.rgb;
#endif
    payload.throughput = 0.xxx;
}

[shader("miss")]
void photonMiss(inout PhotonPayload payload)
{
    payload.throughput = float3(0,0,0);
}

[shader("miss")]
void dummyMiss(inout Payload payload)
{
    //no op
}
