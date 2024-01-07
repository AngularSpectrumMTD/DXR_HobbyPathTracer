#ifndef __OPTICALFUNCTION_HLSLI__
#define __OPTICALFUNCTION_HLSLI__

#include "common.hlsli"

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

struct MaterialParams
{
    float4 albedo;
    float metallic;
    float roughness;
    float specular;
    float transRatio;
    float4 transColor;
    float4 emission;
};

static uint rseed;

float rand()//0-1
{
    rseed += 1.0;
    return frac(sin(dot(DispatchRaysIndex().xy, float2(12.9898, 78.233)) + rseed + getLightRandomSeed() * 0.01) * 43758.5453);
}

bool isPhotonStoreRequired(in MaterialParams params)
{
    return rand() < params.roughness;
}

void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
{
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    tangent = normalize(cross(up, normal));
    bitangent = cross(normal, tangent);
}

//Sampling
float3 tangentToWorld(float3 normal, float3 tangentSpaceVec)
{
    float3 tangent;
    float3 bitangent;
    ONB(normal, tangent, bitangent);

    return normalize(tangent * tangentSpaceVec.x + bitangent * tangentSpaceVec.y + normal * tangentSpaceVec.z);
}

float3 HemisphereORCosineSampling(float3 normal, bool isHemi)
{
    float cosT = isHemi ? rand() : sqrt(rand());
    float sinT = sqrt(1 - cosT * cosT);
    float P = 2 * PI * rand();
    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);

    return tangentToWorld(normal, tangentDir);
}

float3 GGX_ImportanceSampling(float3 N, float roughness)
{
    float alpha = roughness * roughness;
    float randX = randXorshift();
    float randY = randXorshift();

    float cosT = sqrt((1.0 - randY) / (1.0 + (alpha * alpha - 1.0) * randY));
    float sinT = sqrt(1 - cosT * cosT);
        
    float P = 2.0 * PI * randX;

    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);
    return tangentToWorld(N, tangentDir);
}

//PDF
float CosineSamplingPDF(float dotNL)
{
    return dotNL / PI;
}

float GGX_ImportanceSamplingPDF(float NDF, float dotNH, float dotVH)
{
    return NDF * dotNH / (4 * dotVH);
}

//Fresnel
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

//BRDF / BTDF
//alpha = roughness * roughness

//alpha^2
//---------
//pi * ((dot(N, H))^2 * (alpha^2 - 1) + 1)^2
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

//k = alpha / 2
//dot(N, V)
//---------
//dot(N, V) * (1 - k) + k
float GGX_Geometry_Schlick(float dotNX, float roughness)
{
    float alpha = roughness * roughness;
    float k = alpha / 2;
    return dotNX / (dotNX * (1 - k) + k);
}

//GGX_Geometry_Schlick(V) * GGX_Geometry_Schlick(L)
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

//Lighting
struct LightSample
{
    float3 direction;
    float3 normal;
    float3 emission;
    //float3 sufacePosition;
    float distance;
    float pdf;
};

void SampleSphereLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float eps = 0.001f;
    float3 vectorToScatterPosition = scatterPosition - lightGen.position;
    float distToScatterPosition = length(vectorToScatterPosition);
    
    float3 sampledDir = HemisphereORCosineSampling(normalize(vectorToScatterPosition), true);
    float3 lightSurfacePos = lightGen.position + sampledDir * lightGen.sphereRadius;
    
    //lightSample.sufacePosition = lightSurfacePos;
    lightSample.direction = lightSurfacePos - scatterPosition;
    lightSample.distance = length(lightSample.direction);
    const float distanceSq = lightSample.distance * lightSample.distance + eps;

    lightSample.direction /= lightSample.distance;
    lightSample.normal = normalize(lightSurfacePos - lightGen.position);
    lightSample.emission = lightGen.emission * getLightNum();
    lightSample.pdf = distanceSq / (lightGen.influenceDistance * 0.5 * abs(dot(lightSample.normal, lightSample.direction)));
}

void SampleRectLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float eps = 0.001f;
    float rnd0 = (rand() * 0.5 - 1) * 2;//-1 to 1
    float rnd1 = (rand() * 0.5 - 1) * 2; //-1 to 1

    float3 lightSurfacePos = lightGen.position + rnd0 * lightGen.U + rnd1 * lightGen.V;
    
    //lightSample.sufacePosition = lightSurfacePos;
    lightSample.direction = lightSurfacePos - scatterPosition;
    lightSample.distance = length(lightSample.direction);
    const float distanceSq = lightSample.distance * lightSample.distance + eps;

    lightSample.direction /= lightSample.distance;
    lightSample.normal = normalize(cross(lightGen.U, lightGen.V));
    lightSample.emission = lightGen.emission * getLightNum();
    lightSample.pdf = distanceSq / (lightGen.influenceDistance * abs(dot(lightSample.normal, lightSample.direction)));
}

void SampleSpotLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float eps = 0.001f;
    float rnd0 = rand(); //0 to 1
    float rnd1 = rand(); //0 to 1
    float r = lengthSqr(lightGen.U) * sqrt(rnd0);
    float p = 2 * PI * rnd1;

    float3 localXYZ = float3(r * cos(p), lengthSqr(lightGen.V) / lengthSqr(lightGen.U) * r * sin(p), 0);
    float3 worldXYZ = lightGen.U * localXYZ.x + lightGen.V * localXYZ.y;
    
    float pointToSpot = 0.001f;
    float spotLightHalfAngle = atan2(length(lightGen.U), pointToSpot);
    float saturatedCosLight = saturate(cos(spotLightHalfAngle));
    float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    float saturatedCosScatter = saturate(dot(dominantDir, normalize(scatterPosition - lightGen.position)));
    float coef = saturate(saturatedCosScatter - saturatedCosLight) / (1 - saturatedCosLight);

    float3 lightSurfacePos = lightGen.position + worldXYZ;
    lightSample.direction = lightSurfacePos - scatterPosition;
    lightSample.distance = length(lightSample.direction);
    const float distanceSq = lightSample.distance * lightSample.distance + eps;

    lightSample.direction /= lightSample.distance;
    lightSample.normal = dominantDir;
    lightSample.emission = isVisualizeLightRange() ? 0.xxx : lightGen.emission * getLightNum() * coef;
    lightSample.pdf = distanceSq / (lightGen.influenceDistance);
}

void SampleDirectionalLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    //lightSample.sufacePosition = 0.xxx;
    lightSample.direction = normalize(-lightGen.position);//pos as dir
    lightSample.normal = normalize(lightGen.position);
    lightSample.emission = lightGen.emission * getLightNum();
    lightSample.distance = 10000000;
    lightSample.pdf = 1.0;
}

void SampleLight(in float3 scatterPosition, inout LightSample lightSample)
{
    const uint sampleID = (uint) (getLightRandomSeed()) % getLightNum();
    LightGenerateParam param = gLightGenerateParams[sampleID];
    
    if (param.type == LIGHT_TYPE_SPHERE)
    {
        SampleSphereLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_RECT)
    {
        SampleRectLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_SPOT)
    {
        SampleSpotLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_DIRECTIONAL)
    {
        SampleDirectionalLight(param, scatterPosition, lightSample);
    }
}

bool isShadow(in float3 scatterPosition, in LightSample lightSample)
{
    Payload shadowPayload;
    shadowPayload.isShadowRay = 1;
    shadowPayload.isShadowMiss = 0;

    const float eps = 0.001;
    RayDesc shadowRay;
    shadowRay.TMin = eps;
    shadowRay.TMax = lightSample.distance;

    shadowRay.Direction = lightSample.direction;
    shadowRay.Origin = scatterPosition;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK);

    TraceRay(
            gRtScene,
            flags,
            rayMask,
            0, // ray index
            1, // MultiplierForGeometryContrib
            0, // miss index
            shadowRay,
            shadowPayload);

    return shadowPayload.isShadowMiss == 0;
}

