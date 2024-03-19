#ifndef __GEOMETRY_INTERSECTION_HLSLI__
#define __GEOMETRY_INTERSECTION_HLSLI__
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

#endif//__GEOMETRY_INTERSECTION_HLSLI__