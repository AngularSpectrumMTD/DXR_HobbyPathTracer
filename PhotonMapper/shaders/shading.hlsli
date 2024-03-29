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

void updateDirectionAndThroughput(in MaterialParams material, float3 N, inout RayDesc nextRay, inout float3 throughput, in float wavelength = 0)
{
    const bool isPathTrace = (wavelength == 0);
    nextRay.TMin = 0.0001;
    nextRay.TMax = 10000;
    
    const float eps = 0.001;
    const float3 currentRayDir = WorldRayDirection();
    const float3 currentRayOrigin = nextRay.Origin;

    const float roulette = rand();
    const float blending = rand();
    const float probability = 1 - material.transRatio;

    if (blending < probability)
    {
        if (dot(currentRayDir, N) > 0)
        {
            N *= -1;
        }

        //sample direction
        float3 L = 0.xxx;
        
        const float diffRatio = 1.0 - material.metallic;
        const float3 V = normalize(-currentRayDir);
        
        if (roulette < diffRatio)//diffuse
        {
            L = HemisphereORCosineSampling(N, false);
        }
        else //specular
        {
            //const float3 halfVec = GGX_ImportanceSampling(N, material.roughness);
            float u = rand();
            float v = rand();
            float a = material.roughness * material.roughness;
            float th = atan(a * sqrt(u) / sqrt(1 - u));
            float ph = 2 * PI * v;
            float st = sin(th);
            float ct = cos(th);
            float sp = sin(ph);
            float cp = cos(ph);
            float3 t, b;
            ONB(N, t, b);
            float3 h = (st * cp) * t + (sp * cp) * b + ct * N;
            L = normalize(2.0f * dot(V, h) * h - V);
        }

        nextRay.Origin = currentRayOrigin + N * eps;
        nextRay.Direction = L;
        
        //compute bsdf    V : wo   L : wi(sample)
        float3 brdfDevPDF = specularBRDFdevidedPDF(material, N, V, L);
        const float cosine = max(0, dot(N, nextRay.Direction));
        throughput *= brdfDevPDF * cosine;// / probability;
    }
    else
    {
        //sample direction
        bool isFromOutside = dot(currentRayDir, N) < 0;

        //change the normal direction to the incident direction side
        if (dot(currentRayDir, N) > 0)
        {
            N *= -1;
        }
        
        const float etaOUT = (wavelength > 0) ? J_Bak4.computeRefIndex(wavelength * 1e-3) : 1.7;

        float3 V = normalize(-currentRayDir);
        //float3 H = GGX_ImportanceSampling(N, material.roughness);

        float u = rand();
        float v = rand();
        float a = material.roughness * material.roughness;
        float th = atan(a * sqrt(u) / sqrt(1 - u));
        float ph = 2 * PI * v;
        float st = sin(th);
        float ct = cos(th);
        float sp = sin(ph);
        float cp = cos(ph);
        float3 t, b;
        ONB(N, t, b);
        float3 h = (st * cp) * t + (sp * cp) * b + ct * N;
        float3 H = normalize(h);
        
        const float specRatio = FresnelReflectance(currentRayDir, N, etaOUT);

        float3 L = 0.xxx;

        bool isRefractSampled = true;
        {
            float eta = isFromOutside ? ETA_AIR / etaOUT : etaOUT / ETA_AIR;
            float3 refractVec = refract(currentRayDir, H, eta);
            if (length(refractVec) < 0.001f)
            {
                L = reflect(currentRayDir, H); //handle as total reflection
                isRefractSampled = false;
            }
            else
            {
                L = normalize(refractVec);
                isRefractSampled = true;
            }
            nextRay.Origin = currentRayOrigin - N * eps;
            nextRay.Direction = L;
        }

        //compute bsdf    V : wo   L : wi(sample)
        float3 brdfDevPDF = transBRDFdevidedPDF(material, N, V, L, H, ETA_AIR, etaOUT, isRefractSampled, isFromOutside);
        const float cosine = abs(dot(N, nextRay.Direction));
        throughput *= brdfDevPDF * cosine;// / (1 - probability);
    }
}

#endif//__SHADING_HLSLI__