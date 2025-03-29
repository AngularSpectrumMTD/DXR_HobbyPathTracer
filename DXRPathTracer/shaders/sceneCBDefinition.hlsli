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
    float4 photonParams;//x unused
    float4 cameraParams;//near far reserved reserved
    float4 gatherParams;
    float4 gatherParams2;
    float4 spotLightParams;
    float4 viewVec;
    uint4 additional;
    uint4 additional1;
    uint4 additional2;
    float4 sssParam;
    float4 toneMappingParam;
};

#endif//__SCENE_CB_DEFINITION_HLSLI__