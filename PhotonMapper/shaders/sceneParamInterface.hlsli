#ifndef __SCENE_PARAM_INTERFACE_HLSLI__
#define __SCENE_PARAM_INTERFACE_HLSLI__

////////////////////////////////////
//Interpret Scene Param
////////////////////////////////////
bool isDirectionalLight()
{
    return gSceneParam.flags.x == 0;
}

bool isUseTexture()
{
    return gSceneParam.flags.y == 1;
}

bool isVisualizePhotonDebugDraw()
{
    return gSceneParam.flags.z == 1;
}

bool isVisualizeLightRange()
{
    return gSceneParam.flags.w == 1;
}

uint getLightNum()
{
    return gSceneParam.additional.x;
}

bool isIndirectOnly()
{
    return gSceneParam.additional.y == 1;
}

bool isUseNEE()
{
    return gSceneParam.additional.z == 1;
}

bool isUseWRS_RIS()
{
    return gSceneParam.additional.w == 1;
}

bool isApplyCaustics()
{
    return gSceneParam.photonParams.x == 1;
}

float getSpectrumMode()
{
    return gSceneParam.photonParams.z;
}

float getMaxPhotonBounceNum()
{
    return gSceneParam.photonParams.w;
}

float getNearPlaneDistance()
{
    return gSceneParam.cameraParams.x;
}

float getFarPlaneDistance()
{
    return gSceneParam.cameraParams.y;
}

float getMaxBounceNum()
{
    return gSceneParam.cameraParams.z;
}

float getGatherRadius()
{
    return gSceneParam.gatherParams.x;
}

float getGatherSharpness()
{
    return gSceneParam.gatherParams.y;
}

float getGatherBlockRange()
{
    return gSceneParam.gatherParams.w;
}

uint getPhotonUnitNum()
{
    return gSceneParam.gatherParams2.x;
}

bool isAccumulationApply()
{
    return (gSceneParam.gatherParams2.y == 1);
}

float getLightRange()
{
    return gSceneParam.spotLightParams.x;
}

float getLightRandomSeed()
{
    return gSceneParam.spotLightParams.y;
}

float getLightLambdaNum()
{
    return gSceneParam.spotLightParams.z;
}

float getCausticsBoost()
{
    return gSceneParam.spotLightParams.w;
}

float3 getViewVec()
{
    return gSceneParam.viewVec.xyz;
}

bool isUseReservoirTemporalReuse()
{
    return gSceneParam.additional1.x == 1;
}

#endif//__SCENE_PARAM_INTERFACE_HLSLI__