#include "opticalFunction.hlsli"

#define SPP 1

//
//DispatchRays By Screen Size2D
//
[shader("raygeneration")]
void rayGen() {
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    float2 dims = float2(DispatchRaysDimensions().xy);

    //clear gbuffer
    gDiffuseAlbedoBuffer[launchIndex.xy] = 0.xxxx;
    gNormalDepthBuffer[launchIndex.xy] = 0.xxxx;
    gPositionBuffer[launchIndex.xy] = 0.xxxx;
    gIDRoughnessBuffer[launchIndex.xy] = 0.xxxx;
    gVelocityBuffer[launchIndex.xy] = 0.xx;

    //clear DI / GI buffer
    gDIBuffer[launchIndex.xy] = 0.xxxx;
    gGIBuffer[launchIndex.xy] = 0.xxxx;

    //clear caustics buffer
    gCausticsBuffer[launchIndex.xy] = 0.xxxx;

    //clear reservoir buffer
    DIReservoir dummyReservoir;
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    dummyReservoir.initialize();
    gDIReservoirBuffer[serialIndex] = dummyReservoir;
    gDISpatialReservoirBufferSrc[serialIndex] = dummyReservoir;

    float3 accumCaustics = 0.xxx;
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
        payload.caustics = float3(0, 0, 0);
        payload.recursive = 0;
        payload.storeIndexXY = launchIndex.xy;
        payload.flags = 0;//empty
        payload.eyeDir = nextRay.Direction;
        payload.DI = 0.xxx;
        payload.GI = 0.xxx;

        RAY_FLAG flags = RAY_FLAG_NONE;

        uint rayMask = 0xFF;

        TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);

        accumCaustics += payload.caustics;
        accumDI += payload.DI;
        accumGI += payload.GI;
    }
    float3 finalDI = max(0.xxx, accumDI / SPP);
    float3 finalGI = max(0.xxx, accumGI / SPP);
    float3 finalCaustics = max(0.xxx, accumCaustics / SPP);

    gDIBuffer[launchIndex.xy] = float4(finalDI, 0);
    gGIBuffer[launchIndex.xy] = float4(finalGI, 0);
    gCausticsBuffer[launchIndex.xy] = float4(finalCaustics, 0);
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

    const float LAMBDA_NM = LAMBDA_VIO_NM + LAMBDA_STEP * (randGenState % LAMBDA_NUM);

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

    TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
}

#define SPATIAL_REUSE_NUM 4

float rand2(in int2 indexXY)//0-1
{
    rseed += 1.0;
    return frac(sin(dot(indexXY.xy, float2(12.9898, 78.233)) * (getLightRandomSeed() + 1) * 0.001 + rseed) * 43758.5453);
}

[shader("raygeneration")]
void spatialReuse() {
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    float2 dims = float2(DispatchRaysDimensions().xy);
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    //random
    float LightSeed = getLightRandomSeed();
    uint seed = (launchIndex.x + (DispatchRaysDimensions().x + 100000 * (uint) LightSeed.x) * launchIndex.y);
    randGenState = uint(pcgHash(seed));
    rseed = LightSeed.x;

    DIReservoir spatDIReservoir;
    spatDIReservoir.initialize();

    DIReservoir currDIReservoir = gDISpatialReservoirBufferSrc[serialIndex];
    const float currUpdateW = currDIReservoir.W_sum;
    combineDIReservoirs(spatDIReservoir, currDIReservoir, currUpdateW, rand());

    const float centerDepth = gNormalDepthBuffer[launchIndex.xy].w;
    const float3 centerNormal = gNormalDepthBuffer[launchIndex.xy].xyz;
    const float3 scatterPosition = gPositionBuffer[launchIndex.xy].xyz;
    const uint centerPrimitiveIndex = gIDRoughnessBuffer[launchIndex.xy].y;
    const float centerRoughness = gIDRoughnessBuffer[launchIndex.xy].z;
    const float centerAlbedoLuminance = gIDRoughnessBuffer[launchIndex.xy].w;

    //combine reservoirs
    if(isUseReservoirSpatialReuse())
    {
        for(int s = 0; s < gReSTIRParam.data.x; s++)
        {
            const float r = rand() * gReSTIRParam.data.x;
            const float v = rand();
            const float phi = 2.0f * PI * v;
            float2 sc = 0.xx;
            sincos(phi, sc.x, sc.y);
            int3 nearIndex = launchIndex + int3(r * sc, 0);
            
            if(!isWithinBounds(nearIndex.xy, dims))
            {
                continue;
            }

            const uint serialNearID = serialRaysIndex(nearIndex, dispatchDimensions);

            DIReservoir nearDIReservoir = gDISpatialReservoirBufferSrc[serialNearID];
            const float nearDepth = gNormalDepthBuffer[nearIndex.xy].w;
            const float3 nearNormal = gNormalDepthBuffer[nearIndex.xy].xyz;
            const uint nearPrimitiveIndex = gIDRoughnessBuffer[nearIndex.xy].y;
            const float nearRoughness = gIDRoughnessBuffer[nearIndex.xy].z;
            const float nearAlbedoLuminance = gIDRoughnessBuffer[nearIndex.xy].w;

            const bool isNearDepth = ((centerDepth * 0.95 < nearDepth) && (nearDepth < centerDepth * 1.05)) && (centerDepth > 0) && (nearDepth > 0);
            const bool isNearNormal = dot(centerNormal, nearNormal) > 0.9;
            const bool isSameInstance = (centerPrimitiveIndex == nearPrimitiveIndex);
            const bool isNearRoughness = (abs(centerRoughness - nearRoughness) < 0.05);
            const bool isNearAlbedoLuminance = (abs(centerAlbedoLuminance - nearAlbedoLuminance) < 0.05);

            const bool isSimilar = isNearDepth && isNearNormal && isSameInstance && isNearRoughness && isNearAlbedoLuminance;//((nearDepth * 0.95 < centerDepth) && (centerDepth < nearDepth * 1.05));//5%
            if(!isSimilar)
            {
                continue;
            }

            LightSample lightSample;
            sampleLightWithID(scatterPosition, nearDIReservoir.Y, lightSample);

            // if(!isVisible(scatterPosition, lightSample))
            // {
            //     continue;
            // }

            const float nearUpdateW = nearDIReservoir.W_sum;// * (spatDIReservoir.targetPDF / nearDIReservoir.targetPDF);
            combineDIReservoirs(spatDIReservoir, nearDIReservoir, nearUpdateW, rand2(nearIndex.xy));
        }
    }
    if(spatDIReservoir.M > MAX_SPATIAL_REUSE_M)
    {
        float r = max(0, ((float)MAX_SPATIAL_REUSE_M / spatDIReservoir.M));
        spatDIReservoir.W_sum *= r;
        spatDIReservoir.M = MAX_SPATIAL_REUSE_M;
    }
    gDIReservoirBuffer[serialIndex] = spatDIReservoir;
}