#include "photonGathering.hlsli"
#include "opticalFunction.hlsli"

struct VertexPNT {
    float3 Position;
    float3 Normal;
    float2 UV;
};

StructuredBuffer<uint>   indexBuffer : register(t0, space1);
StructuredBuffer<VertexPNT> vertexBuffer: register(t1, space1);
Texture2D<float4> diffuse : register(t2, space1);

VertexPNT GetVertex(TriangleIntersectionAttributes attrib)
{
    VertexPNT v = (VertexPNT)0;
    uint start = PrimitiveIndex() * 3; // Triangle List.

    float3 positionTbl[3], normalTbl[3];
    float2 texcoordTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
        texcoordTbl[i] = vertexBuffer[index].UV;
    }
    v.Position = ComputeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = ComputeInterpolatedAttributeF3(normalTbl, attrib.barys);
    v.UV = ComputeInterpolatedAttributeF2(texcoordTbl, attrib.barys);

    v.Normal = normalize(v.Normal);
    return v;
}

[shader("closesthit")]
void floorClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib) {
    // ClosestHit.
    if (isReachedRecursiveLimitPayload(payload)) {
        return;
    }

    VertexPNT vtx = GetVertex(attrib);
    depthPositionNormalStore(payload, vtx.Normal);

    float4 diffuseColor = diffuse.SampleLevel(gSampler, vtx.UV, 0.0);

    if(!isUseTextureForStage())
    {
        diffuseColor = float4(1, 1, 1 , 1);
    }

    float3 bestFitWorldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
    float3 bestHitWorldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());
    
    const float3 photonIrradiance = photonGather(bestFitWorldPosition, payload.eyeDir, bestHitWorldNormal);
    float3 curEnergy = payload.energy;
    
    payload.color += diffuseColor.xyz * curEnergy * photonIrradiance + diffuseColor.xyz * float3(0.1f, 0.1f, 0.1f);
}

[shader("closesthit")]
void floorStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    // ClosestHit.
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload)) {
        return;
    }
    
    storePhoton(payload);
}

[shader("closesthit")]
void floorDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
}
