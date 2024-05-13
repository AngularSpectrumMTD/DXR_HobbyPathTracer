#ifndef __SCENE_CB_DEFINITION_HLSLI__
#define __SCENE_CB_DEFINITION_HLSLI__

struct SceneCB
{
    matrix mtxView;
    matrix mtxProj;
    matrix mtxViewInv;
    matrix mtxProjInv;
    matrix mtxViewPrev;
    matrix mtxProjPrev;
    matrix mtxViewInvPrev;
    matrix mtxProjInvPrev;
    uint4 flags;
    float4 photonParams;
    float4 cameraParams;
    float4 gatherParams;
    float4 gatherParams2;
    float4 spotLightParams;
    float4 viewVec;
    uint4 additional;
    uint4 additional1;
};

#endif//__SCENE_CB_DEFINITION_HLSLI__