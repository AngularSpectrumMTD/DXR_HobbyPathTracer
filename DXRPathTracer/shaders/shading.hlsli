#ifndef __SHADING_HLSLI__
#define __SHADING_HLSLI__

#include "subSurfaceScattering.hlsli"

bool isUseNEE(in MaterialParams material)
{
    return isNEEExecutable(material) && isUseNEE();
}

//=========================================================================
//Shading
//=========================================================================
void sampleBSDF(in Surface surface, inout RayDesc nextRay, inout Payload payload, in float wavelength = 0)
{
    float3 currentRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    float3 n_global = surface.normal;
    MaterialParams currMaterial = surface.material;

    //to store the normal and position of secondary hit
    if(payload.recursive == 2)
    {
        GIReservoir giReservoir;
        giReservoir.initialize();
        giReservoir.giSample.pos2 = currentRayOrigin;
        giReservoir.giSample.nml2 = n_global;
        //giReservoir.compressedMaterial = compressMaterialParams(currMaterial);

        setGIReservoir(giReservoir);
    }

    const uint currentSeed = payload.randomSeed;
    {
        nextRay.TMin = RAY_MIN_T;
        nextRay.TMax = RAY_MAX_T;
        
        const float3 wo_global = -WorldRayDirection();

        const float roulette = rand(payload.randomSeed);
        const float blending = rand(payload.randomSeed);
        const float probability = 1 - currMaterial.transRatio;

        float3 wo_local = worldToTangent(n_global, wo_global);

        if (blending < probability)
        {
            if (wo_local.z < 0)
            {
                n_global *= -1;
            }

            wo_local = worldToTangent(n_global, wo_global);

            //sample direction
            float3 wi_local = 0.xxx;
            
            const float diffRatio = 1.0 - currMaterial.metallic;
            wo_local = normalize(wo_local);
            
            if (roulette < diffRatio)//diffuse
            {
                float2 dummy = 0.xx;
                wi_local = HemisphereORCosineSampling(Z_AXIS, false, payload.randomSeed, dummy);
            }
            else //specular
            {
                //const float3 H = GGX_ImportanceSampling(N, material.roughness);
                //const float3 halfVec_local = ImportanceSampling(Z_AXIS, material.roughness);
                //wi_local = normalize(2.0f * dot(wo_local, halfVec_local) * halfVec_local - wo_local);

                const float a = currMaterial.roughness * currMaterial.roughness;
                float3 Vh_local = normalize(float3(a * wo_local.x, a * wo_local.y, wo_local.z));
                float phi = 2 * PI * rand(payload.randomSeed);
                float z = (1 - rand(payload.randomSeed)) * (1 + Vh_local.z) - Vh_local.z;
                float R = sqrt(saturate(1 - z * z));
                float2 sincosXY = 0.xx;
                sincos(phi, sincosXY.x, sincosXY.y);
                float x = R * sincosXY.y;
                float y = R * sincosXY.x;
                float3 Nh_local = float3(x, y, z) + Vh_local;
                float3 Ne_local = normalize(float3(a * Nh_local.x, a * Nh_local.y, max(0, Nh_local.z)));
                wi_local = reflect(-wo_local, Ne_local);
            }

            nextRay.Origin = currentRayOrigin + surface.geomNormal * RAY_T_BIAS;
            nextRay.Direction = tangentToWorld(n_global, wi_local);
            
            //compute bsdf    V : wo   L : wi(sample)
            float4 fp = specularBSDF_PDF(currMaterial, Z_AXIS, wo_local, wi_local);
            fp.w *= probability;
            const float cosine = max(0, abs(wi_local.z));
            const float3 weight = fp.xyz * cosine / fp.w;

            if(isDirectRay(payload))
            {
                //The influence of the initial BSDF on indirect element is evaluated at the end of ray generation shader
                //So, we DON'T update the value of the throughput at this time
                payload.f0 = compressRGBasU32(fp.xyz * cosine);
                payload.p0 = fp.w;
                payload.bsdfRandomSeed = currentSeed;
            }
            else
            {
                payload.updateThroughputByMulitiplicationF3(weight);
            }
        }
        else
        {
            //sample direction
            bool isFromOutside = wo_local.z > 0;

            //change the normal direction to the incident direction side
            if (!isFromOutside)
            {
                n_global *= -1;
            }

            wo_local = worldToTangent(n_global, wo_global);
            
            const float etaOUT = (wavelength > 0) ? J_Bak4.computeRefIndex(wavelength * 1e-3) : 1.7;

            wo_local = normalize(wo_local);
            const float3 halfVec_local = GGX_ImportanceSampling(Z_AXIS, currMaterial.roughness, payload.randomSeed);

            float3 wi_local = 0.xxx;

            {
                float eta = isFromOutside ? ETA_AIR / etaOUT : etaOUT / ETA_AIR;
                float3 refractVec = refract(-wo_local, halfVec_local, eta);
                if (length(refractVec) < 0.001f)
                {
                    wi_local = reflect(-wo_local, halfVec_local); //handle as total reflection
                }
                else
                {
                    wi_local = normalize(refractVec);
                }
                nextRay.Origin = currentRayOrigin - surface.geomNormal * RAY_T_BIAS;
                nextRay.Direction = tangentToWorld(n_global, wi_local);
            }

            //compute bsdf    V : wo   L : wi(sample)
            float4 fp = transmitBSDF_PDF(currMaterial, Z_AXIS, wo_local, wi_local, halfVec_local, ETA_AIR, etaOUT);
            fp.w *= (1 - probability);
            const float cosine = max(0, abs(wi_local.z));
            const float3 weight = fp.xyz * cosine / fp.w;

            if(isDirectRay(payload))
            {
                //The influence of the initial BSDF on indirect element is evaluated at the end of ray generation shader
                //So, we DON'T update the value of the throughput at this time
                payload.f0 = compressRGBasU32(fp.xyz * cosine);
                payload.p0 = fp.w;
                payload.bsdfRandomSeed = currentSeed;
            }
            else
            {
                payload.updateThroughputByMulitiplicationF3(weight);
            }
        }
    }
}

