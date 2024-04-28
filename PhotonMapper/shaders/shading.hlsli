#ifndef __SHADING_HLSLI__
#define __SHADING_HLSLI__

#define ETA_AIR 1.0f

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
float3 specularBRDFdevidedPDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi)
{
    if (dot(N, wi) <= 0)
    {
        return 0.xxx;
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
    const float3 sumBRDF = (diffBRDF * kD + specBRDF) * dotNL;
    const float sumPDF = diffRatio * diffPDF + specRatio * specPDF;

    if (sumPDF > 0)
    {
        return sumBRDF / sumPDF;
    }
    else
    {
        return 0.xxx;
    }
}

float3 transBRDFdevidedPDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi, in float3 H, in float etaIN, in float etaOUT, in bool isRefractSampled, in bool isFromOutside)
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
    const float3 sumBRDF = (specBRDF + refrBTDF * material.transColor.rgb) * dotNL;
    const float sumPDF = specRatio * specPDF + (1 - specRatio) * refrPDF;

    if (sumPDF > 0)
    {
        return sumBRDF / sumPDF;
    }
    else
    {
        return 0.xxx;
    }
}

float4 specularBRDFandPDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi)
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
    const float3 sumBRDF = (diffBRDF * kD + specBRDF) * dotNL;
    const float sumPDF = diffRatio * diffPDF + specRatio * specPDF;

    if (sumPDF > 0)
    {
        return float4(sumBRDF, sumPDF);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

float4 transBRDFandPDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi, in float3 H, in float etaIN, in float etaOUT, in bool isRefractSampled, in bool isFromOutside)
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
    const float3 sumBRDF = (specBRDF + refrBTDF * material.transColor.rgb) * dotNL;
    const float sumPDF = specRatio * specPDF + (1 - specRatio) * refrPDF;

    if (sumPDF > 0)
    {
        return float4(sumBRDF, sumPDF);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

void updateDirectionAndThroughput(in MaterialParams material, in float3 N_global, inout RayDesc nextRay, inout float3 throughput, in float wavelength = 0)
{
    nextRay.TMin = 0.0001;
    nextRay.TMax = 10000;
    
    const float eps = 0.001;
    const float3 wo_global = -WorldRayDirection();
    const float3 currentRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();;

    const float roulette = rand();
    const float blending = rand();
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
            L_local = HemisphereORCosineSampling(Z_AXIS, false);
        }
        else //specular
        {
            //const float3 H = GGX_ImportanceSampling(N, material.roughness);
            //const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness);
            //L_local = normalize(2.0f * dot(V_local, H_local) * H_local - V_local);

            const float a = material.roughness * material.roughness;
            float3 Vh_local = normalize(float3(a * V_local.x, a * V_local.y, V_local.z));
            float phi = 2 * PI * rand();
            float z = (1 - rand()) * (1 + Vh_local.z) - Vh_local.z;
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
        float3 brdfDevPDF = specularBRDFdevidedPDF(material, Z_AXIS, V_local, L_local);
        const float cosine = max(0, abs(L_local.z));
        throughput *= brdfDevPDF * cosine;
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
        const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness);

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
        float3 brdfDevPDF = transBRDFdevidedPDF(material, Z_AXIS, V_local, L_local, H_local, ETA_AIR, etaOUT, isRefractSampled, isFromOutside);
        const float cosine = max(0, abs(L_local.z));
        throughput *= brdfDevPDF * cosine;
    }
}

float4 bsdf_pdf(in MaterialParams material, in float3 N_global, in float3 wo_global, in float3 wi_global, in float wavelength = 0)
{
    float4 brdfAndPDF = 0.xxxx;

    const float eps = 0.001;

    const float roulette = rand();
    const float blending = rand();
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
        brdfAndPDF = specularBRDFandPDF(material, Z_AXIS, V_local, L_local);
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
        const float3 H_local = ImportanceSampling(Z_AXIS, material.roughness);

        //compute bsdf    V : wo   L : wi(sample)
        brdfAndPDF = transBRDFandPDF(material, Z_AXIS, V_local, L_local, H_local, ETA_AIR, etaOUT, true, isFromOutside);
    }

    return brdfAndPDF;
}

