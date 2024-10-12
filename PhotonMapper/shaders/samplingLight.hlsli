#ifndef __SAMPLING_LIGHT_HLSLI__
#define __SAMPLING_LIGHT_HLSLI__

//=========================================================================
//Lighting
//=========================================================================
struct LightSample
{
    uint type;
    float3 directionToLight;
    float3 normal;
    float3 emission;
    float distance;
    float pdf;
};

bool isSphereLight(in LightSample lightSample)
{
    return (lightSample.type == LIGHT_TYPE_SPHERE);
}

bool isRectLight(in LightSample lightSample)
{
    return (lightSample.type == LIGHT_TYPE_RECT);
}

bool isSpotLight(in LightSample lightSample)
{
    return (lightSample.type == LIGHT_TYPE_SPOT);
}

bool isDirectionalLight(in LightSample lightSample)
{
    return (lightSample.type == LIGHT_TYPE_DIRECTIONAL);
}

bool isSphereLight(in LightGenerateParam lightParam)
{
    return (lightParam.type == LIGHT_TYPE_SPHERE);
}

bool isRectLight(in LightGenerateParam lightParam)
{
    return (lightParam.type == LIGHT_TYPE_RECT);
}

bool isSpotLight(in LightGenerateParam lightParam)
{
    return (lightParam.type == LIGHT_TYPE_SPOT);
}

bool isDirectionalLight(in LightGenerateParam lightParam)
{
    return (lightParam.type == LIGHT_TYPE_DIRECTIONAL);
}

#define DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN 5 * PI / 180

//ok
void sampleSphereLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample, inout uint randomSeed)
{
    float u = rand(randomSeed);
    float v = rand(randomSeed);
    float z = -2.0f * u + 1.0f;
    float y = sqrt(max(0, 1 - z * z)) * sin(2.0f * PI * v);
    float x = sqrt(max(0, 1 - z * z)) * cos(2.0f * PI * v);

    float3 pos = lightGen.positionORDirection + lightGen.sphereRadius * float3(x, y, z);
    lightSample.type = LIGHT_TYPE_SPHERE;
    lightSample.directionToLight = pos - scatterPosition;
    lightSample.normal = normalize(float3(x, y, z));
    lightSample.emission = lightGen.emission;
    lightSample.distance = sqrt(dot(lightSample.directionToLight, lightSample.directionToLight));
    lightSample.directionToLight /= lightSample.distance;
    lightSample.pdf = 1 / (4 * PI * lightGen.sphereRadius * lightGen.sphereRadius);
}

void sampleRectLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample, inout uint randomSeed)
{
    float u = rand(randomSeed);
    float v = rand(randomSeed);
    float lenU = sqrt(dot(lightGen.U, lightGen.U));
    float lenV = sqrt(dot(lightGen.V, lightGen.V));

    float3 pos = lightGen.positionORDirection + 2 * (u - 0.5) * lightGen.U + 2 * (v - 0.5) * lightGen.V;
    lightSample.type = LIGHT_TYPE_RECT;
    lightSample.directionToLight = pos - scatterPosition;
    lightSample.normal = normalize(cross(lightGen.U, lightGen.V));
    lightSample.emission = lightGen.emission;
    lightSample.distance = sqrt(dot(lightSample.directionToLight, lightSample.directionToLight));
    lightSample.directionToLight /= lightSample.distance;
    lightSample.pdf = 1 / (4 * lenU * lenV);
}

void sampleSpotLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample, inout uint randomSeed)
{
    float u = rand(randomSeed);
    float v = rand(randomSeed);
    float lenU = sqrt(dot(lightGen.U, lightGen.U));
    float lenV = sqrt(dot(lightGen.V, lightGen.V));
    float r = lenU * sqrt(u);
    float theta = 2 * PI * v;
    float x = r * cos(theta);
    float y = r * sin(theta);
    y *= lenV / lenU;
    
    float3 pos = lightGen.positionORDirection + x * normalize(lightGen.U) + y * normalize(lightGen.V);
    lightSample.type = LIGHT_TYPE_SPOT;
    lightSample.directionToLight = pos - scatterPosition;
    lightSample.normal = normalize(cross(lightGen.U, lightGen.V));
    lightSample.emission = lightGen.emission;
    lightSample.distance = sqrt(dot(lightSample.directionToLight, lightSample.directionToLight));
    lightSample.directionToLight /= lightSample.distance;
    lightSample.pdf = 1 / (PI * lenU * lenV);
}

float3 coneSample(float3 N, float cosMax, inout uint randomSeed)
{
    float u = rand(randomSeed);
    float v = rand(randomSeed);
    float ct = 1 - u * (1 - cosMax);
    float st = sqrt(1 - ct * ct);
    float phi = 2 * PI * v;
    float2 csp;
    sincos(phi, csp.x, csp.y);
    return tangentToWorld(N, float3(st * csp.x, st * csp.y, ct));
}

float3 coneSampleHighFreq(float3 N, float cosMax, inout uint randomSeed, out float2 randomUV)
{
    float u = randHighFreq(randomSeed);
    float v = randHighFreq(randomSeed);
    randomUV = float2(u, v);
    float ct = 1 - u * (1 - cosMax);
    float st = sqrt(1 - ct * ct);
    float phi = 2 * PI * v;
    float2 csp;
    sincos(phi, csp.x, csp.y);
    return tangentToWorld(N, float3(st * csp.x, st * csp.y, ct));
}

float3 coneSampleWithUV(float3 N, float cosMax, in float2 randomUV)
{
    float u = randomUV.x;
    float v = randomUV.y;
    float ct = 1 - u * (1 - cosMax);
    float st = sqrt(1 - ct * ct);
    float phi = 2 * PI * v;
    float2 csp;
    sincos(phi, csp.x, csp.y);
    return tangentToWorld(N, float3(st * csp.x, st * csp.y, ct));
}

void sampleDirectionalLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample, inout uint randomSeed)
{
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);

    float3 fromLight = normalize(lightGen.positionORDirection);
    float3 emit = coneSample(fromLight, cosMax, randomSeed);
    
    lightSample.type = LIGHT_TYPE_DIRECTIONAL;
    lightSample.directionToLight = -normalize(emit);
    lightSample.normal = fromLight;
    lightSample.emission = lightGen.emission;
    lightSample.distance = RAY_MAX_T;
    lightSample.pdf = 1 / (2 * PI * (1 - cosMax));
}

void sampleSphereLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, inout uint randomSeed, out float2 randomUV)
{
    emitDir = HemisphereORCosineSampling(float3(1, 0, 0), false, randomSeed, randomUV);
    position = lightGen.positionORDirection;
}

void sampleSphereLightEmitDirAndPositionWithUV(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    emitDir = HemisphereORCosineSamplingWithUV(float3(1, 0, 0), false, randomUV);
    position = lightGen.positionORDirection;
}

void sampleRectLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, inout uint randomSeed, out float2 randomUV)
{
    float u = randHighFreq(randomSeed);
    float v = randHighFreq(randomSeed);
    randomUV = float2(u, v);
    float rnd0 = (u - 0.5) * 2; //-1 to 1
    float rnd1 = (v - 0.5) * 2; //-1 to 1
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    emitDir = normalize(dominantDir * LIGHT_BASE_LENGTH + rnd0 * lightGen.U + rnd1 * lightGen.V);
    position = lightGen.positionORDirection;
}

void sampleRectLightEmitDirAndPositionWithUV(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    float u = randomUV.x;
    float v = randomUV.y;
    float rnd0 = (u - 0.5) * 2; //-1 to 1
    float rnd1 = (v - 0.5) * 2; //-1 to 1
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    emitDir = normalize(dominantDir * LIGHT_BASE_LENGTH + rnd0 * lightGen.U + rnd1 * lightGen.V);
    position = lightGen.positionORDirection;
}

void sampleSpotLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, inout uint randomSeed, inout float2 randomUV)
{
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    const float cosMax = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    emitDir = coneSampleHighFreq(dominantDir, cosMax, randomSeed, randomUV);
    position = lightGen.positionORDirection;
}

void sampleSpotLightEmitDirAndPositionWithUV(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    const float cosMax = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    emitDir = coneSampleWithUV(dominantDir, cosMax, randomUV);
    position = lightGen.positionORDirection;
}

void sampleDirectionalLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, inout uint randomSeed, inout float2 randomUV)
{
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
    float3 fromLight = normalize(lightGen.positionORDirection);
    emitDir = coneSampleHighFreq(fromLight, cosMax, randomSeed, randomUV);
    position = 100 * -emitDir;
}

void sampleDirectionalLightEmitDirAndPositionWithUV(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
    float3 fromLight = normalize(lightGen.positionORDirection);
    emitDir = coneSampleWithUV(fromLight, cosMax, randomUV);
    position = 100 * -emitDir;
}

void sampleLight(in float3 scatterPosition, inout LightSample lightSample, inout uint randomSeed)
{
    const uint lightID = getRandomLightID(randomSeed);
    LightGenerateParam param = gLightGenerateParams[lightID];
    
    if (isSphereLight(param))
    {
        sampleSphereLight(param, scatterPosition, lightSample, randomSeed);
    }
    if (isRectLight(param))
    {
        sampleRectLight(param, scatterPosition, lightSample, randomSeed);
    }
    if (isSpotLight(param))
    {
        sampleSpotLight(param, scatterPosition, lightSample, randomSeed);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLight(param, scatterPosition, lightSample, randomSeed);
    }

    lightSample.pdf *= 1.0f / getLightNum();
}

void sampleLightWithID(in float3 scatterPosition, in int ID, inout LightSample lightSample, inout uint randomSeed)
{
    LightGenerateParam param = gLightGenerateParams[ID];
    
    if (isSphereLight(param))
    {
        sampleSphereLight(param, scatterPosition, lightSample, randomSeed);
    }
    if (isRectLight(param))
    {
        sampleRectLight(param, scatterPosition, lightSample, randomSeed);
    }
    if (isSpotLight(param))
    {
        sampleSpotLight(param, scatterPosition, lightSample, randomSeed);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLight(param, scatterPosition, lightSample, randomSeed);
    }
}

float getModifiedSquaredDistance(in LightSample lightSample)
{
    if (isDirectionalLight(lightSample))
    {
        return 1;
    }
    else
    {
        return lightSample.distance * lightSample.distance;
    }
}

bool intersectLightWithCurrentRay(out float3 Le, inout uint randomSeed)
{
    const float3 rayOrigin = WorldRayOrigin();
    const float3 rayDiretion = WorldRayDirection();
    const float rayT = RayTCurrent();

    const uint lightID = getRandomLightID(randomSeed);
    LightGenerateParam param = gLightGenerateParams[lightID];

    if (isSphereLight(param))
    {
        const float3 shapeForwardDir = normalize(cross(param.U, param.V));
        //float2 tt = intersectEllipsoid(rayOrigin, rayDiretion, param.position,
        //shapeForwardDir, normalize(param.V), param.sphereRadius, param.sphereRadius, param.sphereRadius);
        //float frontT = tt.x >= 0 ? tt.x : tt.y;
        float t = intersectSphere(rayOrigin, rayDiretion, param.positionORDirection,
        shapeForwardDir, normalize(param.V), param.sphereRadius, param.sphereRadius, param.sphereRadius);

        Le = param.emission * max(1, getLightNum() - 1);
        return (t >= 0 && t < rayT);
    }
    else if (isRectLight(param))
    {
        const float3 shapeForwardDir = normalize(cross(param.U, param.V));
        float hittedT = intersectRectangle(rayOrigin, rayDiretion, param.positionORDirection, param.U, param.V);
        const bool isFrontHit = (length(shapeForwardDir) > 0) && (dot(shapeForwardDir, -rayDiretion) > 0);

        Le = param.emission * max(1, getLightNum() - 1);
        return isFrontHit && (hittedT > 0 && hittedT < rayT);
    }
    else if (isSpotLight(param))
    {
        const float3 shapeForwardDir = normalize(cross(param.U, param.V));
        float hittedT = intersectEllipse(rayOrigin, rayDiretion, param.positionORDirection, param.U, param.V);
        const bool isFrontHit = (length(shapeForwardDir) > 0) && (dot(shapeForwardDir, -rayDiretion) > 0);

        Le = param.emission * max(1, getLightNum() - 1);
        return isFrontHit && (hittedT > 0 && hittedT < rayT);
    }

    return false;
}

