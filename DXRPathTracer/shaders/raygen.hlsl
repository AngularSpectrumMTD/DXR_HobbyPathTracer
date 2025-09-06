#include "common.hlsli"
#include "geometryIntersection.hlsli"
#include "samplingBSDF.hlsli"
#include "samplingLight.hlsli"
#include "spectralRenderingHelper.hlsli"

#define SPP 1
#define CLAMP_VALUE 1000

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

    //test
    gDebugTexture[DispatchRaysIndex().xy] = float4(0, 10, 0, 0);

    //clear gbuffer
    gNormalDepthBuffer[launchIndex.xy] = 0.xxxx;
    gPositionBuffer[launchIndex.xy] = 0.xxxx;
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
    {
        DIReservoir dummyReservoir;
        int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
        dummyReservoir.initialize();
        gDIReservoirBuffer[serialIndex] = dummyReservoir;
        gDIReservoirBufferSrc[serialIndex] = dummyReservoir;
    }
    {
        GIReservoir dummyReservoir;
        int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
        dummyReservoir.initialize();
        gGIReservoirBuffer[serialIndex] = dummyReservoir;
        gGIReservoirBufferSrc[serialIndex] = dummyReservoir;
    }

    //random
    uint randomSeed = 0;

    initializeRNG(launchIndex.xy, randomSeed);

    const float energyBoost = 1.0f;
    
    Payload payload;

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

        AABB photonAABB;
        photonAABB.maxElem = gGridParam.photonExistRange.xxx;
        photonAABB.minElem = -gGridParam.photonExistRange.xxx;
        float t = 0;
#ifdef PHOTON_AABB_DEBUG
        const bool intersectedPhotonAABB = isIntersectedOriginOrientedAABBvsRay(nextRay.Origin, nextRay.Direction, photonAABB, t);
#endif

        payload.throughputU32 = compressRGBasU32(energyBoost * float3(1, 1, 1));
        payload.recursive = 0;
        payload.flags = 0;//empty
        payload.T = 0;
        payload.f0 = 0u;
        payload.p0 = 1;
        payload.randomSeed = randomSeed;

        RAY_FLAG flags = RAY_FLAG_NONE;

        uint rayMask = 0xFF;

        TraceDefaultRay(flags, rayMask, nextRay, payload);

#ifdef PHOTON_AABB_DEBUG
        //photon AABB debug draw
        {
            if((payload.recursive == 0) && intersectedPhotonAABB)
            {
                addDI(float3(0, 1, 0));
            }
        }
