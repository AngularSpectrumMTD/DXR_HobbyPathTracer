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
        diffuseColor = float4(0.1, 0.1, 0.1 , 1);
    }
    
    uint instanceID = InstanceID();
    
    float3 color = diffuseColor.xyz;
    float3 reflectColor = Reflection(vtx.Position, vtx.Normal, payload.recursive, payload.eyeDir, payload.weight);

    bool isSpecular = (instanceID == 0);
    
    payload.color = lerp(reflectColor, color, isSpecular ? 0.8 : 1.0);
    float3 worldNormal = mul(vtx.Normal, (float3x3) ObjectToWorld4x3());

#ifdef SHADOW_PASS_ENABLE
    if(isInShadow(vtx.Position, vtx.Normal, worldNormal))
    {
        payload.color.xyz *= 0.5;
    }
#endif

    {//Apply Caustics
        if(payload.weight > 0)
        {
            payload.photonColor.xyz += payload.weight + photonGather(WorldRayOrigin() + WorldRayDirection() * RayTCurrent(), payload.eyeDir, worldNormal);
        }
        else
        {
            payload.photonColor.xyz += photonGather(WorldRayOrigin() + WorldRayDirection() * RayTCurrent(), payload.eyeDir, worldNormal);
        }

        //for seeing througth Object
        if(payload.recursive >= 2)
        {
            if(payload.weight > 0)
            {
                payload.color.xyz += payload.weight * payload.photonColor.xyz;
            }
            else{
                payload.color.xyz += payload.photonColor.xyz;
            }
            
        }
    }
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