void sampleBSDF(in Surface surface, inout RayDesc nextRay, inout PhotonPayload payload, in float wavelength = 0)
{
    float3 currentRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    float3 n_global = surface.normal;
    MaterialParams currMaterial = surface.material;

    const uint currentSeed = payload.randomSeed;
    {
        nextRay.TMin = RAY_MIN_T;
        nextRay.TMax = RAY_MAX_T;
        
        const float3 wo_global = -WorldRayDirection();

        const float roulette = rand(payload.randomSeed);
        const float blending = rand(payload.randomSeed);
        const float probability = 1 - currMaterial.transRatio;

        float3 wo_local = worldToTangent(n_global, wo_global);

        if (blending < probability)
        {
            if (wo_local.z < 0)
            {
                n_global *= -1;
            }

            wo_local = worldToTangent(n_global, wo_global);

            //sample direction
            float3 wi_local = 0.xxx;
            
            const float diffRatio = 1.0 - currMaterial.metallic;
            wo_local = normalize(wo_local);
            
            if (roulette < diffRatio)//diffuse
            {
                float2 dummy = 0.xx;
                wi_local = HemisphereORCosineSampling(Z_AXIS, false, payload.randomSeed, dummy);
            }
            else //specular
            {
                //const float3 H = GGX_ImportanceSampling(N, material.roughness);
                //const float3 halfVec_local = ImportanceSampling(Z_AXIS, material.roughness);
                //wi_local = normalize(2.0f * dot(wo_local, halfVec_local) * halfVec_local - wo_local);

                const float a = currMaterial.roughness * currMaterial.roughness;
                float3 Vh_local = normalize(float3(a * wo_local.x, a * wo_local.y, wo_local.z));
                float phi = 2 * PI * rand(payload.randomSeed);
                float z = (1 - rand(payload.randomSeed)) * (1 + Vh_local.z) - Vh_local.z;
                float R = sqrt(saturate(1 - z * z));
                float2 sincosXY = 0.xx;
                sincos(phi, sincosXY.x, sincosXY.y);
                float x = R * sincosXY.y;
                float y = R * sincosXY.x;
                float3 Nh_local = float3(x, y, z) + Vh_local;
                float3 Ne_local = normalize(float3(a * Nh_local.x, a * Nh_local.y, max(0, Nh_local.z)));
                wi_local = reflect(-wo_local, Ne_local);
            }

            nextRay.Origin = currentRayOrigin + surface.geomNormal * RAY_T_BIAS;
            nextRay.Direction = tangentToWorld(n_global, wi_local);
            
            //compute bsdf    V : wo   L : wi(sample)
            float4 fp = specularBSDF_PDF(currMaterial, Z_AXIS, wo_local, wi_local);
            fp.w *= probability;
            const float cosine = max(0, abs(wi_local.z));
            const float3 weight = fp.xyz * cosine / fp.w;
            payload.updateThroughputByMulitiplicationF3(weight);
        }
        else
        {
            //sample direction
            bool isFromOutside = wo_local.z > 0;

            //change the normal direction to the incident direction side
            if (!isFromOutside)
            {
                n_global *= -1;
            }

            wo_local = worldToTangent(n_global, wo_global);
            
            const float etaOUT = (payload.lambdaNM > 0) ? J_Bak4.computeRefIndex(payload.lambdaNM * 1e-3) : 1.7;

            wo_local = normalize(wo_local);
            const float3 halfVec_local = GGX_ImportanceSampling(Z_AXIS, currMaterial.roughness, payload.randomSeed);

            float3 wi_local = 0.xxx;

            {
                float eta = isFromOutside ? ETA_AIR / etaOUT : etaOUT / ETA_AIR;
                float3 refractVec = refract(-wo_local, halfVec_local, eta);
                if (length(refractVec) < 0.001f)
                {
                    wi_local = reflect(-wo_local, halfVec_local); //handle as total reflection
                }
                else
                {
                    wi_local = normalize(refractVec);
                }
                nextRay.Origin = currentRayOrigin - surface.geomNormal * RAY_T_BIAS;
                nextRay.Direction = tangentToWorld(n_global, wi_local);
            }

            //compute bsdf    V : wo   L : wi(sample)
            float4 fp = transmitBSDF_PDF(currMaterial, Z_AXIS, wo_local, wi_local, halfVec_local, ETA_AIR, etaOUT);
            fp.w *= 1 - probability;
            const float cosine = max(0, abs(wi_local.z));
            const float3 weight = fp.xyz * cosine / fp.w;
            payload.updateThroughputByMulitiplicationF3(weight);
        }
    }
}