//Shading
void SurafceShading(in MaterialParams material, in float3 N, inout RayDesc nextRay, inout float3 throughput, float lambda = 0)
{
    nextRay.TMin = 0.001;
    nextRay.TMax = 10000;
    
    const float eps = 0.001;
    const float3 currentRayDir = WorldRayDirection();
    const float3 currentRayOrigin = nextRay.Origin;

    const float roulette = rand();
    const float blending = rand();

    if (blending < 1 - material.transRatio)
    {
        float3 reflectDir = 0.xxx;
        
        const float diffRatio = 1.0 - material.metallic;
        const float specRatio = 1 - diffRatio;

        const float3 V = normalize(-currentRayDir);
        
        if (roulette < diffRatio)//diffuse
        {
            reflectDir = HemisphereORCosineSampling(N, false);
        }
        else//specular
        {
            const float3 halfVec = GGX_ImportanceSampling(N, material.roughness);
            reflectDir = normalize(2.0f * dot(V, halfVec) * halfVec - V);
        }
        
        const float3 L = normalize(reflectDir);
        const float3 H = normalize(L + V);

        const float dotNL = abs(dot(N, L));
        const float dotNH = abs(dot(N, H));
        const float dotVH= abs(dot(V, H));

        float3 F0 = 0.08.xxx;
        F0 = lerp(F0 * material.specular, material.albedo.xyz, (material.metallic).xxx);
        //F0 = lerp(F0 * material.specular, 1.xxx, (material.metallic).xxx);//No Albedo
        
        const float NDF = GGX_Distribution(N, H, material.roughness);
        const float G = GGX_Geometry_Smith(N, V, L, material.roughness);
        const float3 F = FresnelSchlick(max(dot(V, H), 0), F0);

        const float3 kS = F;
        const float3 kD = (1 - kS) * (1 - material.metallic);
        
        const float3 specBRDF = SpecularBRDF(NDF, G, F, V, L, N);
        const float specPDF = GGX_ImportanceSamplingPDF(NDF, dotNH, dotVH);
        const float3 diffBRDF = DiffuseBRDF(material.albedo.rgb);
        //const float3 diffBRDF = DiffuseBRDF(1.xxx); //No Albedo
        const float diffPDF = CosineSamplingPDF(dotNL);
        const float3 sumBRDF = (diffBRDF * kD + specBRDF) * dotNL;
        const float sumPDF = diffRatio * diffPDF + specRatio * specPDF;
        
        nextRay.Origin = currentRayOrigin + N * eps;
        nextRay.Direction = reflectDir;

        if (sumPDF > 0)
        {
            throughput *= sumBRDF / sumPDF;
        }
    }
    else
    {
        bool isFromOutside = dot(currentRayDir, N) < 0;
        N *= isFromOutside ? 1 : -1;
        
        float etaIN = 1;
        float etaOUT = 1.7;

        if (lambda > 0)
        {
            etaOUT = J_Bak4.computeRefIndex(lambda * 1e-3);
        }

        float3 V = normalize(-currentRayDir);
        float3 H = GGX_ImportanceSampling(N, material.roughness);

        float3 F0 = 0.08.xxx * material.specular;
        float3 F = FresnelSchlick(max(dot(H, V), 0), F0);

        const float kR = FresnelReflectance(currentRayDir, N, etaOUT);
        
        const float specRatio = kR;
        const float refrRatio = 1 - kR;

        float3 L = 0.xxx;

        if (roulette <= specRatio)
        {
            nextRay.Origin = currentRayOrigin + N * eps;
            L = reflect(currentRayDir, H);
            nextRay.Direction = L;
        }
        else
        {
            float eta = isFromOutside ? etaIN / etaOUT : etaOUT / etaIN;
            float3 refractVec = refract(currentRayDir, H, eta);
            if (length(refractVec) < 0.00001f)
            {
                L = reflect(currentRayDir, H);//handle as total reflection
            }
            else
            {
                L = normalize(refractVec);
            }
            nextRay.Direction = L;
            nextRay.Origin = currentRayOrigin - N * eps;
            if (!isFromOutside)
            {
                float3 tmp = L;
                L = V;
                V = tmp;
                N *= -1;
                H *= -1;
            }
        }

        const float dotNL = abs(dot(N, L));
        const float dotNV = abs(dot(N, V));
        const float dotNH = abs(dot(N, H));
        const float dotVH = abs(dot(V, H));
        const float dotLH = abs(dot(L, H));

        float NDF = GGX_Distribution(N, H, material.roughness);
        float G = GGX_Geometry_Smith(N, V, L, material.roughness);

        float3 specBRDF = SpecularBRDF(NDF, G, F, V, L, N);
        float specPDF = GGX_ImportanceSamplingPDF(NDF, dotNH, dotVH);
        float3 refrBTDF = RefractionBTDF(NDF, G, F, V, L, N, H, etaIN, etaOUT);
        float refrPDF = GGX_ImportanceSamplingPDF(NDF, dotNH, dotVH);
        const float3 sumBRDF = (specBRDF + refrBTDF * material.transColor.rgb) * dotNL;
        const float sumPDF = specRatio * specPDF + refrRatio * refrPDF;
        
        if (sumPDF > 0)
        {
            throughput *= sumBRDF / sumPDF;
        }
    }
}

