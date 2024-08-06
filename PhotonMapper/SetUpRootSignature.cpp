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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 11);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 12);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 13);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSig = rsCreater.Create(mDevice, false, L"RootSignatureGlobal");
        mRegisterMapGlobalRootSig["gGridParam"] = 0;
        mRegisterMapGlobalRootSig["gBVH"] = 1;
        mRegisterMapGlobalRootSig["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSig["gLightGenerateParams"] = 3;
        mRegisterMapGlobalRootSig["gSceneParam"] = 4;
        mRegisterMapGlobalRootSig["gPhotonMap"] = 5;
        mRegisterMapGlobalRootSig["gNormalDepthBuffer"] = 6;
        mRegisterMapGlobalRootSig["gPhotonGridIdBuffer"] = 7;
        mRegisterMapGlobalRootSig["gDiffuseAlbedoBuffer"] = 8;
        mRegisterMapGlobalRootSig["gPositionBuffer"] = 9;
        mRegisterMapGlobalRootSig["gIDRoughnessBuffer"] = 10;
        mRegisterMapGlobalRootSig["gPrevIDBuffer"] = 11;
        mRegisterMapGlobalRootSig["gDIBuffer"] = 12;
        mRegisterMapGlobalRootSig["gGIBuffer"] = 13;
        mRegisterMapGlobalRootSig["gCausticsBuffer"] = 14;
        mRegisterMapGlobalRootSig["gDIReservoirBuffer"] = 15;
        mRegisterMapGlobalRootSig["gDISpatialReservoirBufferSrc"] = 16;
        mRegisterMapGlobalRootSig["gPhotonRandomCounterMap"] = 17;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap"] = 18;
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 11);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 12);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 13);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSigPhoton = rsCreater.Create(mDevice, false, L"RootSignatureGlobalPhoton");
        mRegisterMapGlobalRootSigPhoton["gGridParam"] = 0;
        mRegisterMapGlobalRootSigPhoton["gBVH"] = 1;
        mRegisterMapGlobalRootSigPhoton["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSigPhoton["gLightGenerateParams"] = 3;
        mRegisterMapGlobalRootSigPhoton["gSceneParam"] = 4;
        mRegisterMapGlobalRootSigPhoton["gPhotonMap"] = 5;
        mRegisterMapGlobalRootSigPhoton["gNormalDepthBuffer"] = 6;
        mRegisterMapGlobalRootSigPhoton["gPhotonGridIdBuffer"] = 7;
        mRegisterMapGlobalRootSigPhoton["gDiffuseAlbedoBuffer"] = 8;
        mRegisterMapGlobalRootSigPhoton["gPositionBuffer"] = 9;
        mRegisterMapGlobalRootSigPhoton["gIDRoughnessBuffer"] = 10;
        mRegisterMapGlobalRootSigPhoton["gPrevIDBuffer"] = 11;
        mRegisterMapGlobalRootSigPhoton["gDIBuffer"] = 12;
        mRegisterMapGlobalRootSigPhoton["gGIBuffer"] = 13;
        mRegisterMapGlobalRootSigPhoton["gCausticsBuffer"] = 14;
        mRegisterMapGlobalRootSigPhoton["gDIReservoirBuffer"] = 15;
        mRegisterMapGlobalRootSigPhoton["gDISpatialReservoirBufferSrc"] = 16;
        mRegisterMapGlobalRootSigPhoton["gPhotonRandomCounterMap"] = 17;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap"] = 18;
    }
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 11);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 12);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 13);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 2);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSigReservoirSpatialReuse = rsCreater.Create(mDevice, false, L"RootSignatureGlobalReservoirSpatialReuse");
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gGridParam"] = 0;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gBVH"] = 1;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gEquiRecEnvMap"] = 2;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gLightGenerateParams"] = 3;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gSceneParam"] = 4;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonMap"] = 5;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gNormalDepthBuffer"] = 6;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonGridIdBuffer"] = 7;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDiffuseAlbedoBuffer"] = 8;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPositionBuffer"] = 9;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gIDRoughnessBuffer"] = 10;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPrevIDBuffer"] = 11;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIBuffer"] = 12;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gGIBuffer"] = 13;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gCausticsBuffer"] = 14;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIReservoirBuffer"] = 15;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDISpatialReservoirBufferSrc"] = 16;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonRandomCounterMap"] = 17;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap"] = 18;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gReSTIRParam"] = 19;
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