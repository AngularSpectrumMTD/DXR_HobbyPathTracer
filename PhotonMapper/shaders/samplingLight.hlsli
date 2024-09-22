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
void sampleSphereLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    float u = rand();
    float v = rand();
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

void sampleRectLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    float u = rand();
    float v = rand();
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

void sampleSpotLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    float u = rand();
    float v = rand();
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

float3 coneSample(float3 N, float cosMax, out float2 randomUV)
{
    float u = rand();
    float v = rand();
    randomUV = float2(u, v);
    float ct = 1 - rand() * (1 - cosMax);
    float st = sqrt(1 - ct * ct);
    float phi = 2 * PI * rand();
    float2 csp;
    sincos(phi, csp.x, csp.y);
    return tangentToWorld(N, float3(st * csp.x, st * csp.y, ct));
}

float3 coneSampleWithRandom(float3 N, float cosMax, in float2 randomUV)
{
    float u = randomUV.x;
    float v = randomUV.y;
    float ct = 1 - rand() * (1 - cosMax);
    float st = sqrt(1 - ct * ct);
    float phi = 2 * PI * rand();
    float2 csp;
    sincos(phi, csp.x, csp.y);
    return tangentToWorld(N, float3(st * csp.x, st * csp.y, ct));
}

void sampleDirectionalLight(in LightGenerateParam lightGen, in float3 scatterPosition, inout LightSample lightSample)
{
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);

    float3 fromLight = normalize(lightGen.positionORDirection);
    float2 dummy = 0.xx;
    float3 emit = coneSample(fromLight, cosMax, dummy);
    
    lightSample.type = LIGHT_TYPE_DIRECTIONAL;
    lightSample.directionToLight = -normalize(emit);
    lightSample.normal = fromLight;
    lightSample.emission = lightGen.emission;
    lightSample.distance = RAY_MAX_T;
    lightSample.pdf = 1 / (2 * PI * (1 - cosMax));
}

void sampleSphereLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, out float2 randomUV)
{
    emitDir = HemisphereORCosineSampling(float3(1, 0, 0), false, randomUV);
    position = lightGen.positionORDirection;
}

void sampleSphereLightEmitDirAndPositionWithRandom(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    emitDir = HemisphereORCosineSamplingWithRandom(float3(1, 0, 0), false, randomUV);
    position = lightGen.positionORDirection;
}

void sampleRectLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, out float2 randomUV)
{
    float u = rand();
    float v = rand();
    randomUV = float2(u, v);
    float rnd0 = (u - 0.5) * 2; //-1 to 1
    float rnd1 = (v - 0.5) * 2; //-1 to 1
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    emitDir = normalize(dominantDir * LIGHT_BASE_LENGTH + rnd0 * lightGen.U + rnd1 * lightGen.V);
    position = lightGen.positionORDirection;
}

void sampleRectLightEmitDirAndPositionWithRandom(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    float u = randomUV.x;
    float v = randomUV.y;
    float rnd0 = (u - 0.5) * 2; //-1 to 1
    float rnd1 = (v - 0.5) * 2; //-1 to 1
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    emitDir = normalize(dominantDir * LIGHT_BASE_LENGTH + rnd0 * lightGen.U + rnd1 * lightGen.V);
    position = lightGen.positionORDirection;
}

void sampleSpotLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, out float2 randomUV)
{
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    const float cosMax = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    emitDir = coneSample(dominantDir, cosMax, randomUV);
    position = lightGen.positionORDirection;
}

void sampleSpotLightEmitDirAndPositionWithRandom(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    const float3 dominantDir = normalize(cross(lightGen.U, lightGen.V));
    const float cosMax = atan2(length(lightGen.U), LIGHT_BASE_LENGTH);
    emitDir = coneSampleWithRandom(dominantDir, cosMax, randomUV);
    position = lightGen.positionORDirection;
}

void sampleDirectionalLightEmitDirAndPosition(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, out float2 randomUV)
{
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
    float3 fromLight = normalize(lightGen.positionORDirection);
    emitDir = coneSample(fromLight, cosMax, randomUV);
    position = 100 * -emitDir;
}

