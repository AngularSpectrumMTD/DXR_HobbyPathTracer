#include "photonGathering.hlsli"
#include "opticalFunction.hlsli"

struct VertexPN
{
    float3 Position;
    float3 Normal;
};
struct MaterialCB
{
    float4 diffuseColor;
    float4 specular;
};

StructuredBuffer<uint> indexBuffer : register(t0, space1);
StructuredBuffer<VertexPN> vertexBuffer : register(t1, space1);

VertexPN GetVertex(TriangleIntersectionAttributes attrib)
{
    VertexPN v = (VertexPN) 0;
    uint start = PrimitiveIndex() * 3; // Triangle List.

    float3 positionTbl[3], normalTbl[3];
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer[start + i];
        positionTbl[i] = vertexBuffer[index].Position;
        normalTbl[i] = vertexBuffer[index].Normal;
    }

    v.Position = ComputeInterpolatedAttributeF3(positionTbl, attrib.barys);
    v.Normal = ComputeInterpolatedAttributeF3(normalTbl, attrib.barys);
    v.Normal = normalize(v.Normal);
    return v;
}

[shader("closesthit")]
void reflectReflactMaterialClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }

    VertexPN vtx = GetVertex(attrib);

    depthPositionNormalStore(payload, vtx.Normal);

    uint instanceID = InstanceID();
    if (instanceID == 0) {
        payload.color = Reflection(vtx.Position, vtx.Normal, payload.recursive, payload.eyeDir, payload.weight);
    }
    else if(instanceID == 1)
    {
        payload.color = Refraction(vtx.Position, vtx.Normal, payload.recursive, payload.eyeDir, payload.weight);
    }
}

[shader("closesthit")]
void reflectReflactMaterialStorePhotonClosestHit(inout PhotonPayload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPhotonPayload(payload) || isPhotonStored(payload) || isOverSplitted(payload))
    {
        return;
    }
    VertexPN vtx = GetVertex(attrib);

    uint instanceID = InstanceID();

    if (instanceID == 0) {
        ReflectionPhoton(vtx.Position, vtx.Normal, payload);
    }
    else if(instanceID == 1)
    {
        RefractionPhoton(vtx.Position, vtx.Normal, payload);
    }
}

[shader("closesthit")]
void reflectReflactMaterialDummyClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    //no op
}