//Spectral Rendering Helper
#define REFLECTANCE_BOOST 4

#define LANBDA_INF_NM 770
#define LAMBDA_VIO_NM 380
#define LAMBDA_NUM 40
#define LAMBDA_STEP 10

float luminanceFromRGB(const float3 rgb)
{
    return 0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b;
}

//http://nalab.mind.meiji.ac.jp/2017/2018-suzuki.pdf
static float3 XYZ380to770_10nmTbl[LAMBDA_NUM] =
{
    float3(0.0014, 0, 0.0065),
    float3(0.0042, 0.0001, 0.0201),
    float3(0.0143, 0.0004, 0.0679),
    float3(0.0435, 0.0012, 0.2074),
    float3(0.1344, 0.004, 0.6456),
    float3(0.2839, 0.0116, 1.3856),
    float3(0.3483, 0.023, 1.7471),
    float3(0.3362, 0.038, 1.7721),
    float3(0.2908, 0.06, 1.6692),
    float3(0.1954, 0.091, 1.2876),
    float3(0.0956, 0.139, 0.813),
    float3(0.032, 0.208, 0.4652),
    float3(0.0049, 0.323, 0.272),
    float3(0.0093, 0.503, 0.1582),
    float3(0.0633, 0.71, 0.0782),
    float3(0.1655, 0.862, 0.0422),
    float3(0.294, 0.954, 0.0203),
    float3(0.4334, 0.995, 0.0087),
    float3(0.5945, 0.995, 0.0039),
    float3(0.7621, 0.952, 0.0021),
    float3(0.9163, 0.87, 0.0017),
    float3(1.0263, 0.757, 0.0011),
    float3(1.0622, 0.631, 0.0008),
    float3(1.0026, 0.503, 0.0003),
    float3(0.8544, 0.381, 0.0002),
    float3(0.6424, 0.265, 0),
    float3(0.4479, 0.175, 0),
    float3(0.2835, 0.107, 0),
    float3(0.1649, 0.061, 0),
    float3(0.0874, 0.032, 0),
    float3(0.0468, 0.017, 0),
    float3(0.0227, 0.0082, 0),
    float3(0.0114, 0.0041, 0),
    float3(0.0058, 0.0021, 0),
    float3(0.0029, 0.001, 0),
    float3(0.0014, 0.0005, 0),
    float3(0.0007, 0.0003, 0),
    float3(0.0003, 0.0001, 0),
    float3(0.0002, 0.0001, 0),
    float3(0.0001, 0, 0)
};
static float Rmin = -0.925239563;
static float Gmin = -0.221841574;
static float Bmin = -0.169706330;

static float Rmax = 2.47556806;
static float Gmax = 1.50557864;
static float Bmax = 1.88461602;

float3 lambda2XYZ(float lambdaNM)
{
    float fid = clamp((lambdaNM - LAMBDA_VIO_NM) / LAMBDA_STEP, 0, LAMBDA_NUM);
    int baseID = int(fid + 0.5);
    float t = fid - baseID;
    float3 XYZ0 = XYZ380to770_10nmTbl[baseID];
    float3 XYZ1 = XYZ380to770_10nmTbl[baseID + 1];
    return lerp(XYZ0, XYZ1, t);
}

float gamma(float val)
{
    if (val <= 0.0031308)
    {
        return val * 12.92;
    }
    else
    {
        return pow(val, 1.0 / 2.4) * 1.055 - 0.055;
    }
}