#endif
    }

    //The influence of the initial BSDF on indirect element is evaluated at the end of ray generation shader
    float3 f1p1_from = getGI();
    f1p1_from = clamp(f1p1_from, 0.xxx, CLAMP_VALUE.xxx);

    GISample giSample = (GISample)0;
    //CompressedMaterialParams compressedMaterial = (CompressedMaterialParams)0;
    
    {
        GIReservoir giInitialReservoir = getGIReservoir();

        giSample.Lo2_U32 = compressRGBasU32(f1p1_from);
        giSample.pos2 = giInitialReservoir.giSample.pos2;
        giSample.nml2 = giInitialReservoir.giSample.nml2;
        //compressedMaterial = giInitialReservoir.compressedMaterial;
    }

    const float3 f0 = decompressU32asRGB(payload.f0);
    const float safeEPS = 0.001f;
    const float p0 = payload.p0 + safeEPS;
    float3 f0f1p1_from = f1p1_from * f0;
    f0f1p1_from = clamp(f0f1p1_from, 0.xxx, CLAMP_VALUE.xxx);
    const float p_hat = computeLuminance(f0f1p1_from);
    const float updateW = p_hat / p0;

    GIReservoir giReservoir;
    giReservoir.initialize();
    if((p0 > 0.001) && (updateW < 100))
    {
        giReservoir.W_sum = updateW;
        giReservoir.M = 1u;
        giReservoir.targetPDF = p_hat;
        giReservoir.targetPDF_3f_U32 = compressRGBasU32(f0f1p1_from);
        giReservoir.randomSeed = payload.bsdfRandomSeed;
        giReservoir.giSample = giSample;
        //giReservoir.compressedMaterial = compressedMaterial;
    }
    setGIReservoir(giReservoir);

    //set the value computed by a pure Monte-Calro strategy
    setGI(f0f1p1_from / p0);
    
    finalizeRNG(launchIndex.xy, payload.randomSeed);
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
    uint randomSeed = 0;
    initializeRNG(launchIndex.xy, randomSeed);

    PhotonInfo photon;
    photon.throughputU32 = 0u;
    photon.position = float3(0,0,0);

    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    const int COLOR_ID = serialIndex % getLightLambdaNum();

    gPhotonMap[serialIndex] = photon;//initialize

    float3 emitOrigin = 0.xxx;
    float3 emitDir = 0.xxx;

    float2 randomUV = 0.xx;
    float pdf = 0;
    sampleLightEmitDirAndPosition(emitDir, emitOrigin, randomUV,  pdf, randomSeed);

    const float2 origRandomUV = randomUV;

    const float LAMBDA_NM = LAMBDA_VIO_NM + LAMBDA_STEP * (randomSeed % LAMBDA_NUM);
    const float flutter = 0.2f;
    const float2 guidedUV = (rand(randomSeed) < flutter) ? origRandomUV : emissionGuiding(randomUV);

    sampleLightEmitDirAndPositionWithUV(emitDir, emitOrigin, guidedUV, randomSeed);

    RayDesc nextRay;
    nextRay.Origin = emitOrigin;
    nextRay.Direction = emitDir;
    nextRay.TMin = 0;
    nextRay.TMax = 100000;

    PhotonPayload payload;
#ifdef USE_SPECTRAL_RENDERED_CAUSTICS
    payload.throughputU32 = compressRGBasU32(getBaseLightXYZ(LAMBDA_NM) / pdf);
#else
    payload.throughputU32 = compressRGBasU32(1.xxx / pdf);
#endif
    payload.recursive = 0;
    payload.flags = 0;//empty
    payload.lambdaNM = LAMBDA_NM;
    payload.randomUV = origRandomUV;
    payload.randomSeed = randomSeed;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK); //ignore your self!! lightsource model

    TraceDefaultPhoton(flags, rayMask, nextRay, payload);

    finalizeRNG(launchIndex.xy, payload.randomSeed);
}

void performTemporalResampling(inout DIReservoir currDIReservoir, in DIReservoir prevDIReservoir, inout uint randomState)
{
    //Limitting
    if(prevDIReservoir.M > MAX_REUSE_M_DI)
    {
        float r = max(0, ((float)MAX_REUSE_M_DI / prevDIReservoir.M));
        prevDIReservoir.W_sum *= r;
        prevDIReservoir.M = MAX_REUSE_M_DI;
    }

    DIReservoir tempDIReservoir;
    tempDIReservoir.initialize();
    //combine reservoirs
    {
        const float currUpdateW = currDIReservoir.W_sum;
        combineDIReservoirs(tempDIReservoir, currDIReservoir, currUpdateW, rand(randomState));
        const float prevUpdateW = prevDIReservoir.W_sum;// * (prevDIReservoir.targetPDF / currDIReservoir.targetPDF);
        combineDIReservoirs(tempDIReservoir, prevDIReservoir, prevUpdateW, rand(randomState));
    }
    currDIReservoir = tempDIReservoir;
}

