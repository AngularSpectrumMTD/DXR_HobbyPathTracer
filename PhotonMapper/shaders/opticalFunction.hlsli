#ifndef __OPTICALFUNCTION_HLSLI__
#define __OPTICALFUNCTION_HLSLI__

#include "common.hlsli"

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

struct Reservoir
{
    uint Y;//index of most important light
    float W_y;//weight of light
    float W_sum;//sum of all weight
    float M; //number of ligts processed for this reservoir

    void initialize()
    {
        Y = 0;
        W_y = 0;
        W_sum = 0;
        M = 0;
    }
};

bool updateReservoir(inout Reservoir reservoir, in uint X, in float w, in uint c, in float rnd01)
{
    reservoir.W_sum += w;
    reservoir.M += c;

    if (rnd01 < w / reservoir.W_sum)
    {
        reservoir.Y = X;
        return true;
    }
    return false;
}

static uint rseed;

float rand()//0-1
{
    rseed += 1.0;
    return frac(sin(dot(DispatchRaysIndex().xy, float2(12.9898, 78.233)) + rseed + getLightRandomSeed() * 0.01) * 43758.5453);
}

bool isPhotonStoreRequired(in MaterialParams params)
{
    return 0.3 < params.roughness;
}

void ONB(in float3 normal, out float3 tangent, out float3 bitangent)
{
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    tangent = normalize(cross(up, normal));
    bitangent = cross(normal, tangent);
}

//=========================================================================
//Geometry Intersection
//=========================================================================
#define RAY_MAX_T 1000000

float3x3 constructWorldToLocalMatrix(float3 forwardDir, float3 upDir)
{
    float3 rightDir = cross(forwardDir, upDir);
    float3x3 result = { rightDir, upDir, forwardDir };
    return result;
}

//ax^2+bx+c = 0 , x = (-b +- sqrt(d)) / 2a
float2 quadraticFormula(float a, float b, float c)//x : (-b - sqrt(d)) / 2a , y : (-b + sqrt(d)) / 2a
{
    float discriminant = b * b - 4 * a * c;
    if (discriminant >= 0)
    {
        float s = sqrt(discriminant);
    
        float minDst = (-b - s) / (2 * a);
        float maxDst = (-b + s) / (2 * a);
    
        return float2(minDst, maxDst);
    }
    
    return float2(RAY_MAX_T, RAY_MAX_T);
}

float2 intersectEllipsoid(float3 lineOrigin, float3 lineDir, float3 shapeOrigin, float3 shapeForwardDir, float3 shapeUpDir, float u, float v, float w)
{
    float3x3 transMat = constructWorldToLocalMatrix(shapeForwardDir, shapeUpDir);
    float3 orig = mul(transMat, lineOrigin - shapeOrigin);
    float3 dir = mul(transMat, lineDir);
    
    float U = 1.0 / (u * u);
    float V = 1.0 / (v * v);
    float W = 1.0 / (w * w);
    
    float a = dir.x * dir.x * U + dir.y * dir.y * V + dir.z * dir.z * W;
    float b = 2 * (orig.x * dir.x * U + orig.y * dir.y * V + orig.z * dir.z * W);
    float c = orig.x * orig.x * U + orig.y * orig.y * V + orig.z * orig.z * W - 1;
    
    return quadraticFormula(a, b, c);
} 

//u v : length of axis
float intersectEllipse(float3 lineOrigin, float3 lineDir, float3 shapeOrigin, float3 vecU, float3 vecV)
{
    float3x3 transMat = constructWorldToLocalMatrix(normalize(vecV), normalize(cross(vecU, vecV)));
    float3 orig = mul(transMat, lineOrigin - shapeOrigin);
    float3 dir = mul(transMat, lineDir);
    
    float denom = dir.y;
    float num = orig.y;
    
    const float T = -num / denom;
    const float u = length(vecU);
    const float v = length(vecV);
    float3 samplePos = orig + dir * T;
    float judgeValue = samplePos.x * samplePos.x / (u * u) + samplePos.z * samplePos.z / (v * v);
    bool isInEllipse = (judgeValue <= 1);
    return isInEllipse ? T : -1;
}

float intersectRectangle(float3 lineOrigin, float3 lineDir, float3 shapeOrigin, float3 vecU, float3 vecV)
{
    float3x3 transMat = constructWorldToLocalMatrix(normalize(vecV), normalize(cross(vecU, vecV)));
    float3 orig = mul(transMat, lineOrigin - shapeOrigin);
    float3 dir = mul(transMat, lineDir);
    
    float denom = dir.y;
    float num = orig.y;
    
    const float T = -num / denom;
    float3 samplePos = orig + dir * T;
    bool isInRectangle = (abs(samplePos.x) <= length(vecU) && abs(samplePos.z) <= length(vecV));
    return isInRectangle ? T : -1;
}

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

