#ifndef __RESERVOIR_HLSLI__
#define __RESERVOIR_HLSLI__

#define MAX_TEMPORAL_REUSE_M 200
#define MAX_SPATIAL_REUSE_M 100

struct DIReservoir
{
    uint lightID;//light ID of most important light
    uint randomSeed;//replay
    float targetPDF; //weight of light
    float3 targetPDF_3f; //weight of light(float 3)
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    void initialize()
    {
        lightID = 0;
        randomSeed = 0;
        targetPDF = 0;
        targetPDF_3f = 0.xxx;
        W_sum = 0;
        M = 0;
    }
};

bool updateDIReservoir(inout DIReservoir reservoir, in uint inLightID, in uint randomSeed, in float w, in float p_hat, in float3 p_hat_3f, in uint c, in float rnd01)
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
    return reservoir.targetPDF_3f * invPDF;
}

bool isValidReservoir(in DIReservoir reservoir)
{
    return (reservoir.M > 0);
}

void recognizeAsShadowedReservoir(inout DIReservoir reservoir)
{
    reservoir.targetPDF_3f = 0.xxx;
}

#endif//__RESERVOIR_HLSLI__