float4 computeLambertianBSDF_PDF(in MaterialParams material, in float3 n_global, in float3 wo_global, in float3 wi_global, inout uint randomSeed, in float wavelength = 0)
{
    float4 fp = 0.xxxx;

    const float roulette = rand(randomSeed);
    const float blending = rand(randomSeed);
    const float probability = 1 - material.transRatio;

    float3 wo_local = worldToTangent(n_global, wo_global);
    float3 wi_local = worldToTangent(n_global, wi_global);

   if (wo_local.z < 0)
    {
        n_global *= -1;
    }

    wo_local = worldToTangent(n_global, wo_global);
    
    wo_local = normalize(wo_local);
    
    //compute bsdf    V : wo   L : wi(sample)
    fp = ForceLambertianBSDF_PDF(material, Z_AXIS, wo_local, wi_local);

    return fp;
}

void sampleLightStreamingRIS(in Surface surface, inout LightSample lightSample, out DIReservoir reservoir, inout uint randomSeed, in bool isSSSSample)
{
    const float pdf = 1.0f / getLightNum();//ordinal pdf to get the one sample from all lights
    float p_hat = 0;

    reservoir.initialize();

    [unroll]
    for (int i = 0; i < 8; i++)
    {
        const uint lightID = getRandomLightID(randomSeed);
        const uint replayRandomSeed = randomSeed;
        sampleLightWithID(surface.position, lightID, lightSample, randomSeed);

        float3 lightNormal = lightSample.normal;
        float3 wi = lightSample.directionToLight;
        float receiverCos = dot(surface.normal, wi);
        float emitterCos = dot(lightNormal, -wi);

        float4 fp = 0.xxxx;
        if(isSSSSample)
        {
            MaterialParams sssMaterial = surface.material;
            sssMaterial.albedo = 1.xxxx;
            fp = computeLambertianBSDF_PDF(sssMaterial, surface.normal, -WorldRayDirection(), wi, randomSeed);
        }
        else
        {
            fp = computeBSDF_PDF(surface.material, surface.normal, -WorldRayDirection(), wi, randomSeed);
        }
        float G = max(0, receiverCos) * max(0, emitterCos) / getModifiedSquaredDistance(lightSample);

        if((i > 0) && (G < 0.00001f))
        {
            continue;
        }

        float3 FGL = saturate(fp.xyz * G) * lightSample.emission / lightSample.pdf;

        float3 p_hat_3F = FGL;
        p_hat = computeLuminance(FGL);

        float updateW = p_hat / pdf;

        updateDIReservoir(reservoir, lightID, replayRandomSeed, updateW, p_hat, compressRGBasU32(p_hat_3F), 1u, rand(randomSeed));
    }
    uint replaySeed = reservoir.randomSeed;
    sampleLightWithID(surface.position, reservoir.lightID, lightSample, replaySeed);
    reservoir.M = 1;
}

