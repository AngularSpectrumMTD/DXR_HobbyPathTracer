#ifndef __OPTICALFUNCTION_HLSLI__
#define __OPTICALFUNCTION_HLSLI__

#include "common.hlsli"

#define REFLECTANCE_BOOST 4

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

//in n0 n1(coat d1 thickness) n2 out 
float FresnelAR(float3 normalizedRayDir, float3 normalizedSurfaceNorm, float lambda, float n0, float n2, float n1 = 1, float d1 = 0)
{
    if (d1 > 0)
        n1 = max(sqrt(n0 * n2), 1.38);

    float cosThetaRAD0 = dot(-normalizedRayDir, normalizedSurfaceNorm);
    float theta0RAD = acos(cosThetaRAD0);

    //refraction angles in coating and the 2nd medium
    float sinTheta0 = sqrt(1 - cosThetaRAD0 * cosThetaRAD0);//sin(acos(x)) = sqrt(1 - x^2)
    float theta1RAD = asin(sinTheta0 * n0 / n1);
    float theta2RAD = asin(sinTheta0 * n0 / n2);

    //transmission on topmost interface
    float theta0plus1 = theta0RAD + theta1RAD;
    float theta0minus1 = theta0RAD - theta1RAD;
    float rs01 = -sin(theta0minus1) / sin(theta0plus1);
    float rp01 = tan(theta0minus1) / tan(theta0plus1);
    float ts01 = 2 * (sinTheta0 * n0 / n1) * cosThetaRAD0 / sin(theta0plus1);
    float tp01 = ts01 * cos(theta0minus1);

    //amplitude for inner reflection
    float theta1plus2RAD = theta1RAD + theta2RAD;
    float theta1minus2RAD = theta1RAD - theta2RAD;
    float rs12 = -sin(theta1minus2RAD) / sin(theta1plus2RAD);
    float rp12 = +tan(theta1minus2RAD) / tan(theta1plus2RAD);

    //after passing throught first surface twice (2 transmissions and 1 reflection)
    float ris = ts01 * ts01 * rs12;
    float rip = tp01 * tp01 * rp12;

    //phase difference between outer and inner reflections
    float dy = d1 * n1;
    float dx = dy * theta1RAD / sqrt(1 - theta1RAD * theta1RAD);//tan(asin(X)) = x / sqrt(1 - x^2)
    float delay = sqrt(dx * dx + dy * dy);
    float cosRelPhase = cos(4 * PI / lambda * (delay - dx * sinTheta0));

    float outS2 = rs01 * rs01 + ris * ris + 2 * rs01 * ris * cosRelPhase;
    float outP2 = rp01 * rp01 + rip * rip + 2 * rp01 * rip * cosRelPhase;

    //reflectivity
    return (outS2 + outP2) / 2;
}

float3 Reflection(float3 vertexPosition, float3 vertexNormal, int recursive, float3 eyeDir, float weight)
{
    float3 worldPos = mul(float4(vertexPosition, 1), ObjectToWorld4x3());
    float3 worldNormal = mul(vertexNormal, (float3x3) ObjectToWorld4x3());
    float3 worldRayDir = WorldRayDirection();
    float3 reflectDir = reflect(worldRayDir, worldNormal);

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;

    RayDesc rayDesc;
    rayDesc.Origin = worldPos;
    rayDesc.Direction = reflectDir;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 100000;

    Payload reflectPayload;
    reflectPayload.color = float3(0, 0, 0);
    reflectPayload.photonColor = float3(0, 0, 0);
    reflectPayload.recursive = recursive;
    reflectPayload.storeIndexXY = int2(0, 0);
    reflectPayload.stored = 0; //empty
    reflectPayload.eyeDir = eyeDir;
    reflectPayload.weight = weight;
    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        reflectPayload);
    return reflectPayload.color;
}

