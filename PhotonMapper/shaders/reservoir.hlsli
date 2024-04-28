#ifndef __RESERVOIR_HLSLI__
#define __RESERVOIR_HLSLI__

struct Reservoir
{
    uint Y; //index of most important light
    float targetPDF; //weight of light
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    void initialize()
    {
        Y = 0;
        targetPDF = 0;
        W_sum = 0;
        M = 0;
    }
};

bool updateReservoir(inout Reservoir reservoir, in uint X, in float w, in float p_hat, in uint c, in float rnd01)
{
    reservoir.W_sum += w;
    reservoir.M += c;

    if (rnd01 < w / reservoir.W_sum)
    {
        reservoir.Y = X;
        reservoir.targetPDF = p_hat;
        return true;
    }
    return false;
}

#endif//__RESERVOIR_HLSLI__