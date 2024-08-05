#ifndef __SAMPLING_BSDF_HLSLI__
#define __SAMPLING_BSDF_HLSLI__

//=========================================================================
//Sampling
//=========================================================================
float3 tangentToWorld(float3 N, float3 tangentSpaceVec)
{
    float3 tangent;
    float3 bitangent;
    ONB(N, tangent, bitangent);

    return normalize(tangent * tangentSpaceVec.x + bitangent * tangentSpaceVec.y + N * tangentSpaceVec.z);
}

float3 worldToTangent(float3 N, float3 worldSpaceVec)
{
    float3 tangent;
    float3 bitangent;
    ONB(N, tangent, bitangent);

    return normalize(float3(dot(tangent, worldSpaceVec), dot(bitangent, worldSpaceVec), dot(N, worldSpaceVec)));
}

float3 HemisphereORCosineSampling(float3 N, bool isHemi, out float2 randomUV)
{
    float u = rand();
    float v = rand();
    float cosT = isHemi ? u : sqrt(u);
    float sinT = sqrt(1 - cosT * cosT);
    float P = 2 * PI * v;
    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);
    randomUV = float2(u, v);

    return tangentToWorld(N, tangentDir);
}

float3 GGX_ImportanceSampling(float3 N, float roughness)
{
    float alpha = roughness * roughness;
    float randX = rand();
    float randY = rand();

    float cosT = sqrt((1.0 - randY) / (1.0 + (alpha * alpha - 1.0) * randY));
    float sinT = sqrt(1 - cosT * cosT);
        
    float P = 2.0 * PI * randX;

    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);
    return tangentToWorld(N, tangentDir);
}

float3 ImportanceSampling(float3 N, float roughness)
{
    float u = rand();
    float v = rand();
    float a = roughness * roughness;
    float th = atan(a * sqrt(u) / sqrt(1 - u));
    float ph = 2 * PI * v;
    float st = sin(th);
    float ct = cos(th);
    float sp = sin(ph);
    float cp = cos(ph);
    float3 t, b;
    ONB(N, t, b);
    return normalize((st * cp) * t + (sp * cp) * b + ct * N);
}

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
    
    return alpha_pow2 / denom;
}

float GGX_Geometry_Schlick(float dotNX, float roughness)
{
    float alpha = roughness * roughness;
    float k = alpha / 2;
    return dotNX / (dotNX * (1 - k) + k);
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
    float eps = 0.001;

    return (D * G * F) / (4 * dotNV * dotNL + eps);
}

float3 RefractionBTDF(float D, float G, float3 F, float3 V, float3 L, float3 N, float3 H, float etaIN, float etaOUT)
{
    float dotNL = abs(dot(N, L));
    float dotNV = abs(dot(N, V));
    float dotVH = abs(dot(V, H));
    float dotLH = abs(dot(L, H));
    float eps = 0.001;
    
    float A = dotVH * dotLH / (dotNV * dotNL);
    float3 XYZ = etaOUT * etaOUT * (1 - F) * G * D;
    float B = (etaIN * dotVH + etaOUT * dotLH) * (etaIN * dotVH + etaOUT * dotLH) + eps;
    return A * XYZ / B;
}

#endif//__SAMPLING_BSDF_HLSLI__