float3 HemisphereORCosineSampling(float3 N, bool isHemi)
{
    float cosT = isHemi ? rand() : sqrt(rand());
    float sinT = sqrt(1 - cosT * cosT);
    float P = 2 * PI * rand();
    float3 tangentDir = float3(cos(P) * sinT, sin(P) * sinT, cosT);

    return tangentToWorld(N, tangentDir);
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

//=========================================================================
//Lighting
//=========================================================================
struct LightSample
{
    float3 direction; //from scatterPos to lightPos;
    float3 normal;
    float3 emission;
    //float3 sufacePosition;
    float distance;
    float pdf;
};

void sampleSphereLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float eps = 0.001f;
    float3 vectorToScatterPosition = scatterPosition - lightGen.position;
    float distToScatterPosition = length(vectorToScatterPosition);
    
    float3 sampledDir = HemisphereORCosineSampling(normalize(vectorToScatterPosition), true);
    float3 lightSurfacePos = lightGen.position + sampledDir * lightGen.sphereRadius;
    
    lightSample.direction = lightSurfacePos - scatterPosition;
    lightSample.distance = length(lightSample.direction);
    const float distanceSq = lightSample.distance * lightSample.distance + eps;

    lightSample.direction /= lightSample.distance;
    lightSample.normal = normalize(lightSurfacePos - lightGen.position);
    const float coef = 0.5 * abs(dot(lightSample.normal, lightSample.direction));
    const float cosine = max(0, dot(lightSample.normal, -lightSample.direction));
    lightSample.emission = lightGen.emission * lightGen.influenceDistance * coef * cosine / distanceSq;
    lightSample.pdf = 1;
}

void sampleRectLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float eps = 0.001f;
    float rnd0 = (rand() * 0.5 - 1) * 2;//-1 to 1
    float rnd1 = (rand() * 0.5 - 1) * 2; //-1 to 1

    float3 lightSurfacePos = lightGen.position + rnd0 * lightGen.U + rnd1 * lightGen.V;
    
    lightSample.direction = lightSurfacePos - scatterPosition;
    lightSample.distance = length(lightSample.direction);
    const float distanceSq = lightSample.distance * lightSample.distance + eps;

    const float3 originToScatterVec = scatterPosition - lightGen.position;
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));

    const float rectLightHalfAngleU = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    const float saturatedCosLightU = saturate(cos(rectLightHalfAngleU));
    const float rectLightHalfAngleV = atan2(length(lightGen.V), LIGHT_BASE_LENGTH);
    const float saturatedCosLightV = saturate(cos(rectLightHalfAngleV));

    const float3 computePlaneCenterVec = saturate(dot(dominantDir, normalize(originToScatterVec))) * length(originToScatterVec) * dominantDir;
    const float3 vecOnPlane = originToScatterVec - computePlaneCenterVec;
    float3 originToScatterVecU = normalize(computePlaneCenterVec + dot(vecOnPlane, normalize(lightGen.U)) * normalize(lightGen.U));
    float3 originToScatterVecV = normalize(computePlaneCenterVec + dot(vecOnPlane, normalize(lightGen.V)) * normalize(lightGen.V));
    const float saturatedCosScatterU = saturate(dot(dominantDir, originToScatterVecU));
    const float saturatedCosScatterV = saturate(dot(dominantDir, originToScatterVecV));
    const float coefU = saturate(saturatedCosScatterU - saturatedCosLightU) / (1 - saturatedCosLightU);
    const float coefV = saturate(saturatedCosScatterV - saturatedCosLightV) / (1 - saturatedCosLightV);

    lightSample.direction /= lightSample.distance;
    lightSample.normal = normalize(cross(lightGen.U, lightGen.V));
    const float coef = coefU * coefV;
    const float cosine = max(0, dot(lightSample.normal, -lightSample.direction));
    lightSample.emission = lightGen.emission * lightGen.influenceDistance * coef * cosine / distanceSq;
    lightSample.pdf = 1;
}

void sampleSpotLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float eps = 0.001f;
    float rnd0 = rand(); //0 to 1
    float rnd1 = rand(); //0 to 1
    float r = lengthSqr(lightGen.U) * sqrt(rnd0);
    float p = 2 * PI * rnd1;

    float3 localXYZ = float3(r * cos(p), lengthSqr(lightGen.V) / lengthSqr(lightGen.U) * r * sin(p), 0);
    float3 worldXYZ = lightGen.U * localXYZ.x + lightGen.V * localXYZ.y;
    
    const float spotLightHalfAngle = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    const float saturatedCosLight = saturate(cos(spotLightHalfAngle));
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    const float saturatedCosScatter = saturate(dot(dominantDir, normalize(scatterPosition - lightGen.position)));
    const float coef = saturate(saturatedCosScatter - saturatedCosLight) / (1 - saturatedCosLight);

    float3 lightSurfacePos = lightGen.position + worldXYZ;
    lightSample.direction = lightSurfacePos - scatterPosition;
    lightSample.distance = length(lightSample.direction);
    const float distanceSq = lightSample.distance * lightSample.distance + eps;

    lightSample.direction /= lightSample.distance;
    lightSample.normal = dominantDir;
    const float cosine = max(0, dot(lightSample.normal, -lightSample.direction));
    lightSample.emission = lightGen.emission * lightGen.influenceDistance * coef * cosine / distanceSq;
    lightSample.pdf = 1;
}

void sampleDirectionalLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    lightSample.direction = normalize(-lightGen.position);//pos as dir
    lightSample.normal = normalize(lightGen.position);
    const float valid = (dot(lightSample.normal, lightSample.direction) < 0) ? 1 : 0;
    lightSample.emission = lightGen.emission * valid;
    lightSample.distance = 10000000;
    lightSample.pdf = 1;
}

void sampleSphereLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position)
{
    emitDir = HemisphereORCosineSampling(float3(1, 0, 0), false);
    position = lightGen.position;
}

void sampleRectLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position)
{
    float rnd0 = (pcgHashState() - 0.5) * 2; //-1 to 1
    float rnd1 = (pcgHashState() - 0.5) * 2; //-1 to 1
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    emitDir = normalize(dominantDir * LIGHT_BASE_LENGTH + rnd0 * lightGen.U + rnd1 * lightGen.V);
    position = lightGen.position;
}

void sampleSpotLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position)
{
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    const float spotLightHalfAngle = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    emitDir = getConeSample(pcgHashState(), dominantDir, spotLightHalfAngle);
    position = lightGen.position;
}

void sampleDirectionalLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position)
{
    float rnd0 = (pcgHashState() - 0.5) * 2; //-1 to 1
    float rnd1 = (pcgHashState() - 0.5) * 2; //-1 to 1
    emitDir = normalize(lightGen.position); //pos as dir
    float3 tangent;
    float3 bitangent;
    ONB(emitDir, tangent, bitangent);
    position = 1000 * -emitDir + tangent * 1000 * rnd0 + bitangent * 1000 * rnd1;
}

void sampleLight(in float3 scatterPosition, inout LightSample lightSample)
{
    const uint lightID = (uint) (rand() * (getLightNum()));
    LightGenerateParam param = gLightGenerateParams[lightID];
    
    if (param.type == LIGHT_TYPE_SPHERE)
    {
        sampleSphereLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_RECT)
    {
        sampleRectLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_SPOT)
    {
        sampleSpotLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_DIRECTIONAL)
    {
        sampleDirectionalLight(param, scatterPosition, lightSample);
    }

    lightSample.pdf *= 1.0f / getLightNum();
}

bool intersectLightWithCurrentRay(out float3 Le)
{
    const float3 rayOrigin = WorldRayOrigin();
    const float3 rayDiretion = WorldRayDirection();
    const float rayT = RayTCurrent();

    const uint lightID = (uint) (rand() * (getLightNum()));
    LightGenerateParam param = gLightGenerateParams[lightID];

    if (param.type == LIGHT_TYPE_SPHERE)
    {
        float2 tt = intersectEllipsoid(rayOrigin, rayDiretion, param.position,
        normalize(param.U), normalize(param.V), param.sphereRadius, param.sphereRadius, param.sphereRadius);
        float hittedT = min(tt.x, tt.y);

        Le = param.emission;
        return (hittedT > 0 && hittedT < rayT);
    }
    else if (param.type == LIGHT_TYPE_RECT)
    {
        const float3 shapeForwardDir = normalize(cross(param.U, param.V));
        float hittedT = intersectRectangle(rayOrigin, rayDiretion, param.position, param.U, param.V);
        const bool isFrontHit = (length(shapeForwardDir) > 0) && (dot(shapeForwardDir, -rayDiretion) > 0);

        Le = param.emission;
        return isFrontHit && (hittedT > 0 && hittedT < rayT);
    }
    else if (param.type == LIGHT_TYPE_SPOT)
    {
        const float3 shapeForwardDir = normalize(cross(param.U, param.V));
        float hittedT = intersectEllipse(rayOrigin, rayDiretion, param.position, param.U, param.V);
        const bool isFrontHit = (length(shapeForwardDir) > 0) && (dot(shapeForwardDir, -rayDiretion) > 0);

        Le = param.emission;
        return isFrontHit && (hittedT > 0 && hittedT < rayT);
    }

    return false;
}