float3 performNEE(inout Payload payload, in Surface surface, inout DIReservoir reservoir, in float3 originalNormal, in float3 originalScatterPositionForSSS, in bool isSSSSample)
{
    float3 estimatedColor = 0.xxx;
    if (isUseStreamingRIS())
    {
        LightSample lightSample;

        bool visibility = false;

        if(isSSSSample)
        {
            sampleLightStreamingRIS(surface, lightSample, reservoir, payload.randomSeed, true);
            visibility = isVisible(surface.position, lightSample);

            if(!visibility)
            {
                uint reservoirRandomSeed = reservoir.randomSeed;
                sampleLightWithID(originalScatterPositionForSSS, reservoir.lightID, lightSample, reservoirRandomSeed);

                //recomputation
                float3 lightNormal = lightSample.normal;
                float3 wi = lightSample.directionToLight;
                float receiverCos = dot(surface.normal, wi);
                float emitterCos = dot(lightNormal, -wi);
                //Only albedo on the incident side is considered
                MaterialParams sssMaterial = surface.material;
                sssMaterial.albedo = 1.xxxx;
                float4 fp = computeLambertianBSDF_PDF(sssMaterial, surface.normal, -WorldRayDirection(), wi, reservoirRandomSeed);
                float G = max(0, receiverCos) * max(0, emitterCos) / getModifiedSquaredDistance(lightSample);
                float3 FGL = saturate(fp.xyz * G) * lightSample.emission / lightSample.pdf;

                return isVisible(originalScatterPositionForSSS, lightSample) ? FGL : 0.xxx;
            }
        }
        else
        {
            sampleLightStreamingRIS(surface, lightSample, reservoir, payload.randomSeed, false);
            visibility = isVisible(surface.position, lightSample);
        }

        if (visibility)
        {
            estimatedColor = resolveDIReservoir(reservoir);
        }
        else
        {
            recognizeAsShadowedReservoir(reservoir);
        }
    }
    else
    {
        //explicitly connect to light source
        LightSample lightSample;
        sampleLight(surface.position, lightSample, payload.randomSeed);
        float3 lightNormal = lightSample.normal;
        float3 wi = lightSample.directionToLight;
        float receiverCos = dot(surface.normal, wi);
        float emitterCos = dot(lightNormal, -wi);
        if (isVisible(surface.position, lightSample) && (receiverCos > 0) && (emitterCos > 0))
        {
            float4 fp = computeBSDF_PDF(surface.material, surface.normal, -WorldRayDirection(), wi, payload.randomSeed);
            float G = receiverCos * emitterCos / getModifiedSquaredDistance(lightSample);
            float3 FGL = saturate(fp.xyz * G) * lightSample.emission / lightSample.pdf;
            estimatedColor = FGL;
        }
    }
    return estimatedColor;
}