void performTemporalResampling(inout GIReservoir currGIReservoir, in GIReservoir prevGIReservoir, inout uint randomState)
{
    //Limitting
    if(prevGIReservoir.M > MAX_REUSE_M_GI)
    {
        float r = max(0, ((float)MAX_REUSE_M_GI / prevGIReservoir.M));
        prevGIReservoir.W_sum *= r;
        prevGIReservoir.M = MAX_REUSE_M_GI;
    }

    GIReservoir tempGIReservoir;
    tempGIReservoir.initialize();
    //combine reservoirs
    {
        const float currUpdateW = currGIReservoir.W_sum;
        combineGIReservoirs(tempGIReservoir, currGIReservoir, currUpdateW, rand(randomState));
        const float prevUpdateW = prevGIReservoir.W_sum;// * (prevDIReservoir.targetPDF / currDIReservoir.targetPDF);
        combineGIReservoirs(tempGIReservoir, prevGIReservoir, prevUpdateW, rand(randomState));
    }
    currGIReservoir = tempGIReservoir;
}

[shader("raygeneration")]
void temporalReuse()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    uint2 dims = DispatchRaysDimensions().xy;
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);
    
    //random
    uint randomSeed = 0;
    initializeRNG(launchIndex.xy, randomSeed);

    uint2 currID = launchIndex.xy;

    if(isUseNEE() && isUseStreamingRIS())
    {
        float currDepth = gNormalDepthBuffer[currID].w;
        float3 currNormal = gNormalDepthBuffer[currID].xyz;
        float3 currObjectWorldPos = gPositionBuffer[currID].xyz;

        int2 prevID = gPrevIDBuffer[currID];
        const uint serialCurrID = currID.y * dims.x + currID.x;
        const uint serialPrevID = clamp(prevID.y * dims.x + prevID.x, 0, dims.x * dims.y - 1);
         float prevDepth = gPrevNormalDepthBuffer[prevID].w;
        float3 prevNormal = gPrevNormalDepthBuffer[prevID].xyz;
        float3 prevObjectWorldPos = gPrevPositionBuffer[prevID].xyz;
        const bool isTemporalReuseEnable = isTemporalReprojectionSuccessed(currDepth, prevDepth, currNormal, prevNormal, currObjectWorldPos, prevObjectWorldPos);
        const bool isTemporalResamplingRequired = isUseReservoirTemporalReuse() && isWithinBounds(prevID, dims) && (isTemporalReuseEnable && (abs(currID.x - prevID.x) <= 1) && (abs(currID.y - prevID.y) <= 1));

        DIReservoir currDIReservoir = gDIReservoirBuffer[serialCurrID];
        if (isTemporalResamplingRequired)
        {
            DIReservoir prevDIReservoir = gDIReservoirBufferSrc[serialPrevID];
            performTemporalResampling(currDIReservoir, prevDIReservoir, randomSeed);
        }
        gDIReservoirBuffer[serialCurrID] = currDIReservoir;

        GIReservoir currGIReservoir = gGIReservoirBuffer[serialCurrID];
        if (isTemporalResamplingRequired)
        {
            GIReservoir prevGIReservoir = gGIReservoirBufferSrc[serialPrevID];
            performTemporalResampling(currGIReservoir, prevGIReservoir, randomSeed);
        }
        gGIReservoirBuffer[serialCurrID] = currGIReservoir;
    }

    finalizeRNG(launchIndex.xy, randomSeed);
}

#define SPATIAL_REUSE_NUM 4

