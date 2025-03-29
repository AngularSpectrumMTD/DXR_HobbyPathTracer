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

bool isUseStreamingRIS()
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

bool isUseReservoirSpatialReuse()
{
    return gSceneParam.additional1.y == 1;
}

bool isUseMetallicTest()
{
    return gSceneParam.additional1.z == 1;
}

bool isHistoryResetRequested()
{
    return gSceneParam.additional1.w == 1;
}

bool isUseIBL()
{
    return gSceneParam.additional2.x == 1;
}

bool isUseEmissivePolygon()
{
    return gSceneParam.additional2.y == 1;
}

bool isUseMedianFiltering()
{
    return gSceneParam.additional2.z == 1;
}

float meanFreePath()
{
    return gSceneParam.sssParam.x;
}

float exposure()
{
    return gSceneParam.toneMappingParam.x;
}

bool isTemporalReprojectionSuccessed(
    in float currDepth, in float prevDepth, 
    in float3 currNormal, in float3 prevNormal,
    in float3 currPos, in float3 prevPos)
{
    const float3 cameraPos = mul(gSceneParam.mtxViewInv, float4(0, 0, 0, 1)).xyz;
    const bool isNearDepth = ((currDepth * 0.99 < prevDepth) && (prevDepth < currDepth * 1.01)) && (currDepth > 0) && (prevDepth > 0);
    const bool isNearNormal = dot(currNormal, prevNormal) > 0.99 || ((length(currNormal) == 0) && (length(prevNormal) == 0));
    //const bool isNearPosition = (sqrt(dot(currPos - prevPos, currPos - prevPos)) < 0.1f * sqrt(dot(cameraPos - currPos, cameraPos - currPos)));
    return isNearDepth && isNearNormal;// && (length(velocity) < 1.0);
}

int2 computeTemporalReprojectedID(in int2 currID, in float2 velocity, in float2 targetBufferSize)
{
    return float2(currID.x, currID.y) - velocity * float2(0.5, -0.5) * targetBufferSize;
}

#endif//__SCENE_PARAM_INTERFACE_HLSLI__