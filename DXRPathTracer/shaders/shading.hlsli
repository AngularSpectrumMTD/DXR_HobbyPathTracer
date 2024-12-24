#ifndef __SHADING_HLSLI__
#define __SHADING_HLSLI__

#include "subSurfaceScattering.hlsli"

#define ETA_AIR 1.0f
#define MEAN_FREE_PATH 1e-5f

bool isUseNEE(in MaterialParams material)
{
    return isNEEExecutable(material) && isUseNEE();
}

struct OpticalGlass
{
    float A0;
    float A1;
    float A2;
    float A3;
    float A4;
    float A5;
    float A6;
    float A7;
    float A8;

    //ex computeRefIndex(lambdaNM * 1e-3)
    float computeRefIndex(float lambdaInMicroMeter)
    {
        float lambdaPow2 = lambdaInMicroMeter * lambdaInMicroMeter;
        float lambdaPow4 = lambdaPow2 * lambdaPow2;
        float invLambdaPow2 = 1 / lambdaPow2;
        float invLambdaPow4 = invLambdaPow2 * invLambdaPow2;
        float invLambdaPow6 = invLambdaPow4 * invLambdaPow2;
        float invLambdaPow8 = invLambdaPow6 * invLambdaPow2;
        float invLambdaPow10 = invLambdaPow8 * invLambdaPow2;
        float invLambdaPow12 = invLambdaPow10 * invLambdaPow2;
        
        return sqrt(A0
		+ A1 * lambdaPow2
		+ A2 * lambdaPow4
		+ A3 * invLambdaPow2
		+ A4 * invLambdaPow4
		+ A5 * invLambdaPow6
		+ A6 * invLambdaPow8
		+ A7 * invLambdaPow10
		+ A8 * invLambdaPow12);
    }
};

static OpticalGlass J_Bak4 =
{
    2.42114503E+00,
    -8.99959341E-03,
    -9.30006854E-05,
    1.43071120E-02,
    1.89993274E-04,
    6.09602388E-06,
    2.25737069E-07,
    0.00000000E+00,
    0.00000000E+00
};