void performSpatialResampling(inout DIReservoir spatDIReservoir, in float centerDepth, in float3 centerNormal, in float3 centerPos, inout uint randomSeed, in MaterialParams centerMaterialParams)
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    float2 dims = float2(DispatchRaysDimensions().xy);
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    DIReservoir currDIReservoir = gDIReservoirBufferSrc[serialIndex];
    combineDIReservoirs(spatDIReservoir, currDIReservoir, currDIReservoir.W_sum, rand(randomSeed));

    const bool isTransparent = isTransparentMaterial(centerMaterialParams);
    float baseSpatialReusingRadius = getDIReservoirSpatialReuseBaseRadius();

    if(isTransparent)
    {
        baseSpatialReusingRadius *= (1.5 - centerMaterialParams.transRatio) / 1.5;
        baseSpatialReusingRadius = max(1, baseSpatialReusingRadius - 1);
    }

    //combine reservoirs
    if(isUseReservoirSpatialReuse())// && (currDIReservoir.M < 5))
    {
        for(int s = 0; s < getDIReservoirSpatialReuseNum(); s++)
        {
            float2 gauss = sample2DGaussianBoxMuller(rand(randomSeed), rand(randomSeed));
            float2 pos;
            pos.x = baseSpatialReusingRadius / 1.96 * gauss.x;
            pos.y = baseSpatialReusingRadius / 1.96 * gauss.y;

            int3 nearIndex = launchIndex + int3(pos, 0);
            
            if(!isWithinBounds(nearIndex.xy, dims))
            {
                continue;
            }

            const uint serialNearID = serialRaysIndex(nearIndex, dispatchDimensions);

            DIReservoir nearDIReservoir = gDIReservoirBufferSrc[serialNearID];
            const float nearDepth = gNormalDepthBuffer[nearIndex.xy].w;
            const float3 nearNormal = gNormalDepthBuffer[nearIndex.xy].xyz;
            const float3 nearPos = gPositionBuffer[nearIndex.xy].xyz;
            MaterialParams screenSpaceMaterialNear = decompressMaterialParams(getScreenSpaceMaterial(nearIndex.xy));

            const bool isNearDepth = ((centerDepth * 0.95 < nearDepth) && (nearDepth < centerDepth * 1.05)) && (centerDepth > 0) && (nearDepth > 0);
            const bool isNearNormal = dot(centerNormal, nearNormal) > 0.9;
            const bool isNearMaterial = (abs(centerMaterialParams.roughness - screenSpaceMaterialNear.roughness) < 0.1) && (abs(centerMaterialParams.metallic - screenSpaceMaterialNear.metallic) < 0.1);

            const bool isSimilar = isNearDepth && isNearNormal && isNearMaterial;
            if(!isSimilar || (length(nearNormal) < 0.01) || (isTransparent != isTransparentMaterial(screenSpaceMaterialNear)))
            {
                continue;
            }
            const float nearUpdateW = nearDIReservoir.W_sum;
            combineDIReservoirs(spatDIReservoir, nearDIReservoir, nearUpdateW, rand(randomSeed));
        }
    }

    spatDIReservoir.applyMCapping();
}

