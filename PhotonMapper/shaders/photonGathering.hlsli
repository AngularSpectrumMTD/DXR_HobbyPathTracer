#ifndef __PHOTONGATHERING_HLSLI__
#define __PHOTONGATHERING_HLSLI__

#include "opticalFunction.hlsli"

static float3 AxisX = float3(1, 0, 0);
static float3 AxisY = float3(0, 1, 0);
static float3 AxisZ = float3(0, 0, 1);

bool isPhotonStored(in PhotonPayload payload)
{
    return (payload.stored == 1);
}

void storePhoton(inout PhotonPayload payload, bool isMiss = false)
{
    bool ignore = isMiss || (isVisualizeLightRange() ? false : (payload.recursive <= 1));
    uint3 dispatchDimensions = DispatchRaysDimensions();

    if (ignore)
    {
        PhotonInfo photon;
        photon.throughput = float3(0, 0, 0);
        photon.position = float3(0, 0, 0);
        //photon.inDir = WorldRayDirection();
        gPhotonMap[payload.storeIndex] = photon;
        payload.stored = 1;
    }
    else
    {
        PhotonInfo photon;
        photon.throughput = payload.throughput;
        photon.position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        //photon.inDir = WorldRayDirection();
        gPhotonMap[payload.storeIndex] = photon;
        payload.stored = 1;
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
    alpha *= 4 * alpha / PI;
    float tmp = maxd * maxd - distance * distance;
    tmp = tmp * tmp * tmp;
    return alpha * tmp;
}

float3 accumulatePhotonHGC(float3 gatherCenterPos, float3 eyeDir, float3 worldNormal, bool isDebug = false)
{
    float gatherRadius = getGatherRadius();
    float sharp = getGatherSharpness();
    const int range = getGatherBlockRange();
    int rangeX = range;
    int rangeY = range;
    int rangeZ = range;
    bool isOutside = false;
    const int3 GridXYZ = (int3) ComputeGridCell(gatherCenterPos, isOutside);

    if (isOutside || !isApplyCaustics())
    {
        return float3(0, 0, 0);
    }

    int X = 0, Y = 0, Z = 0, G = 0;
    float3 accumulateXYZ = float3(0, 0, 0);

    float3 normWN = normalize(worldNormal);
    float3 normEYE = normalize(eyeDir);
    bool isEyeFlag = dot(normEYE, normWN) > 0;

    //Search Near Cell
    for (Z = max(GridXYZ.z - rangeZ, 0); Z <= min(GridXYZ.z + rangeZ, gGridParam.gridDimensions.z - 1); Z++)
    {
        for (Y = max(GridXYZ.y - rangeY, 0); Y <= min(GridXYZ.y + rangeY, gGridParam.gridDimensions.y - 1); Y++)
        {
            for (X = max(GridXYZ.x - rangeX, 0); X <= min(GridXYZ.x + rangeX, gGridParam.gridDimensions.x - 1); X++)
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
                    if ((distanceSqr < gatherRadius * gatherRadius))
                    {
                        accumulateXYZ += comparePhoton.throughput * poly6Kernel2D(sqrt(distanceSqr), gatherRadius);
                    }
                }

                if (isDebug)
                {
                    float db = 10 * (photonIDstardEnd.y - photonIDstardEnd.x + 1);
                    accumulateXYZ = float3(db, db, 0); //debug
                }
            }
        }
    }
    return accumulateXYZ;
}

float3 accumulatePhoton(float3 gatherCenterPos, float3 eyeDir, float3 worldNormal, bool isDebug = false)
{
    return accumulatePhotonHGC(gatherCenterPos, eyeDir, worldNormal, isDebug);
}

#endif//__PHOTONGATHERING_HLSLI__