float3 directionalLightingOnMissShader(Payload payload)
{
    float3 val = 0.xxx;

    float3 directionalDir = 0.xxx;
    bool isDirectionalLightFinded = false;
    float3 emis = 0.xxx;
    for (int i = 0; i < getLightNum(); i++)
    {
        LightGenerateParam param = gLightGenerateParams[i];

        if (isDirectionalLightFinded == false && param.type == LIGHT_TYPE_DIRECTIONAL)
        {
            isDirectionalLightFinded = true;
            directionalDir = normalize(param.position);
            emis = param.emission;
        }
    }

    if (payload.recursive > 0 && isDirectionalLightFinded && dot(WorldRayDirection(), -directionalDir) > 0)
    {
        val = payload.throughput * emis * dot(WorldRayDirection(), -directionalDir);
    }

    return val;
}

void sampleLightWithID(in float3 scatterPosition, in int ID, inout LightSample lightSample)
{
    LightGenerateParam param = gLightGenerateParams[ID];
    
    if (param.type == LIGHT_TYPE_SPHERE)
    {
        sampleSphereLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_RECT)
    {
        sampleRectLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_SPOT)
    {
        sampleSpotLight(param, scatterPosition, lightSample);
    }
    if (param.type == LIGHT_TYPE_DIRECTIONAL)
    {
        sampleDirectionalLight(param, scatterPosition, lightSample);
    }
}

float computeVisibility(in float3 scatterPosition, in LightSample lightSample)
{
    Payload shadowPayload;
    shadowPayload.isShadowRay = 1;
    shadowPayload.isShadowMiss = 0;

    const float eps = 0.001;
    RayDesc shadowRay;
    shadowRay.TMin = eps;
    shadowRay.TMax = lightSample.distance - eps;

    shadowRay.Direction = lightSample.direction;
    shadowRay.Origin = scatterPosition;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK);

    TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, shadowRay, shadowPayload);

    return (shadowPayload.isShadowMiss == 0) ? 0.0f : 1.0f;
}

float3 RIS_WRS_LightIrradiance(in float3 scatterPosition, inout LightSample finalLightSample)
{
    const float pdf = 1.0f / getLightNum();
    float p_hat = 0;
    LightSample lightSample;

    Reservoir reservoir;
    reservoir.initialize();

    for (int i = 0; i < getLightNum(); i++)
    {
        const uint lightID = (uint) (rand() * (getLightNum()));
        sampleLightWithID(scatterPosition, lightID, lightSample);
        p_hat = length(lightSample.emission);
        float updateW = p_hat / pdf;
        updateReservoir(reservoir, lightID, updateW, 1u, rand());
    }

    sampleLightWithID(scatterPosition, reservoir.Y, finalLightSample);
    p_hat = length(computeVisibility(scatterPosition, finalLightSample) * finalLightSample.emission);

    reservoir.W_y = p_hat > 0 ? rcp(p_hat) * reservoir.W_sum / reservoir.M : 0;
    return reservoir.W_y * finalLightSample.emission;
}

void sampleLightEmitDirAndPosition(inout float3 dir, inout float3 position)
{
    const uint lightID = (uint) (rand() * (getLightNum()));
    LightGenerateParam param = gLightGenerateParams[lightID];

    if (param.type == LIGHT_TYPE_SPHERE)
    {
        sampleSphereLightEmitDirAndPosition(param, dir, position);
    }
    if (param.type == LIGHT_TYPE_RECT)
    {
        sampleRectLightEmitDirAndPosition(param, dir, position);
    }
    if (param.type == LIGHT_TYPE_SPOT)
    {
        sampleSpotLightEmitDirAndPosition(param, dir, position);
    }
    if (param.type == LIGHT_TYPE_DIRECTIONAL)
    {
        sampleDirectionalLightEmitDirAndPosition(param, dir, position);
    }
}

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

    if (blending < 1 - material.transRatio)
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
            const float3 halfVec = GGX_ImportanceSampling(N, material.roughness);
            L = normalize(2.0f * dot(V, halfVec) * halfVec - V);
        }

        nextRay.Origin = currentRayOrigin + N * eps;
        nextRay.Direction = L;
        
        //compute bsdf    V : wo   L : wi(sample)
        float3 brdfDevPDF = specularBRDFdevidedPDF(material, N, V, L);
        const float cosine = max(0, dot(N, nextRay.Direction));
        throughput *= brdfDevPDF * cosine;
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
        float3 H = GGX_ImportanceSampling(N, material.roughness);
        
        const float specRatio = FresnelReflectance(currentRayDir, N, etaOUT);

        float3 L = 0.xxx;

        bool isRefractSampled = true;
        {
            float eta = isFromOutside ? ETA_AIR / etaOUT : etaOUT / ETA_AIR;
            float3 refractVec = refract(currentRayDir, H, eta);
            if (length(refractVec) < 0.001f)
            {
                L = reflect(currentRayDir, H);//handle as total reflection
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
        throughput *= brdfDevPDF * cosine;
    }
}

//=========================================================================
//Spectral Rendering Helper
//=========================================================================
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