void performSpatialResampling(inout GIReservoir spatGIReservoir, in float centerDepth, in float3 centerNormal, in float3 centerPos, inout uint randomSeed, in MaterialParams centerMaterialParams)
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    float2 dims = float2(DispatchRaysDimensions().xy);
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    GIReservoir currGIReservoir = gGIReservoirBufferSrc[serialIndex];
    combineGIReservoirs(spatGIReservoir, currGIReservoir, currGIReservoir.W_sum, rand(randomSeed));

    const bool isTransparent = isTransparentMaterial(centerMaterialParams);
    float baseSpatialReusingRadius = getGIReservoirSpatialReuseBaseRadius();

    if(isTransparent)
    {
        baseSpatialReusingRadius *= (1.5 - centerMaterialParams.transRatio) / 1.5;
        baseSpatialReusingRadius = max(1, baseSpatialReusingRadius - 1);
    }

    //combine reservoirs
    if(isUseReservoirSpatialReuse())// && (currGIReservoir.M < 5))
    {
        for(int s = 0; s < getGIReservoirSpatialReuseNum(); s++)
        {
            float2 gauss = sample2DGaussianBoxMuller(rand(randomSeed), rand(randomSeed));
            float2 pos;
            pos.x = baseSpatialReusingRadius / 1.96 * gauss.x;
            pos.y = baseSpatialReusingRadius / 1.96 * gauss.y;

            int3 nearIndex = launchIndex + int3(pos, 0);
            
            if(!isWithinBounds(nearIndex.xy, dims))
            {
                continue;
            }

            const float2 nearUV = nearIndex.xy / dims;

            const uint serialNearID = serialRaysIndex(nearIndex, dispatchDimensions);

            GIReservoir nearGIReservoir = gGIReservoirBufferSrc[serialNearID];
            const float nearDepth = gNormalDepthBuffer[nearIndex.xy].w;
            const float3 nearNormal = gNormalDepthBuffer[nearIndex.xy].xyz;
            const float3 nearPos = gPositionBuffer[nearIndex.xy].xyz;
            MaterialParams screenSpaceMaterialNear = decompressMaterialParams(getScreenSpaceMaterial(nearIndex.xy));

            const bool isNearDepth = ((centerDepth * 0.95 < nearDepth) && (nearDepth < centerDepth * 1.05)) && (centerDepth > 0) && (nearDepth > 0);
            const bool isNearNormal = dot(centerNormal, nearNormal) > 0.9;
            const bool isNearMaterial = (abs(centerMaterialParams.roughness - screenSpaceMaterialNear.roughness) < 0.1) && (abs(centerMaterialParams.metallic - screenSpaceMaterialNear.metallic) < 0.1);

            const bool isSimilar = isNearDepth && isNearNormal && isNearMaterial;
            if(!isSimilar || (length(nearNormal) < 0.01) || (isTransparent != isTransparentMaterial(screenSpaceMaterialNear)))// || isTransparentMaterial(screenSpaceMaterialNear))
            {
                continue;
            }
            const float nearUpdateW = nearGIReservoir.W_sum;
            combineGIReservoirs(spatGIReservoir, nearGIReservoir, nearUpdateW, rand(randomSeed));
        }
    }

    spatGIReservoir.applyMCapping();
}

void perfromReconnection(inout DIReservoir spatDIReservoir, in float3 wo, in float3 centerPos, in float3 centerNormal, in MaterialParams screenSpaceMaterial)
{
    LightSample lightSample;
    uint replayRandomSeed = spatDIReservoir.randomSeed;
    sampleLightWithID(centerPos, spatDIReservoir.lightID, lightSample, replayRandomSeed);
    float3 biasedPosition = centerPos + 0.01f * lightSample.distance * normalize(lightSample.directionToLight);

    float3 lightNormal = lightSample.normal;
    float3 wi = lightSample.directionToLight;
    float receiverCos = dot(centerNormal, wi);
    float emitterCos = dot(lightNormal, -wi);
    if ((spatDIReservoir.targetPDF_3f_U32 > 0) && (receiverCos > 0) && (emitterCos > 0))
    {
        const float4 f0p0 = computeBSDF_PDF(screenSpaceMaterial, centerNormal, wo, wi, replayRandomSeed);
        const float3 f0 = f0p0.xyz;
        float G = receiverCos * emitterCos / getModifiedSquaredDistance(lightSample);
        float3 FGL = saturate(f0 * G) * lightSample.emission / lightSample.pdf;
        spatDIReservoir.targetPDF_3f_U32 = compressRGBasU32(FGL);
    }

    if(!isVisible(biasedPosition, lightSample))
    {
        recognizeAsShadowedReservoir(spatDIReservoir);
    }
}

