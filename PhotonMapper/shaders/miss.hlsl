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
    float hitT = -1;
    float3 hitNormal = 0.xxx;
    if (!isIndirectOnly() && intersectAllLightWithCurrentRay(hittedEmission, hitT, hitNormal))
    {
        payload.DI = hittedEmission;
        payload.throughput = 0.xxx;
    }

    const bool isNEE_Prev_Executable = payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
    const bool isHitLightingRequired = isUseNEE() ? !isNEE_Prev_Executable : (isIndirectOnly() ? isIndirectRay(payload) : true);

    if (isHitLightingRequired)
    {
        float3 element = payload.throughput * directionalLightingOnMissShader(payload);

        if(isDirectRay(payload) || isCompletelyMissRay(payload))
        {
            payload.DI += element;
        }
        if(isIndirectRay(payload))
        {
            payload.GI += element;
        }
    }

#ifdef ENABLE_IBL
    float4 cubemap = gEquiRecEnvMap.SampleLevel(gSampler, EquirecFetchUV(WorldRayDirection()), 0.0);
    float3 element = payload.throughput * cubemap.rgb;
    if(isDirectRay(payload) || isCompletelyMissRay(payload))
    {
        payload.DI += element;
    }
    if(isIndirectRay(payload))
    {
        payload.GI += element;
    }
#endif

    const bool isHitLight = (hitT > 0);
    float3 storePosition = isHitLight ? (WorldRayOrigin() + hitT * WorldRayDirection()) : 0.xxx;
    float3 storeAlbedo = isHitLight ? hittedEmission : cubemap.rgb;
    float3 storeNormal = isHitLight ? hitNormal : 0.xxx;
    storeGBuffer(payload, storePosition, storeAlbedo, storeNormal, -1, -1, -1);

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
