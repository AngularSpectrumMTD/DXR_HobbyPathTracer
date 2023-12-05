#include "opticalFunction.hlsli"

#define SPP 1
#define REINHARD_L 1000

float reinhard(float x, float L)
{
    return (x / (1 + x)) * (1 + x / L / L);
}

float3 reinhard3f(float3 v, float L)
{
    return float3(reinhard(v.x, L), reinhard(v.y, L), reinhard(v.z, L));
}

void applyTimeDivision(inout float3 current, uint2 ID)
{
    float tmpAccmuRatio = getTempAccumuRatio();
    float3 prev = gOutput1[ID].rgb;
    
    float currentDepth = gDepthBuffer[ID];
    float prevDepth = gPrevDepthBuffer[ID];

    bool isNearColor = false;//abs(dot(normalize(prev), normalize(current))) > 0.95;
    bool isNearDepth = abs(currentDepth - prevDepth) < 0.0001;
    bool isAccept = (isNearColor ? true : isNearDepth) && (currentDepth != 0);

    float2 prevLuminanceMoment = gLuminanceMomentBufferSrc[ID];
    float luminance = luminanceFromRGB(current);
    float2 curremtLuminanceMoment = float2(luminance, luminance * luminance);
    
    if (isAccept)
    {
        current = lerp(prev, current, tmpAccmuRatio);
        curremtLuminanceMoment.x = lerp(prevLuminanceMoment.x, curremtLuminanceMoment.x, tmpAccmuRatio);
        curremtLuminanceMoment.y = lerp(prevLuminanceMoment.y, curremtLuminanceMoment.y, tmpAccmuRatio);
    }

    gOutput1[ID].rgb = current;
    gLuminanceMomentBufferDst[ID] = curremtLuminanceMoment;
}

//
//DispatchRays By Screen Size2D
//
[shader("raygeneration")]
void rayGen() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    gDepthBuffer[launchIndex] = 0;
    float2 dims = float2(DispatchRaysDimensions().xy);

    float3 accumColor = 0.xxx;

    //random
    float LightSeed = getLightRandomSeed();
    uint seed = (launchIndex.x + (DispatchRaysDimensions().x + 10 * (uint) LightSeed.x) * launchIndex.y);
    randGenState = uint(pcgHash(seed));
    rseed = LightSeed.x;

    const float energyBoost = 3.0f;

    for(int i = 0; i < SPP ; i++)
    {
        float2 IJ = int2(i / (SPP / 2.f), i % (SPP / 2.f)) - 0.5.xx;

        float2 d = (launchIndex.xy + 0.5) / dims.xy * 2.0 - 1.0 + IJ / dims.xy;
        RayDesc rayDesc;
        rayDesc.Origin = mul(gSceneParam.mtxViewInv, float4(0, 0, 0, 1)).xyz;

        float4 target = mul(gSceneParam.mtxProjInv, float4(d.x, -d.y, 1, 1));
        rayDesc.Direction = normalize(mul(gSceneParam.mtxViewInv, float4(target.xyz, 0)).xyz);

        rayDesc.TMin = 0;
        rayDesc.TMax = 100000;

        Payload payload;
        payload.energy = energyBoost * float3(1, 1, 1);
        payload.color = float3(0, 0, 0);
        payload.recursive = 0;
        payload.storeIndexXY = launchIndex;
        payload.stored = 0;//empty
        payload.eyeDir = rayDesc.Direction;

        RAY_FLAG flags = RAY_FLAG_NONE;

        uint rayMask = 0xFF;

        TraceRay(
            gRtScene, 
            flags,
            rayMask,
            0, // ray index
            1, // MultiplierForGeometryContrib
            0, // miss index
            rayDesc,
            payload);

        accumColor += payload.color;
    }
    float3 finalCol = accumColor / SPP;
    //float3 finalCol = reinhard3f((accumColor + accumPhotonColor) / SPP, REINHARD_L);
    applyTimeDivision(finalCol, launchIndex);
    gOutput[launchIndex.xy] = float4(finalCol, 1);
}

//
//DispatchRays By Photon Size2D
//
[shader("raygeneration")]
void photonEmitting()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    
    //random
    float LightSeed = getLightRandomSeed();
    uint seed = (launchIndex.x + (DispatchRaysDimensions().x + 100000 * (uint)LightSeed.x) * launchIndex.y);
    randGenState = uint(pcgHash(seed));
    
    float3 spotLightPosition = gSceneParam.spotLightPosition.xyz;
    float3 lightDir = gSceneParam.spotLightDirection.xyz;

    PhotonInfo photon;
    photon.throughput = float3(0,0,0);
    photon.position = float3(0,0,0);
    photon.inDir = float3(0,0,0);

    int serialIndex = SerialRaysIndex(launchIndex, dispatchDimensions);
    const int COLOR_ID = serialIndex % getLightLambdaNum();

    gPhotonMap[serialIndex] = photon;//initialize

    float coneAngle = acos(dot(lightDir, normalize((spotLightPosition)))) * 2.0 * getLightRange();
    float randSeed = 0.5 * (randGenState * rnd01Converter + LightSeed * rnd01Converter);
    float3 photonEmitDir = getConeSample(randSeed, lightDir, coneAngle);
    
    float LAMBDA_NM = LAMBDA_VIO_NM + LAMBDA_STEP * (randGenState % LAMBDA_NUM);

    RayDesc rayDesc;
    rayDesc.Origin = spotLightPosition;
    rayDesc.Direction = photonEmitDir;
    rayDesc.TMin = 0;
    rayDesc.TMax = 100000;

    PhotonPayload payload;
    float emitIntensity = length(gSceneParam.lightColor.xyz);
    payload.throughput = emitIntensity * getBaseLightXYZ(LAMBDA_NM);
    payload.recursive = 0;
    payload.storeIndex = serialIndex;
    payload.stored = 0;//empty
    payload.lambdaNM = LAMBDA_NM;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK); //ignore your self!! lightsource model

    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        payload);
}