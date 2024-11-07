#include "common.hlsli"
#include "opticalFunction.hlsli"

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

    if(isSSSRay(payload))
    {
        payload.T = -1;
        return;
    }

    float3 hitLe = 0.xxx;
    float3 hitNormal = 0.xxx;
    float3 hitPosition = 0.xxx;
    if (!isIndirectOnly() && isCompletelyMissRay(payload) && intersectAllLightWithCurrentRay(hitLe, hitPosition, hitNormal))
    {
        setDI(hitLe);
        payload.compressedThroughput = 0u;
        const bool isAnaliticalLightHitted = (length(hitLe) > 0);
        const float3 writeColor = isAnaliticalLightHitted ? hitLe : 0.xxx;
        const float3 writeNormal = isAnaliticalLightHitted ? hitNormal : 0.xxx;
        const float3 writePosition = isAnaliticalLightHitted ? hitPosition : 0.xxx;
        MaterialParams material = (MaterialParams)0;
        material.albedo = float4(writeColor, 0);
        storeGBuffer(payload, writePosition, writeColor, writeNormal, 0, 0, 0, material);
        return;
    }

    const bool isNEE_Prev_Executable = payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
    const bool isHitLightingRequired = isUseNEE() ? !isNEE_Prev_Executable : (isIndirectOnly() ? isIndirectRay(payload) : true);

    if (isHitLightingRequired)
    {
        float3 element = U32toF32x3(payload.compressedThroughput) * directionalLightingOnMissShader(payload);

        if(isDirectRay(payload) || isCompletelyMissRay(payload))
        {
            addDI(element);
        }
        if(isIndirectRay(payload))
        {
            addGI(element);
        }
    }

    MaterialParams material = (MaterialParams)0;
    storeGBuffer(payload, 0.xxx, 0.xxx, 0.xxx, -1, -1, -1, material);

    if(isUseIBL())
    {
        float4 cubemap = gEquiRecEnvMap.SampleLevel(gSampler, EquirecFetchUV(WorldRayDirection()), 0.0);
        float3 element = U32toF32x3(payload.compressedThroughput) * cubemap.rgb;
        if(isDirectRay(payload) || isCompletelyMissRay(payload))
        {
            addDI(element);
        }
        if(isIndirectRay(payload))
        {
            addGI(element);
        }
    }
    
    payload.compressedThroughput = 0u;
}

[shader("miss")]
void photonMiss(inout PhotonPayload payload)
{
    payload.compressedThroughput = 0u;
}

[shader("miss")]
void dummyMiss(inout Payload payload)
{
    //no op
}