float3 Refraction(float3 vertexPosition, float3 vertexNormal, int recursive, float3 eyeDir, float weight)
{
    float4x3 mtx = ObjectToWorld4x3();
    float3 worldPos = mul(float4(vertexPosition, 1), mtx);
    float3 worldNormal = mul(vertexNormal, (float3x3) mtx);
    float3 worldRayDir = normalize(WorldRayDirection());
    worldNormal = normalize(worldNormal);

    OpticalGlass glassJ_FK5;
    glassJ_FK5.A0 = 2.18826855E+00;
    glassJ_FK5.A1 = 9.19044724E-03;
    glassJ_FK5.A2 = 1.11621071E-04;
    glassJ_FK5.A3 = 9.26372815E-03;
    glassJ_FK5.A4 = 7.34900733E-05;
    glassJ_FK5.A5 = 4.19724242E-06;
    glassJ_FK5.A6 = 1.15412203E-07;
    glassJ_FK5.A7 = 0.00000000E+00;
    glassJ_FK5.A8 = 0.00000000E+00;

    OpticalGlass glassJ_Bak4;
    glassJ_Bak4.A0 = 2.42114503E+00;
    glassJ_Bak4.A1 = -8.99959341E-03;
    glassJ_Bak4.A2 = -9.30006854E-05;
    glassJ_Bak4.A3 = 1.43071120E-02;
    glassJ_Bak4.A4 = 1.89993274E-04;
    glassJ_Bak4.A5 = 6.09602388E-06;
    glassJ_Bak4.A6 = 2.25737069E-07;
    glassJ_Bak4.A7 = 0.00000000E+00;
    glassJ_Bak4.A8 = 0.00000000E+00;

    float reflectance = 0.0f;
    const float IoR = glassJ_Bak4.computeRefIndex(780 * 1e-3);
    const float IoR_Air = 1.0f;

    float nr = dot(worldNormal, worldRayDir);
    float3 refracted;
    if (nr < 0)
    {
         //Air -> Glass
        float eta = IoR_Air / IoR; //roi(before)/roi(after)
        refracted = refract(worldRayDir, worldNormal, eta);
        reflectance = FresnelAR(getViewVec(), worldNormal, 780, IoR_Air, IoR);
        reflectance = clamp(reflectance, 0, 1);
    }
    else
    {
         //Glass -> Air
        float eta = IoR / IoR_Air;
        refracted = refract(worldRayDir, -worldNormal, eta);
        reflectance = FresnelAR(getViewVec(), -worldNormal, 780, IoR, IoR_Air);
        reflectance = clamp(reflectance, 0, 1);
    }
    
    if (length(refracted) < 0.000001)
    {
        return Reflection(vertexPosition, vertexNormal, recursive, eyeDir, 0);
        //return 0.xxx;
    }
    else
    {
        RAY_FLAG flags = RAY_FLAG_NONE;
        uint rayMask = 0xff;

        RayDesc rayDesc;
        rayDesc.Origin = worldPos;
        rayDesc.Direction = refracted;
        rayDesc.TMin = 0.001;
        rayDesc.TMax = 100000;

        Payload refractPayload;
        refractPayload.color = float3(0, 1, 0);
        refractPayload.photonColor = float3(0, 0, 0);
        refractPayload.recursive = recursive;
        refractPayload.storeIndexXY = int2(0, 0);
        refractPayload.stored = 0; //empty
        refractPayload.eyeDir = eyeDir;
        refractPayload.weight = 0;
        TraceRay(
            gRtScene,
            flags,
            rayMask,
            0, // ray index
            1, // MultiplierForGeometryContrib
            0, // miss index
            rayDesc,
            refractPayload);
        return refractPayload.color;
    }
}

void ReflectionPhoton(float3 vertexPosition, float3 vertexNormal, PhotonPayload photonPayload)
{
    float3 worldPos = mul(float4(vertexPosition, 1), ObjectToWorld4x3());
    float3 worldNormal = mul(vertexNormal, (float3x3) ObjectToWorld4x3());
    float3 worldRayDir = WorldRayDirection();
    float3 reflectDir = reflect(worldRayDir, worldNormal);

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = ~(LIGHT_INSTANCE_MASK); //light remove

    RayDesc rayDesc;
    rayDesc.Origin = worldPos;
    rayDesc.Direction = reflectDir;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 100000;

    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        photonPayload);
}

