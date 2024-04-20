#include "photonGathering.hlsli"

struct VertexPN {
    float3 Position;
    float3 Normal;
};

ConstantBuffer<MaterialParams> constantBuffer : register(b0, space1);
StructuredBuffer<uint>   indexBuffer : register(t0,space1);
StructuredBuffer<VertexPN> vertexBuffer : register(t1, space1);

VertexPN getVertex(TriangleIntersectionAttributes attrib)
{
    VertexPN v = (VertexPN)0;
    uint start = PrimitiveIndex() * 3;
    
    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }

    v.Position = computeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = normalize(computeInterpolatedAttributeF3(normalTbl, attrib.barys));
    return v;
}

[shader("closesthit")]
void materialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
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
    VertexPN vtx = getVertex(attrib);

    MaterialParams currentMaterial = constantBuffer;
    storeAlbedoDepthPositionNormal(payload, currentMaterial.albedo.xyz, vtx.Normal);

    float3 Le = 0.xxx;
    const bool isLightingRequired = isIndirectOnly() ? (payload.recursive >2) : true;

    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3)ObjectToWorld4x3());

    if (isLightingRequired)
    {
        const bool isHitLightingRequired = isUseNEE() ? (payload.recursive == 1) : true;
        if (isHitLightingRequired)
        {
            if (intersectLightWithCurrentRay(Le))
            {
                payload.color += payload.throughput * Le;
                return;
            }

            //ray hitted the emissive material
            if (length(currentMaterial.emission.xyz) > 0)
            {
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

    const float3 photon = accumulatePhoton(bestFitWorldPosition, payload.eyeDir, bestFitWorldNormal);

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;
    payload.color += payload.throughput * photon;
    updateDirectionAndThroughput(currentMaterial, vtx.Normal, nextRay, payload.throughput);

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xff;
    TraceRay(gRtScene, flags, rayMask, DEFAULT_RAY_ID, DEFAULT_GEOM_CONT_MUL, DEFAULT_MISS_ID, nextRay, payload);
}

[shader("closesthit")]
void materialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }

    VertexPN vtx = getVertex(attrib);

    MaterialParams currentMaterial = constantBuffer;
    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestFitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

    RayDesc nextRay;
    nextRay.Origin = bestFitWorldPosition;
    nextRay.Direction = 0.xxx;

    if (isPhotonStoreRequired(currentMaterial))
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