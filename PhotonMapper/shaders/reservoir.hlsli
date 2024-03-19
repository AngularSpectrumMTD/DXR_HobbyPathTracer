#ifndef __RESERVOIR_HLSLI__
#define __RESERVOIR_HLSLI__

struct Reservoir
{
    uint Y; //index of most important light
    float W_y; //weight of light
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    void initialize()
    {
        Y = 0;
        W_y = 0;
        W_sum = 0;
        M = 0;
    }
};

bool updateReservoir(inout Reservoir reservoir, in uint X, in float w, in uint c, in float rnd01)
{
    reservoir.W_sum += w;
    reservoir.M += c;

    if (rnd01 < w / reservoir.W_sum)
    {
        reservoir.Y = X;
        return true;
    }
    return false;
}

#endif//__RESERVOIR_HLSLI__