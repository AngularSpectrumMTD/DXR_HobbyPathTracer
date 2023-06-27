#include "common.hlsli"

float2 equirecFetchUV(float3 dir)
{
    float2 uv = float2(atan2(dir.z , dir.x) / 2.0 / PI + 0.5, acos(dir.y) / PI);
    return uv;
}

[shader("miss")]
void miss(inout Payload payload) {
    depthPositionNormalStore(payload, gSceneParam.backgroundColor.rgb);
    float4 cubemap = gEquiRecEnvMap.SampleLevel(gSampler, equirecFetchUV(WorldRayDirection()), 0.0);

     if(payload.weight > 0)
    {
        payload.color = payload.weight + cubemap.rgb;
    }
    else
    {
        payload.color = cubemap.rgb;
    }
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