void perfromReconnection(inout GIReservoir spatGIReservoir, in float3 wo, in float3 centerPos, in float3 centerNormal, in MaterialParams screenSpaceMaterial, inout uint randomSeed, in uint serialIndex)
{
    const float3 wi = normalize(spatGIReservoir.giSample.pos2 - centerPos);
    const float4 f0p0 = computeBSDF_PDF(screenSpaceMaterial, centerNormal, wo, wi, randomSeed);
    const float3 f0 = f0p0.xyz;

    const float diffRatio = 1.0 - screenSpaceMaterial.metallic;
    //const bool isReEvaluateValid = !isTransparentMaterial(screenSpaceMaterial) && (diffRatio > 0.1); 
    const bool isReEvaluateValid = true;//(diffRatio > 0.1); 

    float cosine = abs(dot(wi, centerNormal));
    float3 Lo = decompressU32asRGB(spatGIReservoir.giSample.Lo2_U32);

    const bool isIBLSample = (length(spatGIReservoir.giSample.pos2) == 0);

    if(isReEvaluateValid && !isIBLSample)
    {
        float3 dir = spatGIReservoir.giSample.pos2 - centerPos;
        float3 biasedPosition = centerPos + 0.01f * sqrt(dot(dir, dir)) * normalize(dir);
        const float termV = 1;//isVisible(biasedPosition, spatGIReservoir.giSample.pos2) ? 1 : 0;
        spatGIReservoir.targetPDF_3f_U32 = compressRGBasU32(termV * f0 * cosine * Lo);
    }
    else
    {
        spatGIReservoir = gGIReservoirBufferSrc[serialIndex];
        if(spatGIReservoir.M > MAX_REUSE_M_GI)
        {
            float r = max(0, ((float)MAX_REUSE_M_GI / spatGIReservoir.M));
            spatGIReservoir.W_sum *= r;
            spatGIReservoir.M = MAX_REUSE_M_GI;
        }
    }
}

[shader("raygeneration")]
void spatialReuse() {
    uint3 launchIndex = DispatchRaysIndex();
    uint3 dispatchDimensions = DispatchRaysDimensions();
    float2 dims = float2(DispatchRaysDimensions().xy);
    int serialIndex = serialRaysIndex(launchIndex, dispatchDimensions);

    //random
    uint randomSeed = 0;
    initializeRNG(launchIndex.xy, randomSeed);

    if(isUseNEE() && isUseStreamingRIS())
    {
        MaterialParams screenSpaceMaterial = decompressMaterialParams(getScreenSpaceMaterial());
        const float centerDepth = gNormalDepthBuffer[launchIndex.xy].w;
        const float3 centerNormal = gNormalDepthBuffer[launchIndex.xy].xyz;
        const float3 centerPos = gPositionBuffer[launchIndex.xy].xyz;

        const float2 IJ = int2(0 / (1 / 2.f), 0 % (1 / 2.f)) - 0.5.xx;
        const float2 d = (launchIndex.xy + 0.5) / dims.xy * 2.0 - 1.0 + IJ / dims.xy;
        const float4 target = mul(gSceneParam.mtxProjInv, float4(d.x, -d.y, 1, 1));
        const float3 wo = -normalize(mul(gSceneParam.mtxViewInv, float4(target.xyz, 0)).xyz);

        DIReservoir spatDIReservoir;
        spatDIReservoir.initialize();

        performSpatialResampling(spatDIReservoir, centerDepth, centerNormal, centerPos, randomSeed, screenSpaceMaterial);
        perfromReconnection(spatDIReservoir, wo, centerPos, centerNormal, screenSpaceMaterial);

        gDIReservoirBuffer[serialIndex] = spatDIReservoir;
        
        GIReservoir spatGIReservoir;
        spatGIReservoir.initialize();

        performSpatialResampling(spatGIReservoir, centerDepth, centerNormal, centerPos, randomSeed, screenSpaceMaterial);
        perfromReconnection(spatGIReservoir, wo, centerPos, centerNormal, screenSpaceMaterial, randomSeed, serialIndex);

        gGIReservoirBuffer[serialIndex] = spatGIReservoir;
    }
    else
    {
         gDIReservoirBuffer[serialIndex] = gDIReservoirBufferSrc[serialIndex];
         gGIReservoirBuffer[serialIndex] = gGIReservoirBufferSrc[serialIndex];
    }
}