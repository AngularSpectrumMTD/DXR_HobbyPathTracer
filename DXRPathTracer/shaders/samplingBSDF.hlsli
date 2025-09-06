#ifndef __SAMPLING_BSDF_HLSLI__
#define __SAMPLING_BSDF_HLSLI__

//=========================================================================
//Sampling
//=========================================================================

float3 GGX_ImportanceSampling(float3 N, float roughness, inout uint randomSeed)
{
    float alpha = roughness * roughness;
    float randX = rand(randomSeed);
    float randY = rand(randomSeed);

    float cosT = sqrt((1.0 - randY) / (1.0 + (alpha * alpha - 1.0) * randY));
    float sinT = sqrt(1 - cosT * cosT);
        
    float P = 2.0 * PI * randX;

    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);
    return tangentToWorld(N, tangentDir);
}

// float3 ImportanceSampling(float3 N, float roughness, inout uint randomSeed)
// {
//     float u = rand(randomSeed);
//     float v = rand(randomSeed);
//     float a = roughness * roughness;
//     float th = atan(a * sqrt(u) / sqrt(1 - u));
//     float ph = 2 * PI * v;
//     float st = sin(th);
//     float ct = cos(th);
//     float sp = sin(ph);
//     float cp = cos(ph);
//     float3 t, b;
//     ONB(N, t, b);
//     return normalize((st * cp) * t + (sp * cp) * b + ct * N);
// }

//=========================================================================
//PDF
//=========================================================================
float CosineSamplingPDF(float dotNL)
{
    return dotNL / PI;
}

float GGX_ImportanceSamplingPDF(float NDF, float dotNH, float dotVH)
{
    return NDF * dotNH / (4 * dotVH);
}

//=========================================================================
//Fresnel
//=========================================================================
float3 FresnelSchlick(float dotVH, float3 F0)
{
    return F0 + (1 - F0) * pow(1 - dotVH, 5.0);
}

float FresnelReflectance(float3 I, float3 N, float IoR)
{
    float dotNI = clamp(-1, 1, dot(N, I));
    float etaInput = 1, etaTrans = IoR;
    if (dotNI > 0)
    {
        float temp = etaInput;
        etaInput = etaTrans;
        etaTrans = temp;
    }
    
    float kr = 1;

    //Fresnel equations(Snell)
    dotNI = abs(dotNI);
    float sinTrans = etaInput / etaTrans * sqrt(max(0, 1 - dotNI * dotNI));
    if (sinTrans < 1)
    {
        float cosTrans = sqrt(max(0, 1 - sinTrans * sinTrans));
        float Rs = ((etaTrans * dotNI) - (etaInput * cosTrans)) / ((etaTrans * dotNI) + (etaInput * cosTrans));
        float Rp = ((etaInput * dotNI) - (etaTrans * cosTrans)) / ((etaInput * dotNI) + (etaTrans * cosTrans));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
    
    return kr;
}

//=========================================================================
//BRDF / BTDF    alpha = roughness * roughness
//=========================================================================
float GGX_Distribution(float3 N, float3 H, float roughness)
{
    float alpha = roughness * roughness;
    float alpha_pow2 = alpha * alpha;
    float dotNH = max(0.0, dot(N, H));
    float dotNH_pow2 = dotNH * dotNH;
    
    float denom = (dotNH_pow2 * (alpha_pow2 - 1.0) + 1.0);
    denom *= PI * denom;
    return alpha_pow2 / (denom + BSDF_EPS);
}

float GGX_Geometry_Schlick(float dotNX, float roughness)
{
    float alpha = roughness * roughness;
    float k = alpha / 2;
    return dotNX / (dotNX * (1 - k) + k + BSDF_EPS);
}

float GGX_Geometry_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float dotNV = abs(dot(N, V));
    float dotNL = abs(dot(N, L));
    return GGX_Geometry_Schlick(dotNV, roughness) * GGX_Geometry_Schlick(dotNL, roughness);
}

float3 DiffuseBRDF(float3 albedo)
{
    return albedo / PI;
}

float3 SpecularBRDF(float D, float G, float3 F, float3 V, float3 L, float3 N)
{
    float dotNL = abs(dot(N, L));
    float dotNV = abs(dot(N, V));

    return (D * G * F) / (4 * dotNV * dotNL + BSDF_EPS);
}

float3 RefractionBTDF(float D, float G, float3 F, float3 V, float3 L, float3 N, float3 H, float etaIN, float etaOUT)
{
    float dotNL = abs(dot(N, L));
    float dotNV = abs(dot(N, V));
    float dotVH = abs(dot(V, H));
    float dotLH = abs(dot(L, H));
    
    float A = dotVH * dotLH / (dotNV * dotNL);
    float3 XYZ = etaOUT * etaOUT * (1 - F) * G * D;
    float B = (etaIN * dotVH + etaOUT * dotLH) * (etaIN * dotVH + etaOUT * dotLH) + BSDF_EPS;
    return A * XYZ / B;
}

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
        return float4(sumBSDF, sumPDF + BSDF_EPS);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

float4 ForceLambertianBSDF_PDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi)
{
    if (dot(N, wi) <= 0)
    {
        return float4(0, 0, 0, 1);
    }

    const float3 H = normalize(wi + wo);

    const float dotNL = abs(dot(N, wi));

    float3 F0 = 0.08.xxx;
    F0 = lerp(F0 * material.specular, material.albedo.xyz, (material.metallic).xxx);
    
    const float3 F = FresnelSchlick(max(dot(wo, H), 0), F0);

    const float3 kS = F;
    const float3 kD = (1 - kS) * (1 - material.metallic);
    
    const float3 diffBRDF = DiffuseBRDF(material.albedo.rgb);
    const float diffPDF = CosineSamplingPDF(dotNL);
    const float3 sumBSDF = (diffBRDF * kD) * dotNL;
    const float sumPDF = diffPDF;

    if (sumPDF > 0)
    {
        return float4(sumBSDF, sumPDF + BSDF_EPS);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

float4 transmitBSDF_PDF(in MaterialParams material, in float3 N, in float3 wo, in float3 wi, in float3 H, in float etaIN, in float etaOUT)
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
        return float4(sumBSDF, sumPDF + BSDF_EPS);
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}

float4 computeBSDF_PDF(in MaterialParams material, in float3 n_global, in float3 wo_global, in float3 wi_global, inout uint randomSeed, in float wavelength = 0)
{
    float4 fp = 0.xxxx;

    const float roulette = rand(randomSeed);
    const float blending = rand(randomSeed);
    const float probability = 1 - material.transRatio;

    float3 wo_local = worldToTangent(n_global, wo_global);
    float3 wi_local = worldToTangent(n_global, wi_global);

    if (blending < probability)
    {
        if (wo_local.z < 0)
        {
            n_global *= -1;
        }

        wo_local = worldToTangent(n_global, wo_global);
        
        const float3 V_local = normalize(wo_local);
        
        //compute bsdf    V : wo   L : wi(sample)
        fp = specularBSDF_PDF(material, Z_AXIS, V_local, wi_local);
        fp.w *= probability;
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
        const float3 halfVec_local = GGX_ImportanceSampling(Z_AXIS, material.roughness, randomSeed);

        //compute bsdf    V : wo   L : wi(sample)
        fp = transmitBSDF_PDF(material, Z_AXIS, wo_local, wi_local, halfVec_local, ETA_AIR, etaOUT);
        fp.w *= (1 - probability);
    }

    return fp;
}

#endif//__SAMPLING_BSDF_HLSLI__