void RefractionPhoton(float3 vertexPosition, float3 vertexNormal, PhotonPayload photonPayload)
{
    float4x3 mtx = ObjectToWorld4x3();
    float3 worldPos = mul(float4(vertexPosition, 1), mtx);
    float3 worldNormal = mul(vertexNormal, (float3x3) mtx);
    float3 worldRayDir = normalize(WorldRayDirection());
    worldNormal = normalize(worldNormal);

    OpticalGlass glassJ_FK5;
    glassJ_FK5.A0 = 2.18826855E+00;
    glassJ_FK5.A1 = 9.19044724E-03;
    glassJ_FK5.A2 = 1.11621071E-04;
    glassJ_FK5.A3 = 9.26372815E-03;
    glassJ_FK5.A4 = 7.34900733E-05;
    glassJ_FK5.A5 = 4.19724242E-06;
    glassJ_FK5.A6 = 1.15412203E-07;
    glassJ_FK5.A7 = 0.00000000E+00;
    glassJ_FK5.A8 = 0.00000000E+00;

    OpticalGlass glassJ_Bak4;
    glassJ_Bak4.A0 = 2.42114503E+00;
    glassJ_Bak4.A1 = -8.99959341E-03;
    glassJ_Bak4.A2 = -9.30006854E-05;
    glassJ_Bak4.A3 = 1.43071120E-02;
    glassJ_Bak4.A4 = 1.89993274E-04;
    glassJ_Bak4.A5 = 6.09602388E-06;
    glassJ_Bak4.A6 = 2.25737069E-07;
    glassJ_Bak4.A7 = 0.00000000E+00;
    glassJ_Bak4.A8 = 0.00000000E+00;

    const float IoR = glassJ_Bak4.computeRefIndex(photonPayload.lambdaNM * 1e-3);

    float reflectance = 0.0f;
    const float IoR_Air = 1.0f;
    float nr = dot(worldNormal, worldRayDir);
    float3 refracted;
    if (nr < 0)
    {
        //Air -> Glass
        float eta = IoR_Air / IoR; //roi(before)/roi(after)
        refracted = refract(worldRayDir, worldNormal, eta);
        reflectance = FresnelAR(worldRayDir, worldNormal, photonPayload.lambdaNM, IoR_Air, IoR);
        reflectance = clamp(reflectance, 0, 1);
    }
    else
    {
        //Glass -> Air
        float eta = IoR / IoR_Air;
        refracted = refract(worldRayDir, -worldNormal, eta);
        reflectance = FresnelAR(worldRayDir, -worldNormal, photonPayload.lambdaNM, IoR, IoR_Air);
        reflectance = clamp(reflectance, 0, 1);
    }

    {
        const float3 src = photonPayload.throughput;
        photonPayload.throughput *= abs(1 - reflectance);

        RAY_FLAG flags = RAY_FLAG_NONE;
        uint rayMask = ~(LIGHT_INSTANCE_MASK); //light remove

        RayDesc rayDesc;
        rayDesc.Origin = worldPos;
        rayDesc.Direction = refracted;
        rayDesc.TMin = 0.001;
        rayDesc.TMax = 100000;
        
        bool isExecuteReflect = (photonPayload.storeIndex % 10 < 3); //30%Reflect
    
        if (isExecuteReflect)
        {
            PhotonPayload photonPayloadReflect;
            photonPayloadReflect.throughput = length(src).xxx * reflectance * REFLECTANCE_BOOST;
            photonPayloadReflect.recursive = photonPayload.recursive; //if reset this param, infinite photon emission is occurred. This cause GPU HUNG!!!
            photonPayloadReflect.storeIndex = photonPayload.storeIndex;
            photonPayloadReflect.stored = 0; //empty
            photonPayloadReflect.lambdaNM = photonPayload.lambdaNM;
            RayDesc rayDescReflect;
            rayDescReflect.Origin = worldPos;
            rayDescReflect.Direction = reflect(worldRayDir, worldNormal);
            rayDescReflect.TMin = 0.001;
            rayDescReflect.TMax = 100000;

            ReflectionPhoton(vertexPosition, vertexNormal, photonPayloadReflect);
        }
        else
        {
             //refract
            TraceRay(
                gRtScene,
                flags,
                rayMask,
                0, // ray index
                1, // MultiplierForGeometryContrib
                0, // miss index
                rayDesc,
                photonPayload);
        }
    }
}

#endif//__OPTICALFUNCTION_HLSLI__