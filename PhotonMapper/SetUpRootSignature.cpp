#include "DxrPhotonMapper.h"

#include "AppInvoker.h"

#include <fstream>
#include <random>
#include <DirectXTex.h>

#include <wincodec.h>
#include "utility/Utility.h"

void DxrPhotonMapper::CreateRootSignatureGlobal()
{
    //OrdinalRaytracing
    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 5);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 6);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 7);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 8);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 9);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 10);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSig = rsCreater.Create(mDevice, false, L"RootSignatureGlobal");
        mRegisterMapGlobalRootSig["gGridParam"] = 0;
        mRegisterMapGlobalRootSig["gBVH"] = 1;
        mRegisterMapGlobalRootSig["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSig["gLightGenerateParams"] = 3;
        mRegisterMapGlobalRootSig["gSceneParam"] = 4;
        mRegisterMapGlobalRootSig["gPhotonMap"] = 5;
        mRegisterMapGlobalRootSig["gDepthBuffer"] = 6;
        mRegisterMapGlobalRootSig["gPhotonGridIdBuffer"] = 7;
        mRegisterMapGlobalRootSig["gDiffuseAlbedoBuffer"] = 8;
        mRegisterMapGlobalRootSig["gPositionBuffer"] = 9;
        mRegisterMapGlobalRootSig["gNormalBuffer"] = 10;
        mRegisterMapGlobalRootSig["gVelocityBuffer"] = 11;
        mRegisterMapGlobalRootSig["gDIBuffer"] = 12;
        mRegisterMapGlobalRootSig["gGIBuffer"] = 13;
        mRegisterMapGlobalRootSig["gCausticsBuffer"] = 14;
        mRegisterMapGlobalRootSig["gDIReservoirBuffer"] = 15;
    }

    //PhotonMapping
    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 5);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 6);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 7);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 8);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 9);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 10);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSigPhoton = rsCreater.Create(mDevice, false, L"RootSignatureGlobalPhoton");
        mRegisterMapGlobalRootSigPhoton["gGridParam"] = 0;
        mRegisterMapGlobalRootSigPhoton["gBVH"] = 1;
        mRegisterMapGlobalRootSigPhoton["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSigPhoton["gLightGenerateParams"] = 3;
        mRegisterMapGlobalRootSigPhoton["gSceneParam"] = 4;
        mRegisterMapGlobalRootSigPhoton["gPhotonMap"] = 5;
        mRegisterMapGlobalRootSigPhoton["gDepthBuffer"] = 6;
        mRegisterMapGlobalRootSigPhoton["gPhotonGridIdBuffer"] = 7;
        mRegisterMapGlobalRootSigPhoton["gDiffuseAlbedoBuffer"] = 8;
        mRegisterMapGlobalRootSigPhoton["gPositionBuffer"] = 9;
        mRegisterMapGlobalRootSigPhoton["gNormalBuffer"] = 10;
        mRegisterMapGlobalRootSigPhoton["gVelocityBuffer"] = 11;
        mRegisterMapGlobalRootSigPhoton["gDIBuffer"] = 12;
        mRegisterMapGlobalRootSigPhoton["gGIBuffer"] = 13;
        mRegisterMapGlobalRootSigPhoton["gCausticsBuffer"] = 14;
        mRegisterMapGlobalRootSigPhoton["gDIReservoirBuffer"] = 15;
    }
}

void DxrPhotonMapper::CreateRootSignatureLocal()
{
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        mLocalRootSigMaterial = rsCreater.Create(mDevice, true, L"lrsMaterial");
        mRegisterMapGlobalLocalRootSigMaterial["constantBuffer"] = 0;
        mRegisterMapGlobalLocalRootSigMaterial["indexBuffer"] = 1;
        mRegisterMapGlobalLocalRootSigMaterial["vertexBuffer"] = 2;
    }
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 3, regSpace);
        mLocalRootSigMaterialWithTex = rsCreater.Create(mDevice, true, L"lrsMaterialWithTex");
        mRegisterMapGlobalLocalRootSigMaterialWithTex["constantBuffer"] = 0;
        mRegisterMapGlobalLocalRootSigMaterialWithTex["indexBuffer"] = 1;
        mRegisterMapGlobalLocalRootSigMaterialWithTex["vertexBuffer"] = 2;
        mRegisterMapGlobalLocalRootSigMaterialWithTex["diffuseTex"] = 3;
        mRegisterMapGlobalLocalRootSigMaterialWithTex["alphaMask"] = 4;
    }
}