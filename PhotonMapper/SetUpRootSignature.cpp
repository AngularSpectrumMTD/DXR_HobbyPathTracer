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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 3);
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 11);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSig = rsCreater.Create(mDevice, false, L"RootSignatureGlobal");
        mRegisterMapGlobalRootSig["gGridParam"] = 0;
        mRegisterMapGlobalRootSig["gRtScene"] = 1;
        mRegisterMapGlobalRootSig["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSig["gLuminanceMomentBufferSrc"] = 3;
        mRegisterMapGlobalRootSig["gLightGenerateParams"] = 4;
        mRegisterMapGlobalRootSig["gSceneParam"] = 5;
        mRegisterMapGlobalRootSig["gPhotonMap"] = 6;
        mRegisterMapGlobalRootSig["gDepthBuffer"] = 7;
        mRegisterMapGlobalRootSig["gPrevDepthBuffer"] = 8;
        mRegisterMapGlobalRootSig["gPhotonGridIdBuffer"] = 9;
        mRegisterMapGlobalRootSig["gDiffuseAlbedoBuffer"] = 10;
        mRegisterMapGlobalRootSig["gPositionBuffer"] = 11;
        mRegisterMapGlobalRootSig["gNormalBuffer"] = 12;
        mRegisterMapGlobalRootSig["gOutput"] = 13;
        mRegisterMapGlobalRootSig["gAccumulationBuffer"] = 14;
        mRegisterMapGlobalRootSig["gLuminanceMomentBufferDst"] = 15;
        mRegisterMapGlobalRootSig["gAccumulationCountBuffer"] = 16;
        mRegisterMapGlobalRootSig["gVelocityBuffer"] = 17;
    }

    //PhotonMapping
    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 3);
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 11);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSigPhoton = rsCreater.Create(mDevice, false, L"RootSignatureGlobalPhoton");
        mRegisterMapGlobalRootSigPhoton["gGridParam"] = 0;
        mRegisterMapGlobalRootSigPhoton["gRtScene"] = 1;
        mRegisterMapGlobalRootSigPhoton["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSigPhoton["gLuminanceMomentBufferSrc"] = 3;
        mRegisterMapGlobalRootSigPhoton["gLightGenerateParams"] = 4;
        mRegisterMapGlobalRootSigPhoton["gSceneParam"] = 5;
        mRegisterMapGlobalRootSigPhoton["gPhotonMap"] = 6;
        mRegisterMapGlobalRootSigPhoton["gDepthBuffer"] = 7;
        mRegisterMapGlobalRootSigPhoton["gPrevDepthBuffer"] = 8;
        mRegisterMapGlobalRootSigPhoton["gPhotonGridIdBuffer"] = 9;
        mRegisterMapGlobalRootSigPhoton["gDiffuseAlbedoBuffer"] = 10;
        mRegisterMapGlobalRootSigPhoton["gPositionBuffer"] = 11;
        mRegisterMapGlobalRootSigPhoton["gNormalBuffer"] = 12;
        mRegisterMapGlobalRootSigPhoton["gOutput"] = 13;
        mRegisterMapGlobalRootSigPhoton["gAccumulationBuffer"] = 14;
        mRegisterMapGlobalRootSigPhoton["gLuminanceMomentBufferDst"] = 15;
        mRegisterMapGlobalRootSigPhoton["gAccumulationCountBuffer"] = 16;
        mRegisterMapGlobalRootSigPhoton["gVelocityBuffer"] = 17;
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