bool intersectAllLightWithCurrentRay(out float3 Le, out float3 hitPosition, out float3 hitNormal)
{
    const float3 rayOrigin = WorldRayOrigin();
    const float3 rayDiretion = WorldRayDirection();
    const float rayT = RayTCurrent();

    bool isIntersect = false;
    int mostNearIndex = -1;
    float currT = RAY_MAX_T;
    float3 mostNearLe = 0.xxx;
    bool isHit = false;

    for (int i = 0; i < getLightNum(); i++)
    {
        LightGenerateParam param = gLightGenerateParams[i];

        if (isSphereLight(param))
        {
            const float3 shapeForwardDir = normalize(cross(param.U, param.V));
            float hittedT = intersectSphere(rayOrigin, rayDiretion, param.positionORDirection,
            shapeForwardDir, normalize(param.V), param.sphereRadius, param.sphereRadius, param.sphereRadius);

            Le = param.emission * max(1, getLightNum() - 1);
            isIntersect = (hittedT >= 0 && hittedT < rayT);
            if (isIntersect)
            {
                if(currT > hittedT)
                {
                    mostNearIndex = i;
                    currT = hittedT;
                    mostNearLe = Le;
                    hitNormal = normalize(rayOrigin + currT * rayDiretion - param.positionORDirection);
                }
            }
        }
        else if (isRectLight(param))
        {
            const float3 shapeForwardDir = normalize(cross(param.U, param.V));
            float hittedT = intersectRectangle(rayOrigin, rayDiretion, param.positionORDirection, param.U, param.V);
            const bool isFrontHit = (length(shapeForwardDir) > 0) && (dot(shapeForwardDir, -rayDiretion) > 0);

            Le = param.emission * max(1, getLightNum() - 1);
            isIntersect =  isFrontHit && (hittedT > 0 && hittedT < rayT);
            if (isIntersect)
            {
                if(currT > hittedT)
                {
                    mostNearIndex = i;
                    currT = hittedT;
                    mostNearLe = Le;
                    hitNormal = shapeForwardDir;
                }
            }
        }
        else if (isSpotLight(param))
        {
            const float3 shapeForwardDir = normalize(cross(param.U, param.V));
            float hittedT = intersectEllipse(rayOrigin, rayDiretion, param.positionORDirection, param.U, param.V);
            const bool isFrontHit = (length(shapeForwardDir) > 0) && (dot(shapeForwardDir, -rayDiretion) > 0);

            Le = param.emission * max(1, getLightNum() - 1);
            isIntersect =  isFrontHit && (hittedT > 0 && hittedT < rayT);
            if (isIntersect)
            {
                if(currT > hittedT)
                {
                    mostNearIndex = i;
                    currT = hittedT;
                    mostNearLe = Le;
                    hitNormal = shapeForwardDir;
                }
            }
        }
    }

    Le = mostNearLe;
    hitPosition = rayOrigin + currT * rayDiretion;

    return (mostNearIndex != -1);
}

