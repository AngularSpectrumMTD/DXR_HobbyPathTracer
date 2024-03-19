#ifndef __SAMPLING_LIGHT_HLSLI__
#define __SAMPLING_LIGHT_HLSLI__

//=========================================================================
//Lighting
//=========================================================================
struct LightSample
{
    float3 direction; //from scatterPos to lightPos;
    float3 normal;
    float3 emission;
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
    float rnd0 = (rand() * 0.5 - 1) * 2; //-1 to 1
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
    lightSample.direction = normalize(-lightGen.position); //pos as dir
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

#endif//__SAMPLING_LIGHT_HLSLI__