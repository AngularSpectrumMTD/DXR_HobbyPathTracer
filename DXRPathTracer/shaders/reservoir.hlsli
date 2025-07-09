#ifndef __RESERVOIR_HLSLI__
#define __RESERVOIR_HLSLI__

#define MAX_REUSE_M_DI 15
#define MAX_REUSE_M_GI 15

#include "compressionUtility.hlsli"
#include "materialParams.hlsli"

struct DIReservoir
{
    uint lightID;//light ID of most important light
    uint randomSeed;//replay(must be setted before the invocation of sampleLightWithID())
    float targetPDF; //weight of light
    uint targetPDF_3f_U32; //weight of light(float 3)
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    void initialize()
    {
        lightID = 0;
        randomSeed = 0;
        targetPDF = 0;
        targetPDF_3f_U32 = 0;
        W_sum = 0;
        M = 0;
    }

    void applyMCapping()
    {
        if(M > MAX_REUSE_M_DI)
        {
            float r = max(0, ((float)MAX_REUSE_M_DI / M));
            W_sum *= r;
            M = MAX_REUSE_M_DI;
        }
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
        reservoir.targetPDF_3f_U32 = p_hat_3f;
        reservoir.randomSeed = randomSeed;
        return true;
    }
    return false;
}

bool combineDIReservoirs(inout DIReservoir reservoir, in DIReservoir reservoirCombineElem, in float w, in float rnd01)
{
    return updateDIReservoir(reservoir, reservoirCombineElem.lightID, reservoirCombineElem.randomSeed, w, reservoirCombineElem.targetPDF, reservoirCombineElem.targetPDF_3f_U32, reservoirCombineElem.M, rnd01);
}

float3 resolveDIReservoir(in DIReservoir reservoir)
{
    if(reservoir.M == 0)
    {
        return 0.xxx;
    }

    const float invPDF = max(0, reservoir.W_sum / (reservoir.M * reservoir.targetPDF));
    return decompressU32asRGB(reservoir.targetPDF_3f_U32) * invPDF;
}

bool isValidReservoir(in DIReservoir reservoir)
{
    return (reservoir.M > 0);
}

void recognizeAsShadowedReservoir(inout DIReservoir reservoir)
{
    reservoir.targetPDF_3f_U32 = 0;
}

struct GISample
{
    uint Lo_2nd_U32;
    float3 pos_2nd;
    float3 nml_2nd;
};

struct GIReservoir
{
    uint randomSeed;//replay(must be setted before the invocation of computeBSDF_PDF())
    float targetPDF; //weight of light
    uint targetPDF_3f_U32; //weight of light(float 3)
    float W_sum; //sum of all weight
    float M; //number of ligts processed for this reservoir

    GISample giSample;
    CompressedMaterialParams compressedMaterial;

    void initialize()
    {
        randomSeed = 0;
        targetPDF = 0;
        targetPDF_3f_U32 = 0;
        W_sum = 0;
        M = 0;

        giSample = (GISample)0;
        compressedMaterial = (CompressedMaterialParams)0;
    }

    void applyMCapping()
    {
        if(M > MAX_REUSE_M_GI)
        {
            float r = max(0, ((float)MAX_REUSE_M_GI / M));
            W_sum *= r;
            M = MAX_REUSE_M_GI;
        }
    }
};

bool updateGIReservoir(inout GIReservoir reservoir, in uint randomSeed, in float w, in float p_hat, in uint p_hat_3f, in GISample giSample, in CompressedMaterialParams compressedMaterial, in uint c, in float rnd01)
{
    reservoir.W_sum += w;
    reservoir.M += c;

    if ((rnd01 < (w / reservoir.W_sum)) || reservoir.M == 0)
    {
        reservoir.targetPDF = p_hat;
        reservoir.targetPDF_3f_U32 = p_hat_3f;
        reservoir.randomSeed = randomSeed;
        reservoir.giSample = giSample;
        reservoir.compressedMaterial = compressedMaterial;
        return true;
    }
    return false;
}

bool combineGIReservoirs(inout GIReservoir reservoir, in GIReservoir reservoirCombineElem, in float w, in float rnd01)
{
    return updateGIReservoir(reservoir, reservoirCombineElem.randomSeed, w, reservoirCombineElem.targetPDF, reservoirCombineElem.targetPDF_3f_U32, reservoirCombineElem.giSample, reservoirCombineElem.compressedMaterial  , reservoirCombineElem.M, rnd01);
}

float3 resolveGIReservoir(in GIReservoir reservoir)
{
    if(reservoir.M == 0)
    {
        return 0.xxx;
    }

    const float invPDF = max(0, reservoir.W_sum / (reservoir.M * reservoir.targetPDF));
    return decompressU32asRGB(reservoir.targetPDF_3f_U32) * invPDF;
}

bool isValidReservoir(in GIReservoir reservoir)
{
    return (reservoir.M > 0);
}

void recognizeAsShadowedReservoir(inout GIReservoir reservoir)
{
    reservoir.targetPDF_3f_U32 = 0;
}

#endif//__RESERVOIR_HLSLI__