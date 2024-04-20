#include "photonGathering.hlsli"

struct VertexPNT
{
    float3 Position;
    float3 Normal;
    float2 UV;
};

ConstantBuffer<MaterialParams> constantBuffer : register(b0, space1);
StructuredBuffer<uint>   indexBuffer : register(t0,space1);
StructuredBuffer<VertexPNT> vertexBuffer : register(t1, space1);
Texture2D<float4> diffuseTex : register(t2, space1);
Texture2D<float> alphaMask: register(t3, space1);

VertexPNT getVertex(TriangleIntersectionAttributes attrib, inout bool isNoTexture)
{
    VertexPNT v = (VertexPNT) 0;
    uint start = PrimitiveIndex() * 3; // Triangle List.

    float3 positionTbl[3], normalTbl[3];
    float2 texcoordTbl[3];
    
    isNoTexture = false;
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
        texcoordTbl[i] = vertexBuffer[index].UV;
        if (texcoordTbl[i].x == 0xff && texcoordTbl[i].y == 0xff)
        {
            isNoTexture = true;
        }
    }
    v.Position = computeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = computeInterpolatedAttributeF3(normalTbl, attrib.barys);
    v.UV = computeInterpolatedAttributeF2(texcoordTbl, attrib.barys);

    v.Normal = normalize(v.Normal);
    return v;
}

void getTexColor(out float4 diffuseTexColor, out bool isIgnoreHit, in bool isNoTexture, in float2 UV)
{
    diffuseTexColor = 0.xxxx;
    float alpMask = 1;
    float2 diffTexSize = 0.xx;
    diffuseTex.GetDimensions(diffTexSize.x, diffTexSize.y);
    float2 alphaMaskSize = 0.xx;
    alphaMask.GetDimensions(alphaMaskSize.x, alphaMaskSize.y);
    const bool isAlphaMaskInvalid = (alphaMaskSize.x == 1 && alphaMaskSize.y == 1);
    bool isTexInvalid = (diffTexSize.x == 1 && diffTexSize.y == 1);

    if (!isNoTexture && !isTexInvalid)
    {
        diffuseTexColor = diffuseTex.SampleLevel(gSampler, UV, 0.0);
        alpMask = alphaMask.SampleLevel(gSampler, UV, 0.0);
        if (diffuseTexColor.r > 0 && diffuseTexColor.g == 0 && diffuseTexColor.b == 0)//1 channel
        {
            diffuseTexColor.rgb = diffuseTexColor.rrr;
        }
    }
    else
    {
        diffuseTexColor = 1.xxxx;
        alpMask = 1;
        isIgnoreHit = false;
        return;
    }
    
    isIgnoreHit = (diffuseTexColor.a < 1) || (!isAlphaMaskInvalid && alpMask < 1);
}

void editMaterial(inout MaterialParams mat)
{
    //I can't decide these params to recognize material as glass
    if (mat.metallic > 0.3 && mat.roughness > 0.5)
    {
        mat.transColor = 1.xxxx;
        mat.transRatio = 0;
        mat.albedo = 1.xxxx;
    }
    else
    {
        mat.roughness = 0.03;
        mat.transColor = 1.xxxx;
        mat.transRatio = 1;
        mat.metallic = 0;
        mat.albedo = 1.xxxx;

        //mat.emission = float4(1,1,0,0);//test
    }
}

