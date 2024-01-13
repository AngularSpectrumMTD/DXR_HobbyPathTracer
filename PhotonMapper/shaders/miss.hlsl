#include "common.hlsli"

float2 equirecFetchUV(float3 dir)
{
    float2 uv = float2(atan2(dir.z , dir.x) / 2.0 / PI + 0.5, acos(dir.y) / PI);
    return uv;
}

[shader("miss")]
void miss(inout Payload payload) {
    if (payload.isShadowRay == 1)
    {
        payload.isShadowMiss = 1;
        return;
    }

    depthPositionNormalStore(payload, gSceneParam.backgroundColor.rgb);
    float4 cubemap = gEquiRecEnvMap.SampleLevel(gSampler, equirecFetchUV(WorldRayDirection()), 0.0);
    payload.color += payload.energy * cubemap.rgb;
    payload.energy = 0.xxx;
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