void sampleDirectionalLightEmitDirAndPositionWithRandom(in LightGenerateParam lightGen, out float3 emitDir, out float3 position, in float2 randomUV)
{
    const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
    float3 fromLight = normalize(lightGen.positionORDirection);
    emitDir = coneSampleWithRandom(fromLight, cosMax, randomUV);
    position = 100 * -emitDir;
}

void sampleLight(in float3 scatterPosition, inout LightSample lightSample)
{
    const uint lightID = getRandomLightID();
    LightGenerateParam param = gLightGenerateParams[lightID];
    
    if (isSphereLight(param))
    {
        sampleSphereLight(param, scatterPosition, lightSample);
    }
    if (isRectLight(param))
    {
        sampleRectLight(param, scatterPosition, lightSample);
    }
    if (isSpotLight(param))
    {
        sampleSpotLight(param, scatterPosition, lightSample);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLight(param, scatterPosition, lightSample);
    }

    lightSample.pdf *= 1.0f / getLightNum();
}

void sampleLightWithID(in float3 scatterPosition, in int ID, inout LightSample lightSample)
{
    LightGenerateParam param = gLightGenerateParams[ID];
    
    if (isSphereLight(param))
    {
        sampleSphereLight(param, scatterPosition, lightSample);
    }
    if (isRectLight(param))
    {
        sampleRectLight(param, scatterPosition, lightSample);
    }
    if (isSpotLight(param))
    {
        sampleSpotLight(param, scatterPosition, lightSample);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLight(param, scatterPosition, lightSample);
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

bool intersectLightWithCurrentRay(out float3 Le)
{
    const float3 rayOrigin = WorldRayOrigin();
    const float3 rayDiretion = WorldRayDirection();
    const float rayT = RayTCurrent();

    const uint lightID = getRandomLightID();
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

void sampleLightEmitDirAndPosition(inout float3 dir, inout float3 position, out float2 randomUV, out float pdf)
{
    const uint lightID = getRandomLightID();
    LightGenerateParam param = gLightGenerateParams[0];
    //LightGenerateParam param = gLightGenerateParams[lightID];

    if (isSphereLight(param))
    {
        sampleSphereLightEmitDirAndPosition(param, dir, position, randomUV);
        pdf = 1 / (4 * PI * param.sphereRadius * param.sphereRadius);
    }
    if (isRectLight(param))
    {
        sampleRectLightEmitDirAndPosition(param, dir, position, randomUV);
        float lenU = sqrt(dot(param.U, param.U));
        float lenV = sqrt(dot(param.V, param.V));
        pdf = 1 / (4 * lenU * lenV);
    }
    if (isSpotLight(param))
    {
        sampleSpotLightEmitDirAndPosition(param, dir, position, randomUV);
        float lenU = sqrt(dot(param.U, param.U));
        float lenV = sqrt(dot(param.V, param.V));
        pdf = 1 / (PI * lenU * lenV);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLightEmitDirAndPosition(param, dir, position, randomUV);
        const float cosMax = cos(DIRECTIONAL_LIGHT_SPREAD_HALF_ANGLE_RADIAN);
        pdf = 1 / (2 * PI * (1 - cosMax));
    }
}

void sampleLightEmitDirAndPositionWithRandom(inout float3 dir, inout float3 position, in float2 randomUV)
{
    const uint lightID = getRandomLightID();
    LightGenerateParam param = gLightGenerateParams[0];
    //LightGenerateParam param = gLightGenerateParams[lightID];

    if (isSphereLight(param))
    {
        sampleSphereLightEmitDirAndPositionWithRandom(param, dir, position, randomUV);
    }
    if (isRectLight(param))
    {
        sampleRectLightEmitDirAndPositionWithRandom(param, dir, position, randomUV);
    }
    if (isSpotLight(param))
    {
        sampleSpotLightEmitDirAndPositionWithRandom(param, dir, position, randomUV);
    }
    if (isDirectionalLight(param))
    {
        sampleDirectionalLightEmitDirAndPositionWithRandom(param, dir, position, randomUV);
    }
}

#endif//__SAMPLING_LIGHT_HLSLI__