[shader("closesthit")]
void materialWithTexClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isShadowRay(payload))
    {
        setVisibility(payload, false);
        return;
    }

    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }

    bool isNoTexture = false;
    VertexPNT vtx = getVertex(attrib, isNoTexture);
    
    float4 diffuseTexColor = 1.xxxx;

    bool isIgnoreHit = false;
    getTexColor(diffuseTexColor, isIgnoreHit, isNoTexture, vtx.UV);

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);

    //recognize as glass
    if (length(currentMaterial.albedo) == 0 && !isNoTexture)
    {
        editMaterial(currentMaterial);
    }

    float3 Le = 0.xxx;
    const bool isLightingRequired = isIndirectOnly() ? (payload.recursive > 2) : true;

    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3)ObjectToWorld4x3());

    if (isLightingRequired)
    {
        const bool isHitLightingRequired = isUseNEE() ? (payload.recursive == 1) : true;
        if (isHitLightingRequired)
        {
            if (intersectLightWithCurrentRay(Le))
            {
                storeAlbedoDepthPositionNormal(payload, currentMaterial.albedo.xyz, vtx.Normal);
                payload.color += payload.throughput * Le;
                return;
            }

            //ray hitted the emissive material
            if (length(currentMaterial.emission.xyz) > 0)
            {
                storeAlbedoDepthPositionNormal(payload, currentMaterial.albedo.xyz, vtx.Normal);
                payload.color += payload.throughput * currentMaterial.emission.xyz;
                return;
            }
        }

        if (isUseNEE())
        {
            LightSample sampledLight;
            float3 scatterPosition = bestFitWorldPosition;
            bool isDirectionalLightSampled = false;
            sampleLight(scatterPosition, sampledLight, isDirectionalLightSampled);
            if (isVisible(scatterPosition, sampledLight))
            {
                float3 lightNormal = sampledLight.normal;
                float3 wi = sampledLight.directionToLight;
                float dist2 = sampledLight.distance * sampledLight.distance;
                float nwi = dot(vtx.Normal, wi);
                bool isValid = (nwi > 0.001f);
                if (isValid)
                {
                    float light_nwo = -dot(lightNormal, wi);
                    if (light_nwo > 0)
                    {
                        float misWeight = 1;
                        float4 bsdfPDF = bsdf_pdf(currentMaterial, vtx.Normal, -WorldRayDirection(), wi);
                        bsdfPDF.w *= light_nwo / dist2;
                        misWeight = (pow(bsdfPDF.w, 2)) / (pow(bsdfPDF.w, 2) + pow(sampledLight.pdf, 2));

                        dist2 = isDirectionalLightSampled ? 1 : dist2;

                        float G = abs(nwi) * abs(light_nwo) / dist2;
                        payload.color += sampledLight.emission
                            * bsdfPDF.xyz
                            * G / sampledLight.pdf
                            //* misWeight
                            * payload.throughput;

                        //payload.color += (sampledLight.emission * bsdfPDF.xyz * (light_nwo / (dist2 * sampledLight.pdf)) * misWeight) * payload.throughput;
                    }
                }
            }
        }

    }
    
    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;

    if (!isIgnoreHit)
    {
        storeAlbedoDepthPositionNormal(payload, currentMaterial.albedo.xyz, vtx.Normal);
        nextRay.Direction = 0.xxx;
        const float3 photon = accumulatePhoton(bestFitWorldPosition, payload.eyeDir, bestFitWorldNormal);
        payload.color += payload.throughput * photon;
        updateDirectionAndThroughput(currentMaterial, vtx.Normal, nextRay, payload.throughput);
    }
    else
    {
        nextRay.TMin = 0.001;
        nextRay.TMax = 10000;
        nextRay.Direction = WorldRayDirection();
    }

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
}

[shader("closesthit")]
void materialWithTexStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    bool isNoTexture = false;
    VertexPNT vtx = getVertex(attrib, isNoTexture);
    
    float4 diffuseTexColor = 1.xxxx;

    bool isIgnoreHit = false;
    getTexColor(diffuseTexColor, isIgnoreHit, isNoTexture, vtx.UV);

    MaterialParams currentMaterial = constantBuffer;
    currentMaterial.albedo *= float4(diffuseTexColor.rgb, 1);

    //recognize as glass
    if (length(currentMaterial.albedo) == 0 && !isNoTexture)
    {
        editMaterial(currentMaterial);
    }

    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
   
    nextRay.TMin = 0.001;
    nextRay.TMax = 10000;
    nextRay.Direction = WorldRayDirection();

    if (!isIgnoreHit && isPhotonStoreRequired(currentMaterial))
    {
        updateDirectionAndThroughput(currentMaterial, vtx.Normal, nextRay, payload.throughput, payload.lambdaNM);
        storePhoton(payload);
    }
    else
    {
        updateDirectionAndThroughput(currentMaterial, vtx.Normal, nextRay, payload.throughput, payload.lambdaNM);
        RAY_FLAG flags = RAY_FLAG_NONE;
        uint rayMask = 0xff;
        TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
    }
}

[shader("closesthit")]
void materialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    // no op
}