void NEE(inout Payload payload, in MaterialParams material, in float3 scatterPosition, in float3 surfaceNormal)
{
    LightSample sampledLight;
    bool isDirectionalLightSampled = false;
    sampleLight(scatterPosition, sampledLight, isDirectionalLightSampled);
    if (isVisible(scatterPosition, sampledLight))
    {
        float3 lightNormal = sampledLight.normal;
        float3 wi = sampledLight.directionToLight;
        float dist2 = sampledLight.distance * sampledLight.distance;
        float reseiverCos = dot(surfaceNormal, wi);
        if (reseiverCos > 0)
        {
            float emitterCos = dot(lightNormal, -wi);
            if (emitterCos > 0)
            {
                float misWeight = 1;
                float4 bsdfPDF = bsdf_pdf(material, surfaceNormal, -WorldRayDirection(), wi);

                dist2 = isDirectionalLightSampled ? 1 : dist2;
                //misWeight = isDirectionalLightSampled ? (pow(sampledLight.pdf, 2)) / (pow(bsdfPDF.w, 2) + pow(sampledLight.pdf, 2)) : (pow(bsdfPDF.w * emitterCos / dist2, 2)) / (pow(bsdfPDF.w * emitterCos / dist2, 2) + pow(sampledLight.pdf, 2));

                float G = reseiverCos * emitterCos / dist2;
                payload.color +=
                            sampledLight.emission
                            //* (isDirectionalLightSampled ? 1.xxx : bsdfPDF.xyz)
                            * saturate(bsdfPDF.xyz
                            * G) / sampledLight.pdf
                            * misWeight
                            * payload.throughput;
            }
        }
    }
}

bool isNEEExecutable(in MaterialParams material)
{
    //return (material.roughness > 0.5f) && (material.transRatio == 0) && (material.metallic == 0)  && isUseNEE();
    //return isUseNEE();
    return (material.roughness > 0.001f) && (material.transRatio == 0) && isUseNEE();
}

bool executeLighting(inout Payload payload, in MaterialParams material, in float3 scatterPosition, in float3 surfaceNormal, bool isIgnoreHit = false)
{
    bool isFinish = false;
    float3 Le = 0.xxx;
    const bool isNEE_Exec = isNEEExecutable(material);

    if (isNEE_Exec)
    {
        if (!(payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE))
        {
            payload.flags |= PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
        }
    }
    else
    {
        if (payload.flags & PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE)
        {
            payload.flags &= ~PAYLOAD_BIT_MASK_IS_PREV_NEE_EXECUTABLE;
        }
    }

    const bool isHitLightingRequired = isIndirectOnly() ? (payload.recursive > 1) : true;
    if (isHitLightingRequired)
    {
        bool isIntersect = false;
        if (payload.recursive == 1)
        {
            isIntersect = intersectAllLightWithCurrentRay(Le);
        }
        else
        {
            isIntersect = intersectLightWithCurrentRay(Le);;
        }

        //ray hitted the light source
        if (isIntersect)
        {
            if (isNEE_Exec)
            {
                if (payload.recursive == 1 && !isIndirectOnly())
                {
                    payload.color += payload.throughput * Le;
                }
            }
            else
            {
                const bool isLighting = isIndirectOnly() ? (payload.recursive > 1) : true;
                if (isLighting)
                {
                    payload.color += payload.throughput * Le;
                }
            }
            isFinish = true;
            return isFinish;
        }

        //ray hitted the emissive material
        if (length(material.emission.xyz) > 0)
        {
            payload.color += payload.throughput * material.emission.xyz;
            isFinish = false;
            return isFinish;
        }
    }

    if (isNEE_Exec && !isIgnoreHit)
    {
        const bool isNEELightingRequired = isIndirectOnly() ? (payload.recursive > 1) : true;
        if (isNEELightingRequired)
        {
            NEE(payload, material, scatterPosition, surfaceNormal);
        }
    }

    isFinish = false;
    return isFinish;
}

#endif//__SHADING_HLSLI__