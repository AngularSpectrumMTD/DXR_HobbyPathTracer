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

    float3 hitLe = 0.xxx;
    float3 hitNormal = 0.xxx;
    float3 hitPosition = 0.xxx;
    if (!isIndirectOnly() && isCompletelyMissRay(payload) && intersectAllLightWithCurrentRay(hitLe, hitPosition, hitNormal))
    {
        payload.DI = hitLe;
        payload.throughput = 0.xxx;
        const bool isAnaliticalLightHitted = (length(hitLe) > 0);
        const float3 writeColor = isAnaliticalLightHitted ? hitLe : 0.xxx;
        const float3 writeNormal = isAnaliticalLightHitted ? hitNormal : 0.xxx;
        const float3 writePosition = isAnaliticalLightHitted ? hitPosition : 0.xxx;
        storeGBuffer(payload, writePosition, writeColor, writeNormal, 0, 0, 0);
        return;
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

    storeGBuffer(payload, 0.xxx, 0.xxx, 0.xxx, -1, -1, -1);

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