bool performLighting(inout Payload payload, in Surface surface, out float3 hitLe,  out float3 hitPosition, out float3 hitNormal, in float3 originalSurfaceNormal, in float3 originalScatterPositionForSSS, out float3 DIGIelement, in bool isSSSSample)
{
    bool isFinish = false;
    float3 Le = 0.xxx;
    setNEEFlag(payload, isUseNEE(surface.material));

    bool isIntersect = false;
    if (isDirectRay(payload))
    {
        isIntersect = intersectAllLightWithCurrentRay(Le, hitPosition, hitNormal);
        hitLe = isIntersect ? Le : 0.xxx;
    }
    else
    {
        isIntersect = intersectLightWithCurrentRay(Le, payload.randomSeed);
    }

    if (isUseNEE(surface.material))
    {
        //ray hit the light source
        if (isIntersect && isDirectRay(payload) && !isIndirectOnly())
        {
            DIGIelement = decompressU32asRGB(payload.throughputU32) * Le;
            payload.terminate();
            isFinish = true;
            return isFinish;
        }

        //ray hit the emissive material
        if (surface.material.emission.x + surface.material.emission.y + surface.material.emission.z > 0)
        {
            DIGIelement = decompressU32asRGB(payload.throughputU32) * surface.material.emission.xyz;
            payload.terminate();
            isFinish = false;
            return isFinish;
        }

        const bool isNEELightingRequired = isIndirectOnly() ? isIndirectRay(payload) : true;
        if (isNEELightingRequired)
        {
            DIReservoir reservoir;
            DIGIelement = performNEE(payload, surface, reservoir, originalSurfaceNormal, originalScatterPositionForSSS, isSSSSample) * decompressU32asRGB(payload.throughputU32);
            if(isDirectRay(payload) && isUseStreamingRIS() && !isSSSExecutable(surface.material))
            {
                //When we use the ordinal ReSTIR to SSS evaluated sample, that is return to non SSS sample.
                DIGIelement = 0.xxx;
                setDIReservoir(reservoir);
            }
        }
        isFinish = false;
        return isFinish;
    }
    else
    {
        //ray hit the light source
        if (isIntersect)
        {
            const bool isLighting = isIndirectOnly() ? isIndirectRay(payload) : true;
            if (isLighting)
            {
                DIGIelement = decompressU32asRGB(payload.throughputU32) * Le;
            }
            isFinish = true;
            return isFinish;
        }

        //ray hit the emissive material
        if (surface.material.emission.x + surface.material.emission.y + surface.material.emission.z > 0)
        {
            DIGIelement = decompressU32asRGB(payload.throughputU32) * surface.material.emission.xyz;
            isFinish = false;
            return isFinish;
        }
    }

    isFinish = false;
    return isFinish;
}

bool shadeAndSampleRay(in Surface surface, inout Payload payload, inout RayDesc nextRay, in float3 caustics)
{
    float3 surfaceNormal = surface.normal;
    float3 originalSurfaceNormal = surfaceNormal;
    float3 originalScatterPosition = surface.position;

    float3 hitLe = 0.xxx;
    float3 hitNormal = 0.xxx;
    float3 hitPosition = 0.xxx;

    MaterialParams currentMaterial = surface.material;

    bool isSSSSample = false;
    if(isSSSExecutable(currentMaterial))
    {
        isSSSSample = computeSSSPosition(payload, surface.position, surfaceNormal, surface.geomNormal);
    }

    if(isSSSSample)
    {
        payload.updateThroughputByMulitiplicationF3(currentMaterial.albedo.xyz);
    }

    float3 DIGIelement = 0.xxx;
    bool isTerminate = performLighting(payload, surface, hitLe, hitPosition, hitNormal, originalSurfaceNormal, originalScatterPosition, DIGIelement, isSSSSample);
    if(isDirectRay(payload))
    {
        setDI(DIGIelement);
    }
    if(isIndirectRay(payload))
    {
        addGI(DIGIelement);
    }
    const bool isAnaliticalLightHit = (length(hitLe) > 0);
    const float3 writeColor = isAnaliticalLightHit ? hitLe : currentMaterial.albedo.xyz;
    const float3 writeNormal = isAnaliticalLightHit ? hitNormal : originalSurfaceNormal;//this param is used for denoise, so we have to get stable normal when we execute SSS
    const float3 writePosition = isAnaliticalLightHit ? hitPosition : originalScatterPosition;//this param is used for denoise, so we have to get stable normal when we execute SSS
    storeGBuffer(payload, writePosition, writeColor, writeNormal, currentMaterial.roughness, currentMaterial);
    
    addCaustics(caustics);

    nextRay.Origin = surface.position;
    nextRay.Direction = 0.xxx;
    sampleBSDF(surface, nextRay, payload);

    if(!isTransparentMaterial(surface.material) && dot(surface.normal, nextRay.Direction) < 1e-2)
    {
        isTerminate = true;
    }
    //debug
    // if(isDirectRay(payload))
    // {
    //     payload.throughputU32 = compressRGBasU32(float3(1, 0, 0));
    // }

    return isTerminate;
}

#endif//__SHADING_HLSLI__