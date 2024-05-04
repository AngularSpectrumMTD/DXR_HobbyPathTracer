#include "opticalFunction.hlsli"

#define SPP 1
#define REINHARD_L 1000
#define MAX_ACCUMULATION_RANGE 10000

float reinhard(float x, float L)
{
    return (x / (1 + x)) * (1 + x / L / L);
}

float3 reinhard3f(float3 v, float L)
{
    return float3(reinhard(v.x, L), reinhard(v.y, L), reinhard(v.z, L));
}

float ACESFilmicTonemapping(float x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate(x * (a * x + b) / (x * (c * x + d) + c));
}

float3 ACESFilmicTonemapping3f(float3 v)
{
    return float3(ACESFilmicTonemapping(v.x), ACESFilmicTonemapping(v.y), ACESFilmicTonemapping(v.z));
}

void applyTimeDivision(inout float3 current, uint2 ID)
{
    float2 dims = float2(DispatchRaysDimensions().xy);
    float currDepth = gDepthBuffer[ID];
    uint accCount = gAccumulationCountBuffer[ID];

    float2 velocity = gVelocityBuffer[ID] * 2.0 - 1.0;
    uint2 prevID = ID;//(ID / dims - velocity) * dims;
    float3 prev = gAccumulationBuffer[prevID].rgb;
    float prevDepth = gPrevDepthBuffer[prevID];
    float2 prevLuminanceMoment = gLuminanceMomentBufferSrc[prevID];

    float luminance = computeLuminance(current);
    float2 curremtLuminanceMoment = float2(luminance, luminance * luminance);
    
    if (isAccumulationApply())
    {
        accCount++;
    }
    else
    {
        accCount = 1;
    }
    gAccumulationCountBuffer[ID] = accCount;

    const float tmpAccmuRatio = 1.f / accCount;
    current = lerp(prev, current, tmpAccmuRatio);
    if (accCount < MAX_ACCUMULATION_RANGE)
    {
        curremtLuminanceMoment.x = lerp(prevLuminanceMoment.x, curremtLuminanceMoment.x, tmpAccmuRatio);
        curremtLuminanceMoment.y = lerp(prevLuminanceMoment.y, curremtLuminanceMoment.y, tmpAccmuRatio);
        gAccumulationBuffer[ID].rgb = current;
        gLuminanceMomentBufferDst[ID] = curremtLuminanceMoment;
    }
}

//
//DispatchRays By Screen Size2D
//
[shader("raygeneration")]
void rayGen() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    gDepthBuffer[launchIndex] = 0;
    float2 dims = float2(DispatchRaysDimensions().xy);

    //clear gbuffer
    gDiffuseAlbedoBuffer[launchIndex] = 0.xxxx;
    gDepthBuffer[launchIndex] = 0;
    gPositionBuffer[launchIndex] = 0.xxxx;
    gNormalBuffer[launchIndex] = 0.xxxx;
    gVelocityBuffer[launchIndex] = 0.xx;

    //clear DI / GI buffer
    gDIBuffer[launchIndex] = 0.xxxx;
    gGIBuffer[launchIndex] = 0.xxxx;

    float3 accumColor = 0.xxx;
    float3 accumDI = 0.xxx;
    float3 accumGI = 0.xxx;

    //random
    float LightSeed = getLightRandomSeed();
    uint seed = (launchIndex.x + (DispatchRaysDimensions().x + 100000 * (uint) LightSeed.x) * launchIndex.y);
    randGenState = uint(pcgHash(seed));
    rseed = LightSeed.x;

    const float energyBoost = 1.0f;

    for(int i = 0; i < SPP ; i++)
    {
        float2 IJ = int2(i / (SPP / 2.f), i % (SPP / 2.f)) - 0.5.xx;

        float2 d = (launchIndex.xy + 0.5) / dims.xy * 2.0 - 1.0 + IJ / dims.xy;
        RayDesc nextRay;
        nextRay.Origin = mul(gSceneParam.mtxViewInv, float4(0, 0, 0, 1)).xyz;

        float4 target = mul(gSceneParam.mtxProjInv, float4(d.x, -d.y, 1, 1));
        nextRay.Direction = normalize(mul(gSceneParam.mtxViewInv, float4(target.xyz, 0)).xyz);

        nextRay.TMin = 0;
        nextRay.TMax = 100000;

        Payload payload;
        payload.throughput = energyBoost * float3(1, 1, 1);
        payload.color = float3(0, 0, 0);
        payload.recursive = 0;
        payload.storeIndexXY = launchIndex;
        payload.flags = 0;//empty
        payload.eyeDir = nextRay.Direction;
        payload.DI = 0.xxx;
        payload.GI = 0.xxx;

        RAY_FLAG flags = RAY_FLAG_NONE;

        uint rayMask = 0xFF;

        TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);

        accumColor += payload.color;
        accumDI += payload.DI;
        accumGI += payload.GI;
    }
    float3 finalCol = max(0.xxx, accumColor / SPP);
    float3 finalDI = max(0.xxx, accumDI / SPP);
    float3 finalGI = max(0.xxx, accumGI / SPP);
    applyTimeDivision(finalCol, launchIndex);

    float luminance = computeLuminance(finalCol);

    //gOutput[launchIndex.xy] = float4(reinhard3f(finalCol, REINHARD_L), 1);
    gOutput[launchIndex.xy] = float4(finalCol * reinhard(luminance, REINHARD_L) / luminance, 1);//luminance based tone mapping
    //gOutput[launchIndex.xy] = float4(ACESFilmicTonemapping3f(finalCol), 1);
    //gOutput[launchIndex.xy] = float4(finalCol * ACESFilmicTonemapping(luminance) / luminance, 1);

    gDIBuffer[launchIndex.xy] = float4(finalDI, 0);
    gGIBuffer[launchIndex.xy] = float4(finalGI, 0);
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

    PhotonInfo photon;
    photon.throughput = float3(0,0,0);
    photon.position = float3(0,0,0);

    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    const int COLOR_ID = serialIndex % getLightLambdaNum();

    gPhotonMap[serialIndex] = photon;//initialize

    float3 emitOrigin = 0.xxx;
    float3 emitDir = 0.xxx;

    sampleLightEmitDirAndPosition(emitDir, emitOrigin);
    
    Reservoir reservoir;
    reservoir.initialize();

    const uint SAMPLE_NUM = LAMBDA_NUM / 8;
    float p_hat = 0;
    for (int i = 0; i < SAMPLE_NUM; i++)
    {
        float lmbd = LAMBDA_VIO_NM + LAMBDA_STEP * ((uint)(rand() * 100 * LAMBDA_NUM) % LAMBDA_NUM);
        p_hat = length(getBaseLightXYZ(lmbd));
        float updateW = p_hat * SAMPLE_NUM;
        updateReservoir(reservoir, lmbd, updateW, 0, 0, 1u, rand());
    }

    const float LAMBDA_NM = (LightSeed.x < 300) ? reservoir.Y : LAMBDA_VIO_NM + LAMBDA_STEP * (randGenState % LAMBDA_NUM);

    RayDesc nextRay;
    nextRay.Origin = emitOrigin;
    nextRay.Direction = emitDir;
    nextRay.TMin = 0;
    nextRay.TMax = 100000;

    PhotonPayload payload;
    payload.throughput = getBaseLightXYZ(LAMBDA_NM);
    payload.recursive = 0;
    payload.storeIndex = serialIndex;
    payload.stored = 0;//empty
    payload.lambdaNM = LAMBDA_NM;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK); //ignore your self!! lightsource model

    TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
}