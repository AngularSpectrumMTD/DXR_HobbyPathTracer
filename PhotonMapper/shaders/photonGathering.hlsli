#ifndef __PHOTONGATHERING_HLSLI__
#define __PHOTONGATHERING_HLSLI__

#include "opticalFunction.hlsli"

bool isPhotonStored(in PhotonPayload payload)
{
    return (payload.stored == 1);
}

bool isOverSplitted(in PhotonPayload payload)
{
    return (payload.offsetCoef >= 2);
}

void storePhoton(inout PhotonPayload payload, bool isMiss = false)
{
    bool ignore = isMiss || (isVisualizeLightRange() ? false : (payload.recursive <= 1));
    uint3 dispatchDimensions = DispatchRaysDimensions();
    uint photonOffset = dispatchDimensions.x * dispatchDimensions.y;

    if (ignore)
    {
        PhotonInfo photon;
        photon.throughput = float3(0, 0, 0);
        photon.position = float3(0, 0, 0);
        photon.inDir = WorldRayDirection();
        gPhotonMap[payload.storeIndex + payload.offsetCoef * photonOffset] = photon;
        payload.stored = 1;
    }
    else
    {
        PhotonInfo photon;
        photon.throughput = (payload.recursive <= 1) ? payload.throughput : getCausticsBoost() * payload.throughput;
        photon.position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        photon.inDir = WorldRayDirection();
        gPhotonMap[payload.storeIndex + payload.offsetCoef * photonOffset] = photon;
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

float3 photonGatheringWithSortedHashGridCells(float3 gatherCenterPos, float3 eyeDir, float3 worldNormal)
{
    float gatherRadius = getGatherRadius();
    float sharp = getGatherSharpness();
    float boost = getGatherBoost();
    const int range = getGatherBlockRange();
    bool isOutside = false;
    const int3 GridXYZ = (int3) ComputeGridCell(gatherCenterPos, isOutside);

    if (isOutside || !isApplyCaustics())
    {
        return float3(0, 0, 0);
    }

    int X = 0, Y = 0, Z = 0, G = 0;
    float3 accumulateXYZ = float3(0, 0, 0);
    int count = 0;

    float3 normWN = normalize(worldNormal);
    float3 normEYE = normalize(eyeDir);
    bool isEyeFlag = dot(normEYE, normWN) > 0;

    //Search Near Cell
    for (Z = max(GridXYZ.z - range, 0); Z <= min(GridXYZ.z + range, gGridParam.gridDimensions.z - 1); Z++)
    {
        for (Y = max(GridXYZ.y - range, 0); Y <= min(GridXYZ.y + range, gGridParam.gridDimensions.y - 1); Y++)
        {
            for (X = max(GridXYZ.x - range, 0); X <= min(GridXYZ.x + range, gGridParam.gridDimensions.x - 1); X++)
            {
                uint3 XYZ = uint3(X, Y, Z);
                uint GridID = GridHash(XYZ);

                uint2 photonIDstardEnd = gPhotonGridIdBuffer[GridID];
                if (photonIDstardEnd.x == UINT32_MAX)
                    continue; //avoid infinite loop
                if (photonIDstardEnd.y == UINT32_MAX)
                    continue; //avoid infinite loop
                
                if (count < getPhotonUnitNum())
                {
                    for (G = photonIDstardEnd.x; G <= photonIDstardEnd.y; G++)
                    {
                        PhotonInfo comparePhoton = gPhotonMap[G];

                        float distance = length(gatherCenterPos - comparePhoton.position);
                        if ((distance < gatherRadius) &&
                         ((dot(normalize(comparePhoton.inDir), normWN) > 0) == isEyeFlag))//eliminate invisible photon effect
                        {
                            count++;
                            accumulateXYZ += comparePhoton.throughput * poly6Kernel2D(distance, gatherRadius);
                        }
                    }
                }

                if (isVisualizePhotonDebugDraw())
                {
                    float db = 10 * (photonIDstardEnd.y - photonIDstardEnd.x + 1);
                    accumulateXYZ += float3(db, db, 0); //debug
                }
            }
        }
    }

    if (accumulateXYZ.x < 0)
        accumulateXYZ.x = 0;
    if (accumulateXYZ.y < 0)
        accumulateXYZ.y = 0;
    if (accumulateXYZ.z < 0)
        accumulateXYZ.z = 0;

    uint photonMapWidth = 1;
    uint photonStride = 1;
    gPhotonMap.GetDimensions(photonMapWidth, photonStride);

    return boost * mul(accumulateXYZ, XYZtoRGB2) / photonMapWidth;
}

float3 photonGather(float3 gatherCenterPos, float3 eyeDir, float3 worldNormal)
{
    return photonGatheringWithSortedHashGridCells(gatherCenterPos, eyeDir, worldNormal);
}

#endif//__PHOTONGATHERING_HLSLI__