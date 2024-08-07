#include "opticalFunction.hlsli"

#define SPP 1

void clearEmissionGuideMap(int3 launchIndex)
{
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap0.GetDimensions(size.x, size.y);
        if((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap0[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap1.GetDimensions(size.x, size.y);
        if((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap1[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap2.GetDimensions(size.x, size.y);
        if((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap2[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap3.GetDimensions(size.x, size.y);
        if((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap3[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap4.GetDimensions(size.x, size.y);
        if((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap4[launchIndex.xy] = 0;
        }
    }
    {
        float2 size = 0.xx;
        gPhotonEmissionGuideMap5.GetDimensions(size.x, size.y);
        if((launchIndex.x < size.x) && (launchIndex.y < size.y) && (launchIndex.z == 0))
        {
            gPhotonEmissionGuideMap5[launchIndex.xy] = 0;
        }
    }
}

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
    gPrevIDBuffer[launchIndex.xy] = 0.xx;

    //clear DI / GI / caustics buffer
    setDI(0.xxx);
    setGI(0.xxx);
    setCaustics(0.xxx);
    
    gPrevIDBuffer[launchIndex.xy] = 0.xx;

    //clear photon random counter map
    float2 counterMapSize = 0.xx;
    gPhotonRandomCounterMap.GetDimensions(counterMapSize.x, counterMapSize.y);
    if((launchIndex.x < counterMapSize.x) && (launchIndex.y < counterMapSize.y) && (launchIndex.z == 0))
    {
        gPhotonRandomCounterMap[launchIndex.xy] = 0;
    }
    
    //clear reservoir buffer
    DIReservoir dummyReservoir;
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    dummyReservoir.initialize();
    gDIReservoirBuffer[serialIndex] = dummyReservoir;
    gDISpatialReservoirBufferSrc[serialIndex] = dummyReservoir;

    //random
    float LightSeed = getLightRandomSeed();
    uint seed = (launchIndex.x + (DispatchRaysDimensions().x + 100000 * (uint) LightSeed.x) * launchIndex.y);
    randGenState = uint(pcgHash(seed));
    rseed = LightSeed.x;

    const float energyBoost = 1.0f;

    //for(int i = 0; i < SPP ; i++)
    {
        //float2 IJ = int2(i / (SPP / 2.f), i % (SPP / 2.f)) - 0.5.xx;
        float2 IJ = int2(0 / (1 / 2.f), 0 % (1 / 2.f)) - 0.5.xx;

        float2 d = (launchIndex.xy + 0.5) / dims.xy * 2.0 - 1.0 + IJ / dims.xy;
        RayDesc nextRay;
        nextRay.Origin = mul(gSceneParam.mtxViewInv, float4(0, 0, 0, 1)).xyz;

        float4 target = mul(gSceneParam.mtxProjInv, float4(d.x, -d.y, 1, 1));
        nextRay.Direction = normalize(mul(gSceneParam.mtxViewInv, float4(target.xyz, 0)).xyz);

        nextRay.TMin = 0;
        nextRay.TMax = 100000;

        Payload payload;
        payload.throughput = energyBoost * float3(1, 1, 1);
        payload.recursive = 0;
        payload.flags = 0;//empty

        RAY_FLAG flags = RAY_FLAG_NONE;

        uint rayMask = 0xFF;

        TraceRay(gBVH, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
    }
}

float getPhotonEmissionGuideMap(int2 pos, int mip)
{
    switch (mip)
    {
        case 0 :
            return gPhotonEmissionGuideMap0[pos];
            break;
        case 1:
            return gPhotonEmissionGuideMap1[pos];
            break;
        case 2:
            return gPhotonEmissionGuideMap2[pos];
            break;
        case 3:
            return gPhotonEmissionGuideMap3[pos];
            break;
        case 4:
            return gPhotonEmissionGuideMap4[pos];
            break;
        case 5:
            return gPhotonEmissionGuideMap5[pos];
            break;
        case 6:
            return gPhotonEmissionGuideMap6[pos];
            break;
        default:
            return gPhotonEmissionGuideMap0[pos];
            break;
    }
}

float2 emissionGuiding(inout float2 randomXY)
{
    float2 dims;
    gPhotonEmissionGuideMap0.GetDimensions(dims.x, dims.y);

    int2 pos = int2(0, 0);

    for(int i = PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL - 2; i >= 0; i--)
    {
        pos *= 2;

        float lt = getPhotonEmissionGuideMap(pos + int2(0, 0), i);
        float rt = getPhotonEmissionGuideMap(pos + int2(1, 0), i);
        float lb = getPhotonEmissionGuideMap(pos + int2(0, 1), i);
        float rb = getPhotonEmissionGuideMap(pos + int2(1, 1), i);

        float left = lt + lb;
        float right = rt + rb;
        float probLeft = left / (left + right);

        if((left == 0) && (right == 0))
        {
            return randomXY;
        }

        if(randomXY.x < probLeft)
        {
            randomXY.x /= probLeft;
            float probTop = lt / left;

            if(randomXY.y < probTop)
            {
                randomXY.y /= probTop;
            }
            else
            {
                pos.y++;
                randomXY.y = (randomXY.y - probTop) / (1 - probTop);
            }
        }
        else
        {
            pos.x++;
            randomXY.x = (randomXY.x - probLeft) / (1 - probLeft);
            float probTop = rt / right;

            if(randomXY.y < probTop)
            {
                randomXY.y /= probTop;
            }
            else
            {
                pos.y++;
                randomXY.y = (randomXY.y - probTop) / (1 - probTop);
            }
        }
    }

    return (pos + randomXY) / dims;
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

    float2 randomUV = 0.xx;
    float pdf = 0;
    sampleLightEmitDirAndPosition(emitDir, emitOrigin, randomUV,  pdf);

    float2 origRandomUV = randomUV;

    const float LAMBDA_NM = LAMBDA_VIO_NM + LAMBDA_STEP * (randGenState % LAMBDA_NUM);
    const float flutter = 0.1f;
    const float2 guidedUV = rand() < flutter ? randomUV : emissionGuiding(randomUV);

    sampleLightEmitDirAndPositionWithRandom(emitDir, emitOrigin, guidedUV);

    RayDesc nextRay;
    nextRay.Origin = emitOrigin;
    nextRay.Direction = emitDir;
    nextRay.TMin = 0;
    nextRay.TMax = 100000;

    PhotonPayload payload;
    payload.throughput = 1.xxx / pdf;//getBaseLightXYZ(LAMBDA_NM);
    payload.recursive = 0;
    payload.flags = 0;//empty
    payload.lambdaNM = LAMBDA_NM;
    payload.randomUV = origRandomUV;

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
    const float3 centerPos = gPositionBuffer[launchIndex.xy].xyz;

    //combine reservoirs
    if(isUseReservoirSpatialReuse() || (currDIReservoir.M < (MAX_TEMPORAL_REUSE_M / 2)))
    {
        for(int s = 0; s < gReSTIRParam.data.x; s++)
        {
            const float r = rand() * ((currDIReservoir.M > (MAX_TEMPORAL_REUSE_M / 4)) ? 1 : gReSTIRParam.data.x);
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
            const float3 nearPos = gPositionBuffer[nearIndex.xy].xyz;

            const bool isNearDepth = ((centerDepth * 0.95 < nearDepth) && (nearDepth < centerDepth * 1.05)) && (centerDepth > 0) && (nearDepth > 0);
            const bool isNearNormal = dot(centerNormal, nearNormal) > 0.9;
            const bool isSameInstance = (centerPrimitiveIndex == nearPrimitiveIndex);
            const bool isNearRoughness = (abs(centerRoughness - nearRoughness) < 0.05);
            const bool isNearAlbedoLuminance = (abs(centerAlbedoLuminance - nearAlbedoLuminance) < 0.05);
            const bool isNearPosition = (sqrt(dot(centerPos - nearPos, centerPos - nearPos)) < 0.3f);//30cm

            const bool isSimilar = isNearPosition && isNearNormal && isSameInstance && isNearRoughness && isNearAlbedoLuminance;//((nearDepth * 0.95 < centerDepth) && (centerDepth < nearDepth * 1.05));//5%
            if(!isSimilar || (length(nearNormal) < 0.01))
            {
                continue;
            }
            const float nearUpdateW = nearDIReservoir.W_sum;// * (spatDIReservoir.targetPDF / nearDIReservoir.targetPDF);
            combineDIReservoirs(spatDIReservoir, nearDIReservoir, nearUpdateW, rand2(nearIndex.xy));
        }
    }

    LightSample lightSample;
    rseed = spatDIReservoir.randomSeed;
    sampleLightWithID(scatterPosition, spatDIReservoir.lightID, lightSample);
    float3 biasedPosition = scatterPosition + 0.01f * lightSample.distance * normalize(lightSample.directionToLight);
    if(!isVisible(biasedPosition, lightSample))
    {
        recognizeAsShadowedReservoir(spatDIReservoir);
    }

    //The term "FGL" must be recalculated and the loading of MaterialParams must be prepared..... very cumbersome...

    if(spatDIReservoir.M > MAX_SPATIAL_REUSE_M)
    {
        float r = max(0, ((float)MAX_SPATIAL_REUSE_M / spatDIReservoir.M));
        spatDIReservoir.W_sum *= r;
        spatDIReservoir.M = MAX_SPATIAL_REUSE_M;
    }

    gDIReservoirBuffer[serialIndex] = spatDIReservoir;
}