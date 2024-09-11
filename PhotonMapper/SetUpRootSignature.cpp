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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 14);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 15);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 16);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 17);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 18);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 19);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSig = rsCreater.Create(mDevice, false, L"RootSignatureGlobal");
        mRegisterMapGlobalRootSig.clear();
        mRegisterMapGlobalRootSig["gGridParam"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gBVH"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gEquiRecEnvMap"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gLightGenerateParams"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gSceneParam"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonMap"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gNormalDepthBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonGridIdBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPositionBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPrevIDBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gDIBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gGIBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gCausticsBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gDIReservoirBuffer"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gDISpatialReservoirBufferSrc"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonRandomCounterMap"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap0"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap1"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap2"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap3"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap4"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap5"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap6"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gScreenSpaceMaterial"] = mRegisterMapGlobalRootSig.size();
        mRegisterMapGlobalRootSig["gDebugTexture"] = mRegisterMapGlobalRootSig.size();
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 14);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 15);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 16);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 17);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 18);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 19);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSigPhoton = rsCreater.Create(mDevice, false, L"RootSignatureGlobalPhoton");
        mRegisterMapGlobalRootSigPhoton.clear();
        mRegisterMapGlobalRootSigPhoton["gGridParam"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gBVH"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gEquiRecEnvMap"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gLightGenerateParams"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gSceneParam"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonMap"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gNormalDepthBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonGridIdBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPositionBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPrevIDBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gDIBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gGIBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gCausticsBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gDIReservoirBuffer"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gDISpatialReservoirBufferSrc"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonRandomCounterMap"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap0"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap1"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap2"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap3"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap4"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap5"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap6"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gScreenSpaceMaterial"] = mRegisterMapGlobalRootSigPhoton.size();
        mRegisterMapGlobalRootSigPhoton["gDebugTexture"] = mRegisterMapGlobalRootSigPhoton.size();
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 14);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 15);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 16);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 17);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 18);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 19);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 2);
        rsCreater.PushStaticSampler(0);
        mGlobalRootSigReservoirSpatialReuse = rsCreater.Create(mDevice, false, L"RootSignatureGlobalReservoirSpatialReuse");
        mRegisterMapGlobalRootSigReservoirSpatialReuse.clear();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gGridParam"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gBVH"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gEquiRecEnvMap"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gLightGenerateParams"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gSceneParam"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonMap"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gNormalDepthBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonGridIdBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPositionBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPrevIDBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gGIBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gCausticsBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIReservoirBuffer"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDISpatialReservoirBufferSrc"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonRandomCounterMap"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap0"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap1"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap2"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap3"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap4"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap5"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap6"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gScreenSpaceMaterial"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDebugTexture"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gReSTIRParam"] = mRegisterMapGlobalRootSigReservoirSpatialReuse.size();
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
        mRegisterMapGlobalLocalRootSigMaterial.clear();
        mRegisterMapGlobalLocalRootSigMaterial["constantBuffer"] = mRegisterMapGlobalLocalRootSigMaterial.size();
        mRegisterMapGlobalLocalRootSigMaterial["indexBuffer"] = mRegisterMapGlobalLocalRootSigMaterial.size();
        mRegisterMapGlobalLocalRootSigMaterial["vertexBuffer"] = mRegisterMapGlobalLocalRootSigMaterial.size();
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
        mRegisterMapGlobalLocalRootSigMaterialWithTex.clear();
        mRegisterMapGlobalLocalRootSigMaterialWithTex["constantBuffer"] = mRegisterMapGlobalLocalRootSigMaterialWithTex.size();
        mRegisterMapGlobalLocalRootSigMaterialWithTex["indexBuffer"] = mRegisterMapGlobalLocalRootSigMaterialWithTex.size();
        mRegisterMapGlobalLocalRootSigMaterialWithTex["vertexBuffer"] = mRegisterMapGlobalLocalRootSigMaterialWithTex.size();
        mRegisterMapGlobalLocalRootSigMaterialWithTex["diffuseTex"] = mRegisterMapGlobalLocalRootSigMaterialWithTex.size();
        mRegisterMapGlobalLocalRootSigMaterialWithTex["alphaMask"] = mRegisterMapGlobalLocalRootSigMaterialWithTex.size();
    }
}