static float3x3 XYZtoRGB = 
{
    + 3.240479, - 1.537150, - 0.498535,
    - 0.969256, + 1.875992, + 0.041556,
    + 0.055648, - 0.204043, + 1.057311
};

static float3x3 XYZtoRGB2 = 
{
    + 2.3706743, - 0.51388850, + 0.0052982,
    - 0.9000405, + 1.4253036, - 0.0146949,
    - 0.470638, + 0.0885814, + 1.0093968
};

float3 integralColor()
{
    int i = 0;
    float3 XYZ = 0.xxx;
    for(i = 0; i < LAMBDA_NUM; i++)
    {
        XYZ += XYZ380to770_10nmTbl[i];
    }

    return mul(XYZ, XYZtoRGB2);
}

float3 lambda2sRGB_D65_BT709(float lambdaNM)
{
    float3 XYZ = lambda2XYZ(lambdaNM);

    float3 RGB = mul(XYZtoRGB, XYZ);

    RGB.r = gamma((RGB.r - Rmin) / (Rmax - Rmin));
    RGB.g = gamma((RGB.g - Gmin) / (Gmax - Gmin));
    RGB.b = gamma((RGB.b - Bmin) / (Bmax - Bmin));

    return RGB;
}

float3 lambda2RGB(float lambda)
{
    float b = 0.2 * (lerp(1, 1, (lambda - 380) / (490 - 380)) + lerp(1, 0, (lambda - 490) / (510 - 490)));
    if (lambda == 490)
        b *= 0.5;
    float g = 0.3 * (lerp(0, 1, (lambda - 440) / (490 - 440)) + lerp(1, 1, (lambda - 490) / (580 - 490)) + lerp(1, 0, (lambda - 580) / (645 - 580)));
    if (lambda == 490 || lambda == 580)
        g *= 0.5;
    float r = lerp(0, 1, (lambda - 350) / (440 - 350)) + lerp(0, 1, (lambda - 510) / (580 - 510)) + lerp(1, 0, (lambda - 580) / (700 - 580));
    if (lambda == 580)
        r *= 0.5;

    return 0.3 * float3(r, g, b);
}

float3 lambda2RGB_Fluorescent(float lambda)
{
    float b = lerp(0, 1, (lambda - 380) / (460 - 400)) + lerp(1, 0, (lambda - 460) / (500 - 460));
    if (lambda == 460)
        b *= 0.5;
    float g = 0.6 * (lerp(0, 1, (lambda - 460) / (500 - 460)) + lerp(1, 0, (lambda - 500) / (530 - 500)));
    if (lambda == 500)
        g *= 0.5;
    float r = 0.5 * (lerp(0, 1, (lambda - 520) / (580 - 520)) + lerp(1, 0, (lambda - 580) / (700 - 580)));
    if (lambda == 580)
        r *= 0.5;

    return 0.3 * float3(r, g, b);
}

float3 lambda2RGB_IncandescentBulb(float lambda)
{
    float b = 0.1 * (lerp(0, 1, (lambda - 380) / (460 - 400)) + lerp(1, 0, (lambda - 460) / (500 - 460)));
    if (lambda == 460)
        b *= 0.5;
    float g = 0.3 * (lerp(0, 1, (lambda - 460) / (530 - 460)) + lerp(1, 0, (lambda - 530) / (560 - 530)));
    if (lambda == 530)
        g *= 0.5;
    float r = lerp(0, 1, (lambda - 500) / (780 - 500));

    return 0.6 * float3(r, g, b);
}

float3 getBaseLightColor(float lambda)
{
    float3 color = 0.xxx;
    switch(getSpectrumMode())
    {
         case 0:
        color = lambda2sRGB_D65_BT709(lambda);
        break;
         case 1:
        color = lambda2RGB(lambda);
        break;
         case 2:
        color = lambda2RGB_Fluorescent(lambda);
        break;
         case 3:
        color = lambda2RGB_IncandescentBulb(lambda);
        break;
         default:
        color = 0.xxx;
        break;
    }
    return color;
}

float3 getBaseLightXYZ(float lambda)
{
    return lambda2XYZ(lambda);
}
#endif//__OPTICALFUNCTION_HLSLI__