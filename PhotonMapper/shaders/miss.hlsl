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

    //payload.color += 0.1.xxx;
    //payload.color = 0;
    
    float3 directionalLightDir = normalize(gSceneParam.directionalLightDirection.xyz);
    const bool isRecursionDepthZero = (payload.recursive == 0);
    const bool isDirectionalLightReceived = isRecursionDepthZero ? true : (dot(directionalLightDir, WorldRayDirection()) < 0);
    float3 directionalLightEnergy = (isDirectionalLightReceived) ? gSceneParam.directionalLightColor.xyz : float3(0, 0, 0);
    float3 curEnergy = isDirectionalLightReceived ? (payload.energy + (isEnableDirectionalLight() ? directionalLightEnergy : float3(0, 0, 0))) : float3(0, 0, 0);
    payload.color += curEnergy * cubemap.rgb;
    payload.energy = 0.xxx;
    //payload.color = directionalLightEnergy;
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
