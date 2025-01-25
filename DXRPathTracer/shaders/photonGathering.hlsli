#ifndef __PHOTONGATHERING_HLSLI__
#define __PHOTONGATHERING_HLSLI__

#include "opticalFunction.hlsli"

static float3 AxisX = float3(1, 0, 0);
static float3 AxisY = float3(0, 1, 0);
static float3 AxisZ = float3(0, 0, 1);

bool isPhotonStored(in PhotonPayload payload)
{
    return (payload.flags & PHOTON_PAYLOAD_BIT_MASK_IS_PHOTON_STORED);
}

void storePhoton(inout PhotonPayload payload)
{
    const bool ignore = (isVisualizeLightRange() ? false : (payload.recursive <= 1));

    if (ignore)
    {
        PhotonInfo photon;
        photon.terminate();
        photon.position = float3(0, 0, 0);
        //photon.inDir = WorldRayDirection();
        gPhotonMap[serialRaysIndex(DispatchRaysIndex(), DispatchRaysDimensions())] = photon;
        payload.flags |= PHOTON_PAYLOAD_BIT_MASK_IS_PHOTON_STORED;
    }
    else
    {
        PhotonInfo photon;
        photon.throughputU32 = payload.throughputU32;
        photon.position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        //photon.inDir = WorldRayDirection();
        gPhotonMap[serialRaysIndex(DispatchRaysIndex(), DispatchRaysDimensions())] = photon;
        payload.flags |= PHOTON_PAYLOAD_BIT_MASK_IS_PHOTON_STORED;

        //guiding
        if(isPrimarySurfaceHasHighPossibilityCausticsGenerate(payload))
        {
            float2 counterMapSize = 0.xx;
            gPhotonRandomCounterMap.GetDimensions(counterMapSize.x, counterMapSize.y);
            gPhotonRandomCounterMap[clamp(counterMapSize * payload.randomUV, 0.xx, counterMapSize -1.xx)] = 1;
        }
    }
}

float weightFunction(float distance)
{
    return exp(-distance * distance * getGatherSharpness()) / PI / distance / distance;
}

float poly6Kernel2D(float distance, float maxd)
{
    float alpha = 1 / maxd;
    alpha *= alpha;
    alpha *= alpha;
    alpha *= alpha;
    float tmp = maxd * maxd - distance * distance;
    tmp = tmp * tmp * tmp;
    return alpha * tmp * 4 / PI;
}

float3 accumulatePhotonHGC(float3 gatherCenterPos, float3 worldNormal, bool isDebug = false)
{
    bool isOutside = false;
    const int3 GridXYZ = (int3) ComputeGridCell(gatherCenterPos, isOutside);

    if (isOutside || !isApplyCaustics())
    {
        return float3(0, 0, 0);
    }

    const uint CurrGridID = GridHash(GridXYZ);

    int X = 0, Y = 0, Z = 0, G = 0;
    float3 accumulateXYZ = float3(0, 0, 0);
    
    uint2 IDstardEnd = gPhotonGridIdBuffer[CurrGridID];
    if ((IDstardEnd.x == UINT32_MAX) || (IDstardEnd.y == UINT32_MAX) || (IDstardEnd.y == IDstardEnd.x))
    {
        return float3(0, 0, 0);
    }

    //Search Near Cell
    for (Z = max(GridXYZ.z - getGatherBlockRange(), 0); Z <= min(GridXYZ.z + getGatherBlockRange(), gGridParam.gridDimensions.z - 1); Z++)
    {
        for (Y = max(GridXYZ.y - getGatherBlockRange(), 0); Y <= min(GridXYZ.y + getGatherBlockRange(), gGridParam.gridDimensions.y - 1); Y++)
        {
            for (X = max(GridXYZ.x - getGatherBlockRange(), 0); X <= min(GridXYZ.x + getGatherBlockRange(), gGridParam.gridDimensions.x - 1); X++)
            {
                uint3 XYZ = uint3(X, Y, Z);
                uint GridID = GridHash(XYZ);

                uint2 photonIDstardEnd = gPhotonGridIdBuffer[GridID];
                if ((photonIDstardEnd.x == UINT32_MAX) || (photonIDstardEnd.y == UINT32_MAX))
                    continue; //avoid infinite loop

                for (G = photonIDstardEnd.x; G <= photonIDstardEnd.y; G++)
                {
                    PhotonInfo comparePhoton = gPhotonMap[G];
                    float distanceSqr = dot(gatherCenterPos - comparePhoton.position, gatherCenterPos - comparePhoton.position);
                    if ((distanceSqr < getGatherRadius() * getGatherRadius()))
                    {
                        accumulateXYZ += decompressU32asRGB(comparePhoton.throughputU32) * poly6Kernel2D(sqrt(distanceSqr), getGatherRadius());
                    }
                }

                // if (isDebug)
                // {
                //     float db = 10 * (photonIDstardEnd.y - photonIDstardEnd.x + 1);
                //     accumulateXYZ = float3(db, db, 0); //debug
                // }
            }
        }
    }
    return accumulateXYZ;
}

float3 accumulatePhoton(float3 gatherCenterPos, float3 worldNormal, bool isDebug = false)
{
    return accumulatePhotonHGC(gatherCenterPos, worldNormal, isDebug);
}

#endif//__PHOTONGATHERING_HLSLI__