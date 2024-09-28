#ifndef __RESERVOIR_HLSLI__
#define __RESERVOIR_HLSLI__

#define MAX_TEMPORAL_REUSE_M 200
#define MAX_SPATIAL_REUSE_M 100

#include "compressionUtility.hlsli"
#include "materialParams.hlsli"

struct DIReservoir
{
    uint lightID;//light ID of most important light
    uint randomSeed;//replay
    float targetPDF; //weight of light
    uint targetPDF_3f; //weight of light(float 3)
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    void initialize()
    {
        lightID = 0;
        randomSeed = 0;
        targetPDF = 0;
        targetPDF_3f = 0;
        W_sum = 0;
        M = 0;
    }
};

bool updateDIReservoir(inout DIReservoir reservoir, in uint inLightID, in uint randomSeed, in float w, in float p_hat, in uint p_hat_3f, in uint c, in float rnd01)
{
    reservoir.W_sum += w;
    reservoir.M += c;

    if ((rnd01 < (w / reservoir.W_sum)) || reservoir.M == 0)
    {
        reservoir.lightID = inLightID;
        reservoir.targetPDF = p_hat;
        reservoir.targetPDF_3f = p_hat_3f;
        reservoir.randomSeed = randomSeed;
        return true;
    }
    return false;
}

bool combineDIReservoirs(inout DIReservoir reservoir, in DIReservoir reservoirCombineElem, in float w, in float rnd01)
{
    return updateDIReservoir(reservoir, reservoirCombineElem.lightID, reservoirCombineElem.randomSeed, w, reservoirCombineElem.targetPDF, reservoirCombineElem.targetPDF_3f, reservoirCombineElem.M, rnd01);
}

float3 shadeDIReservoir(in DIReservoir reservoir)
{
    if(reservoir.M == 0)
    {
        return 0.xxx;
    }

    const float invPDF = max(0, reservoir.W_sum / (reservoir.M * reservoir.targetPDF));
    return U32toF32x3(reservoir.targetPDF_3f) * invPDF;
}

bool isValidReservoir(in DIReservoir reservoir)
{
    return (reservoir.M > 0);
}

void recognizeAsShadowedReservoir(inout DIReservoir reservoir)
{
    reservoir.targetPDF_3f = 0;
}

struct GIReservoir
{
    uint randomSeed;//replay
    float targetPDF; //weight of light
    uint targetPDF_3f; //weight of light(float 3)
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    uint Lo_2nd;
    float3 pos_2nd;
    float3 nml_2nd;

    void initialize()
    {
        randomSeed = 0;
        targetPDF = 0;
        targetPDF_3f = 0;
        W_sum = 0;
        M = 0;

        Lo_2nd = 0u;
        pos_2nd = 0.xxx;
        nml_2nd = 0.xxx;
    }
};

bool updateGIReservoir(inout GIReservoir reservoir, in uint randomSeed, in float w, in float p_hat, in uint p_hat_3f, in uint Lo_2nd, in float3 pos_2nd, in float3 nml_2nd, in uint c, in float rnd01)
{
    reservoir.W_sum += w;
    reservoir.M += c;

    if ((rnd01 < (w / reservoir.W_sum)) || reservoir.M == 0)
    {
        reservoir.targetPDF = p_hat;
        reservoir.targetPDF_3f = p_hat_3f;
        reservoir.randomSeed = randomSeed;
        reservoir.Lo_2nd = Lo_2nd;
        reservoir.pos_2nd = pos_2nd;
        reservoir.nml_2nd = nml_2nd;
        return true;
    }
    return false;
}

bool combineGIReservoirs(inout GIReservoir reservoir, in GIReservoir reservoirCombineElem, in float w, in float rnd01)
{
    return updateGIReservoir(reservoir, reservoirCombineElem.randomSeed, w, reservoirCombineElem.targetPDF, reservoirCombineElem.targetPDF_3f, reservoirCombineElem.Lo_2nd, reservoirCombineElem.nml_2nd, reservoirCombineElem.pos_2nd, reservoirCombineElem.M, rnd01);
}

float3 shadeGIReservoir(in GIReservoir reservoir)
{
    if(reservoir.M == 0)
    {
        return 0.xxx;
    }

    const float invPDF = max(0, reservoir.W_sum / (reservoir.M * reservoir.targetPDF));
    return U32toF32x3(reservoir.targetPDF_3f) * invPDF;
}

bool isValidReservoir(in GIReservoir reservoir)
{
    return (reservoir.M > 0);
}

void recognizeAsShadowedReservoir(inout GIReservoir reservoir)
{
    reservoir.targetPDF_3f = 0;
}

#endif//__RESERVOIR_HLSLI__