//=========================================================================
//Shading
//=========================================================================
float4 specularBSDF_PDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi)
{
    if (dot(N, wi) <= 0)
    {
        return float4(0, 0, 0, 1);
    }

    const float diffRatio = 1.0 - material.metallic;

    const float specRatio = 1 - diffRatio;
    const float3 H = normalize(wi + wo);

    const float dotNL = abs(dot(N, wi));
    const float dotNH = abs(dot(N, H));
    const float dotVH = abs(dot(wo, H));

    float3 F0 = 0.08.xxx;
    F0 = lerp(F0 * material.specular, material.albedo.xyz, (material.metallic).xxx);
        
    const float NDF = GGX_Distribution(N, H, material.roughness);
    const float G = GGX_Geometry_Smith(N, wo, wi, material.roughness);
    const float3 F = FresnelSchlick(max(dot(wo, H), 0), F0);

    const float3 kS = F;
    const float3 kD = (1 - kS) * (1 - material.metallic);
        
    const float3 specBRDF = SpecularBRDF(NDF, G, F, wo, wi, N);
    const float specPDF = GGX_ImportanceSamplingPDF(NDF, dotNH, dotVH);
    const float3 diffBRDF = DiffuseBRDF(material.albedo.rgb);
    const float diffPDF = CosineSamplingPDF(dotNL);
    const float3 sumBSDF = (diffBRDF * kD + specBRDF) * dotNL;
    const float sumPDF = diffRatio * diffPDF + specRatio * specPDF;

    if (sumPDF > 0)
    {
        return float4(sumBSDF, sumPDF);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

float4 transmitBSDF_PDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi, in float3 H, in float etaIN, in float etaOUT, in bool isRefractSampled, in bool isFromOutside)
{
    const float specRatio = FresnelReflectance(-wo, N, etaOUT);

    const float dotNL = abs(dot(N, wi));
    const float dotNV = abs(dot(N, wo));
    const float dotNH = abs(dot(N, H));
    const float dotVH = abs(dot(wo, H));
    const float dotLH = abs(dot(wi, H));

    float3 F0 = 0.08.xxx * material.specular;
    float3 F = FresnelSchlick(max(dot(H, wo), 0), F0);

    float NDF = GGX_Distribution(N, H, material.roughness);
    float G = GGX_Geometry_Smith(N, wo, wi, material.roughness);

    float3 specBRDF = SpecularBRDF(NDF, G, F, wo, wi, N);
    float specPDF = GGX_ImportanceSamplingPDF(NDF, dotNH, dotVH);
    float3 refrBTDF = RefractionBTDF(NDF, G, F, wo, wi, N, H, etaIN, etaOUT);
    float refrPDF = GGX_ImportanceSamplingPDF(NDF, dotNH, dotVH);
    const float3 sumBSDF = (specBRDF + refrBTDF * material.transColor.rgb) * dotNL;
    const float sumPDF = specRatio * specPDF + (1 - specRatio) * refrPDF;

    if (sumPDF > 0)
    {
        return float4(sumBSDF, sumPDF);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

void sampleBSDF(in MaterialParams material, in float3 N_global, inout RayDesc nextRay, inout Payload payload, in float wavelength = 0)
{
    float3 currentRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    if(payload.recursive == 2)
    {
        GIReservoir giReservoir = getGIReservoir();
        giReservoir.giSample.pos_2nd = currentRayOrigin;
        giReservoir.giSample.nml_2nd = N_global;
        giReservoir.compressedMaterial = compressMaterialParams(material);

        setGIReservoir(giReservoir);
    }

    const uint currentSeed = payload.randomSeed;
    {
        nextRay.TMin = RAY_MIN_T;
        nextRay.TMax = RAY_MAX_T;
        
        const float eps = 0.001;
        const float3 wo_global = -WorldRayDirection();

        const float roulette = rand(payload.randomSeed);
        const float blending = rand(payload.randomSeed);
        const float probability = 1 - material.transRatio;

        float3 wo_local = worldToTangent(N_global, wo_global);

        if (blending < probability)
        {
            if (wo_local.z < 0)
            {
                N_global *= -1;
            }

            wo_local = worldToTangent(N_global, wo_global);

            //sample direction
            float3 L_local = 0.xxx;
            
            const float diffRatio = 1.0 - material.metallic;
            const float3 V_local = normalize(wo_local);
            
            if (roulette < diffRatio)//diffuse
            {
                float2 dummy = 0.xx;
                L_local = HemisphereORCosineSampling(Z_AXIS, false, payload.randomSeed, dummy);
            }
            else //specular
            {
                //const float3 H = GGX_ImportanceSampling(N, material.roughness);
                //const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness);
                //L_local = normalize(2.0f * dot(V_local, H_local) * H_local - V_local);

                const float a = material.roughness * material.roughness;
                float3 Vh_local = normalize(float3(a * V_local.x, a * V_local.y, V_local.z));
                float phi = 2 * PI * rand(payload.randomSeed);
                float z = (1 - rand(payload.randomSeed)) * (1 + Vh_local.z) - Vh_local.z;
                float R = sqrt(saturate(1 - z * z));
                float2 sincosXY = 0.xx;
                sincos(phi, sincosXY.x, sincosXY.y);
                float x = R * sincosXY.y;
                float y = R * sincosXY.x;
                float3 Nh_local = float3(x, y, z) + Vh_local;
                float3 Ne_local = normalize(float3(a * Nh_local.x, a * Nh_local.y, max(0, Nh_local.z)));
                L_local = reflect(-V_local, Ne_local);
            }

            nextRay.Origin = currentRayOrigin + N_global * eps;
            nextRay.Direction = tangentToWorld(N_global, L_local);
            
            //compute bsdf    V : wo   L : wi(sample)
            float4 BSDF_PDF = specularBSDF_PDF(material, Z_AXIS, V_local, L_local);
            const float cosine = max(0, abs(L_local.z));
            const float3 weight = BSDF_PDF.xyz * cosine / BSDF_PDF.w;

            if(isDirectRay(payload))
            {
                //The influence of the initial BSDF on indirect element is evaluated at the end of RayGen
                payload.primaryBSDFU32 = compressRGBasU32(BSDF_PDF.xyz * cosine);
                payload.primaryPDF = BSDF_PDF.w;
                payload.bsdfRandomSeed = currentSeed;
            }
            else
            {
                payload.throughputU32 = compressRGBasU32(decompressU32asRGB(payload.throughputU32) * weight);
            }
        }
        else
        {
            //sample direction
            bool isFromOutside = wo_local.z > 0;

            //change the normal direction to the incident direction side
            if (!isFromOutside)
            {
                N_global *= -1;
            }

            wo_local = worldToTangent(N_global, wo_global);
            
            const float etaOUT = (wavelength > 0) ? J_Bak4.computeRefIndex(wavelength * 1e-3) : 1.7;

            float3 V_local = normalize(wo_local);
            const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness, payload.randomSeed);

            float3 L_local = 0.xxx;

            bool isRefractSampled = true;
            {
                float eta = isFromOutside ? ETA_AIR / etaOUT : etaOUT / ETA_AIR;
                float3 refractVec = refract(-V_local, H_local, eta);
                if (length(refractVec) < 0.001f)
                {
                    L_local = reflect(-V_local, H_local); //handle as total reflection
                    isRefractSampled = false;
                }
                else
                {
                    L_local = normalize(refractVec);
                    isRefractSampled = true;
                }
                nextRay.Origin = currentRayOrigin - N_global * eps;
                nextRay.Direction = tangentToWorld(N_global, L_local);
            }

            //compute bsdf    V : wo   L : wi(sample)
            float4 BSDF_PDF = transmitBSDF_PDF(material, Z_AXIS, V_local, L_local, H_local, ETA_AIR, etaOUT, isRefractSampled, isFromOutside);
            const float cosine = max(0, abs(L_local.z));
            const float3 weight = BSDF_PDF.xyz * cosine / BSDF_PDF.w;

            if(isDirectRay(payload))
            {
                //The influence of the initial BSDF on indirect element is evaluated at the end of RayGen
                payload.primaryBSDFU32 = compressRGBasU32(BSDF_PDF.xyz * cosine);
                payload.primaryPDF = BSDF_PDF.w;
                payload.bsdfRandomSeed = currentSeed;
            }
            else
            {
                payload.throughputU32 = compressRGBasU32(decompressU32asRGB(payload.throughputU32) * weight);
            }
        }
    }
}

void sampleBSDF(in MaterialParams material, in float3 N_global, inout RayDesc nextRay, inout uint throughputU32, inout uint randomSeed, in float wavelength = 0)
{
    nextRay.TMin = RAY_MIN_T;
    nextRay.TMax = RAY_MAX_T;
    
    const float eps = 0.001;
    const float3 wo_global = -WorldRayDirection();
    const float3 currentRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    const float roulette = rand(randomSeed);
    const float blending = rand(randomSeed);
    const float probability = 1 - material.transRatio;

    float3 wo_local = worldToTangent(N_global, wo_global);

    if (blending < probability)
    {
        if (wo_local.z < 0)
        {
            N_global *= -1;
        }

        wo_local = worldToTangent(N_global, wo_global);

        //sample direction
        float3 L_local = 0.xxx;
        
        const float diffRatio = 1.0 - material.metallic;
        const float3 V_local = normalize(wo_local);
        
        if (roulette < diffRatio)//diffuse
        {
            float2 dummy = 0.xx;
            L_local = HemisphereORCosineSampling(Z_AXIS, false, randomSeed, dummy);
        }
        else //specular
        {
            //const float3 H = GGX_ImportanceSampling(N, material.roughness);
            //const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness);
            //L_local = normalize(2.0f * dot(V_local, H_local) * H_local - V_local);

            const float a = material.roughness * material.roughness;
            float3 Vh_local = normalize(float3(a * V_local.x, a * V_local.y, V_local.z));
            float phi = 2 * PI * rand(randomSeed);
            float z = (1 - rand(randomSeed)) * (1 + Vh_local.z) - Vh_local.z;
            float R = sqrt(saturate(1 - z * z));
            float2 sincosXY = 0.xx;
            sincos(phi, sincosXY.x, sincosXY.y);
            float x = R * sincosXY.y;
            float y = R * sincosXY.x;
            float3 Nh_local = float3(x, y, z) + Vh_local;
            float3 Ne_local = normalize(float3(a * Nh_local.x, a * Nh_local.y, max(0, Nh_local.z)));
            L_local = reflect(-V_local, Ne_local);
        }

        nextRay.Origin = currentRayOrigin + N_global * eps;
        nextRay.Direction = tangentToWorld(N_global, L_local);
        
        //compute bsdf    V : wo   L : wi(sample)
        float4 BSDF_PDF = specularBSDF_PDF(material, Z_AXIS, V_local, L_local);
        const float cosine = max(0, abs(L_local.z));
        const float3 weight = BSDF_PDF.xyz * cosine / BSDF_PDF.w;
        throughputU32 = compressRGBasU32(decompressU32asRGB(throughputU32) * weight);
    }
    else
    {
        //sample direction
        bool isFromOutside = wo_local.z > 0;

        //change the normal direction to the incident direction side
        if (!isFromOutside)
        {
            N_global *= -1;
        }

        wo_local = worldToTangent(N_global, wo_global);
        
        const float etaOUT = (wavelength > 0) ? J_Bak4.computeRefIndex(wavelength * 1e-3) : 1.7;

        float3 V_local = normalize(wo_local);
        const float3 H_local = Z_AXIS;//ImportanceSampling(Z_AXIS, material.roughness);

        float3 L_local = 0.xxx;

        bool isRefractSampled = true;
        {
            float eta = isFromOutside ? ETA_AIR / etaOUT : etaOUT / ETA_AIR;
            float3 refractVec = refract(-V_local, H_local, eta);
            if (length(refractVec) < 0.001f)
            {
                L_local = reflect(-V_local, H_local); //handle as total reflection
                isRefractSampled = false;
            }
            else
            {
                L_local = normalize(refractVec);
                isRefractSampled = true;
            }
            nextRay.Origin = currentRayOrigin - N_global * eps;
            nextRay.Direction = tangentToWorld(N_global, L_local);
        }

        //compute bsdf    V : wo   L : wi(sample)
        float4 BSDF_PDF = transmitBSDF_PDF(material, Z_AXIS, V_local, L_local, H_local, ETA_AIR, etaOUT, isRefractSampled, isFromOutside);
        const float cosine = max(0, abs(L_local.z));
        const float3 weight = BSDF_PDF.xyz * cosine / BSDF_PDF.w;
        throughputU32 = compressRGBasU32(decompressU32asRGB(throughputU32) * weight);
    }
}

float4 computeBSDF_PDF(in MaterialParams material, in float3 N_global, in float3 wo_global, in float3 wi_global, inout uint randomSeed, in float wavelength = 0)
{
    float4 BSDF_PDF = 0.xxxx;

    const float eps = 0.001;

    const float roulette = rand(randomSeed);
    const float blending = rand(randomSeed);
    const float probability = 1 - material.transRatio;

    float3 wo_local = worldToTangent(N_global, wo_global);
    float3 L_local = worldToTangent(N_global, wi_global);

    if (blending < probability)
    {
        if (wo_local.z < 0)
        {
            N_global *= -1;
        }

        wo_local = worldToTangent(N_global, wo_global);
        
        const float3 V_local = normalize(wo_local);
        
        //compute bsdf    V : wo   L : wi(sample)
        BSDF_PDF = specularBSDF_PDF(material, Z_AXIS, V_local, L_local);
    }
    else
    {
        //sample direction
        bool isFromOutside = wo_local.z > 0;

        //change the normal direction to the incident direction side
        if (!isFromOutside)
        {
            N_global *= -1;
        }

        wo_local = worldToTangent(N_global, wo_global);

        const float etaOUT = (wavelength > 0) ? J_Bak4.computeRefIndex(wavelength * 1e-3) : 1.7;

        float3 V_local = normalize(wo_local);
        const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness, randomSeed);

        //compute bsdf    V : wo   L : wi(sample)
        BSDF_PDF = transmitBSDF_PDF(material, Z_AXIS, V_local, L_local, H_local, ETA_AIR, etaOUT, true, isFromOutside);
    }

    return BSDF_PDF;
}

void sampleLightStreamingRIS(in MaterialParams material, in float3 scatterPosition, in float3 surfaceNormal, inout LightSample lightSample, out DIReservoir reservoir, inout uint randomSeed)
{
    const float pdf = 1.0f / getLightNum();//ordinal pdf to get the one sample from all lights
    float p_hat = 0;

    reservoir.initialize();

    [unroll]
    for (int i = 0; i < 8; i++)
    {
        const uint lightID = getRandomLightID(randomSeed);
        const uint replayRandomSeed = randomSeed;
        sampleLightWithID(scatterPosition, lightID, lightSample, randomSeed);

        float3 lightNormal = lightSample.normal;
        float3 wi = lightSample.directionToLight;
        float receiverCos = dot(surfaceNormal, wi);
        float emitterCos = dot(lightNormal, -wi);
        float4 bsdfPDF = computeBSDF_PDF(material, surfaceNormal, -WorldRayDirection(), wi, randomSeed);
        float G = max(0, receiverCos) * max(0, emitterCos) / getModifiedSquaredDistance(lightSample);

        if((i > 0) && (G < 0.00001f))
        {
            continue;
        }

        float3 FGL = saturate(bsdfPDF.xyz * G) * lightSample.emission / lightSample.pdf;

        float3 p_hat_3F = FGL;
        p_hat = computeLuminance(FGL);

        float updateW = p_hat / pdf;

        updateDIReservoir(reservoir, lightID, replayRandomSeed, updateW, p_hat, compressRGBasU32(p_hat_3F), 1u, rand(randomSeed));
    }
    uint replaySeed = reservoir.randomSeed;
    sampleLightWithID(scatterPosition, reservoir.lightID, lightSample, replaySeed);
}

float3 performNEE(inout Payload payload, in MaterialParams material, in float3 scatterPosition, in float3 surfaceNormal, inout DIReservoir reservoir, in float3 originalNormal, in float3 originalScatterPositionForSSS)
{
    float3 estimatedColor = 0.xxx;
    if (isUseStreamingRIS())
    {
        LightSample lightSample;

        bool visibility = false;

        if(isSSSExecutable(material))
        {
            sampleLightStreamingRIS(material, scatterPosition, surfaceNormal, lightSample, reservoir, payload.randomSeed);
            visibility = isVisible(scatterPosition, lightSample);

            if(!visibility)
            {
                uint reservoirRandomSeed = reservoir.randomSeed;
                sampleLightWithID(originalScatterPositionForSSS, reservoir.lightID, lightSample, reservoirRandomSeed);

                //recomputation
                float3 lightNormal = lightSample.normal;
                float3 wi = lightSample.directionToLight;
                float receiverCos = dot(surfaceNormal, wi);
                float emitterCos = dot(lightNormal, -wi);
                float4 bsdfPDF = computeBSDF_PDF(material, surfaceNormal, -WorldRayDirection(), wi, reservoirRandomSeed);
                float G = max(0, receiverCos) * max(0, emitterCos) / getModifiedSquaredDistance(lightSample);
                float3 FGL = saturate(bsdfPDF.xyz * G) * lightSample.emission / lightSample.pdf;

                return isVisible(originalScatterPositionForSSS, lightSample) ? FGL : 0.xxx;
            }
        }
        else
        {
            sampleLightStreamingRIS(material, scatterPosition, surfaceNormal, lightSample, reservoir, payload.randomSeed);
            visibility = isVisible(scatterPosition, lightSample);
        }

        if (visibility)
        {
            estimatedColor = shadeDIReservoir(reservoir);
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
        sampleLight(scatterPosition, lightSample, payload.randomSeed);
        float3 lightNormal = lightSample.normal;
        float3 wi = lightSample.directionToLight;
        float receiverCos = dot(surfaceNormal, wi);
        float emitterCos = dot(lightNormal, -wi);
        if (isVisible(scatterPosition, lightSample) && (receiverCos > 0) && (emitterCos > 0))
        {
            float4 bsdfPDF = computeBSDF_PDF(material, surfaceNormal, -WorldRayDirection(), wi, payload.randomSeed);
            float G = receiverCos * emitterCos / getModifiedSquaredDistance(lightSample);
            float3 FGL = saturate(bsdfPDF.xyz * G) * lightSample.emission / lightSample.pdf;
            estimatedColor = FGL;
        }
    }
    return estimatedColor;
}

bool applyLighting(inout Payload payload, in MaterialParams material, in float3 scatterPosition, in float3 surfaceNormal, out float3 hitLe,  out float3 hitPosition, out float3 hitNormal, in float3 originalSurfaceNormal, in float3 originalScatterPositionForSSS, out float3 DIGIelement)
{
    bool isFinish = false;
    float3 Le = 0.xxx;
    setNEEFlag(payload, isUseNEE(material));

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

    if (isUseNEE(material))
    {
        //ray hitted the light source
        if (isIntersect && isDirectRay(payload) && !isIndirectOnly())
        {
            DIGIelement = decompressU32asRGB(payload.throughputU32) * Le;
            isFinish = true;
            return isFinish;
        }

        //ray hitted the emissive material
        if (material.emission.x + material.emission.y + material.emission.z > 0)
        {
            DIGIelement = decompressU32asRGB(payload.throughputU32) * material.emission.xyz;
            isFinish = false;
            return isFinish;
        }

        const bool isNEELightingRequired = isIndirectOnly() ? isIndirectRay(payload) : true;
        if (isNEELightingRequired)
        {
            DIReservoir reservoir;
            DIGIelement = performNEE(payload, material, scatterPosition, surfaceNormal, reservoir, originalSurfaceNormal, originalScatterPositionForSSS) * decompressU32asRGB(payload.throughputU32);
            if(isDirectRay(payload) && isUseStreamingRIS() && !isSSSExecutable(material))
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
        //ray hitted the light source
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

        //ray hitted the emissive material
        if (material.emission.x + material.emission.y + material.emission.z > 0)
        {
            DIGIelement = decompressU32asRGB(payload.throughputU32) * material.emission.xyz;
            isFinish = false;
            return isFinish;
        }
    }

    isFinish = false;
    return isFinish;
}

bool shadeAndSampleRay(in float3 vertexNormal, in float3 vertexPosition, in float3 geomNormal, inout Payload payload, in MaterialParams currentMaterial, inout RayDesc nextRay, in float3 caustics)
{
    float3 surfaceNormal = vertexNormal;
    float3 scatterPosition = mul(float4(vertexPosition, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(surfaceNormal, (float3x3)ObjectToWorld4x3());
    float3 originalSurfaceNormal = surfaceNormal;
    float3 originalScatterPosition = scatterPosition;

    float3 hitLe = 0.xxx;
    float3 hitNormal = 0.xxx;
    float3 hitPosition = 0.xxx;

    if(isSSSExecutable(currentMaterial))
    {
        computeSSSPosition(payload, scatterPosition, surfaceNormal, geomNormal);
    }
    float3 DIGIelement = 0.xxx;
    const bool isTerminate = applyLighting(payload, currentMaterial, scatterPosition, surfaceNormal, hitLe, hitPosition, hitNormal, originalSurfaceNormal, originalScatterPosition, DIGIelement);
    if(isDirectRay(payload))
    {
        setDI(DIGIelement);
    }
    if(isIndirectRay(payload))
    {
        addGI(DIGIelement);
    }
    const bool isAnaliticalLightHitted = (length(hitLe) > 0);
    const float3 writeColor = isAnaliticalLightHitted ? hitLe : currentMaterial.albedo.xyz;
    const float3 writeNormal = isAnaliticalLightHitted ? hitNormal : originalSurfaceNormal;//this param is used for denoise, so we have to get stable normal wthe we execute SSS
    const float3 writePosition = isAnaliticalLightHitted ? hitPosition : originalScatterPosition;//this param is used for denoise, so we have to get stable normal wthe we execute SSS
    storeGBuffer(payload, writePosition, writeColor, writeNormal, currentMaterial.roughness, currentMaterial);

    if (isTerminate)
    {
        return true;
    }
    
    addCaustics(caustics);

    nextRay.Origin = scatterPosition;
    nextRay.Direction = 0.xxx;
    sampleBSDF(currentMaterial, surfaceNormal, nextRay, payload);

    return false;
}

#endif//__SHADING_HLSLI__