float3 directionalLightingOnMissShader(Payload payload)
{
    float3 val = 0.xxx;

    float3 directionalDir = 0.xxx;
    bool isDirectionalLightFinded = false;
    float3 emis = 0.xxx;
    float3 dominantDir = 0.xxx;
    for (int i = 0; i < getLightNum(); i++)
    {
        LightGenerateParam lightGen = gLightGenerateParams[i];

        if (isDirectionalLightFinded == false && isDirectionalLight(lightGen))
        {
            isDirectionalLightFinded = true;
            emis = lightGen.emission;
            dominantDir = normalize(lightGen.positionORDirection);
        }
    }
    
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
    if (payload.recursive > 0 && isDirectionalLightFinded && dot(dominantDir, -WorldRayDirection()) > cosMax)
    {
        val = U32toF32x3(payload.compressedThroughput) * emis;
    }

    return val;
}

bool isVisible(in float3 scatterPosition, in LightSample lightSample)
{
    Payload shadowPayload;
    shadowPayload.flags = 0;
    shadowPayload.flags |= PAYLOAD_BIT_MASK_IS_SHADOW_RAY;
    shadowPayload.T = 0;

    const float eps = 0.005;
    RayDesc shadowRay;
    shadowRay.TMin = eps;
    shadowRay.TMax = lightSample.distance - eps;

    shadowRay.Direction = lightSample.directionToLight;
    shadowRay.Origin = scatterPosition;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK);

    TraceDefaultRay(flags, rayMask, shadowRay, shadowPayload);

    return (shadowPayload.flags & PAYLOAD_BIT_MASK_IS_SHADOW_MISS);
}

void sampleLightEmitDirAndPosition(inout float3 dir, inout float3 position, out float2 randomUV, out float pdf, inout uint randomSeed)
{
    const uint lightID = getRandomLightID(randomSeed);
    LightGenerateParam param = gLightGenerateParams[0];
    //LightGenerateParam param = gLightGenerateParams[lightID];

    if (isSphereLight(param))
    {
        sampleSphereLightEmitDirAndPosition(param, dir, position, randomSeed, randomUV);
        pdf = 1 / (4 * PI * param.sphereRadius * param.sphereRadius);
    }
    if (isRectLight(param))
    {
        sampleRectLightEmitDirAndPosition(param, dir, position, randomSeed, randomUV);
        float lenU = sqrt(dot(param.U, param.U));
        float lenV = sqrt(dot(param.V, param.V));
        pdf = 1 / (4 * lenU * lenV);
    }
    if (isSpotLight(param))
    {
        sampleSpotLightEmitDirAndPosition(param, dir, position, randomSeed, randomUV);
        float lenU = sqrt(dot(param.U, param.U));
        float lenV = sqrt(dot(param.V, param.V));
        pdf = 1 / (PI * lenU * lenV);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLightEmitDirAndPosition(param, dir, position, randomSeed, randomUV);
        const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
        pdf = 1 / (2 * PI * (1 - cosMax));
    }
}

void sampleLightEmitDirAndPositionWithUV(inout float3 dir, inout float3 position, in float2 randomUV, inout uint randomSeed)
{
    const uint lightID = getRandomLightID(randomSeed);
    LightGenerateParam param = gLightGenerateParams[0];
    //LightGenerateParam param = gLightGenerateParams[lightID];

    if (isSphereLight(param))
    {
        sampleSphereLightEmitDirAndPositionWithUV(param, dir, position, randomUV);
    }
    if (isRectLight(param))
    {
        sampleRectLightEmitDirAndPositionWithUV(param, dir, position, randomUV);
    }
    if (isSpotLight(param))
    {
        sampleSpotLightEmitDirAndPositionWithUV(param, dir, position, randomUV);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLightEmitDirAndPositionWithUV(param, dir, position, randomUV);
    }
}

#endif//__SAMPLING_LIGHT_HLSLI__