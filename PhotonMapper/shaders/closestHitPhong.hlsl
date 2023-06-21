#include "photonGathering.hlsli"

struct VertexPN {
    float3 Position;
    float3 Normal;
};
struct MaterialCB {
    float4 albedo;
    float4 specular;
};

ConstantBuffer<MaterialCB> constantBuffer: register(b0, space1);
StructuredBuffer<uint>   indexBuffer : register(t0,space1);
StructuredBuffer<VertexPN>  vertexBuffer : register(t1,space1);

VertexPN GetVertex(TriangleIntersectionAttributes attrib)
{
    VertexPN v = (VertexPN)0;
    uint start = PrimitiveIndex() * 3;
    
    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i) {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }

    v.Position = ComputeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = normalize(ComputeInterpolatedAttributeF3(normalTbl, attrib.barys));
    return v;
}

[shader("closesthit")]
void phongMaterialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPayload(payload)) {
        return;
    }
    VertexPN vtx = GetVertex(attrib);

    depthPositionNormalStore(payload, vtx.Normal);

    uint instanceID = InstanceID();
    float3 albedo = constantBuffer.albedo.xyz;
    float power = constantBuffer.specular.w;

    if (instanceID == 2) {

        float3 worldPosition = mul(float4(vtx.Position, 1), ObjectToWorld4x3());
        float3 worldNormal = mul(vtx.Normal, (float3x3)ObjectToWorld4x3());
        
        payload.color = photonGather(WorldRayOrigin() + WorldRayDirection() * RayTCurrent(), payload.eyeDir, worldNormal) + 0.3 * albedo;
        
    } else {
        payload.color = float3(0,0,0); // meaningless.
    }
}

[shader("closesthit")]
void phongMaterialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload) || isOverSplitted(payload)) {
        return;
    }

    VertexPN vtx = GetVertex(attrib);

    uint instanceID = InstanceID();

    if (instanceID == 2) {
        storePhoton(payload);
    } else {
        payload.throughput = float3(0,0,0); // meaningless.
    }
}

[shader("closesthit")]
void phongMaterialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    // no op
}