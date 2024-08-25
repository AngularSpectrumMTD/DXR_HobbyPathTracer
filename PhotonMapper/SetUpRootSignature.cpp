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
        mRegisterMapGlobalRootSig["gPositionBuffer"] = 8;
        mRegisterMapGlobalRootSig["gPrevIDBuffer"] = 9;
        mRegisterMapGlobalRootSig["gDIBuffer"] = 10;
        mRegisterMapGlobalRootSig["gGIBuffer"] = 11;
        mRegisterMapGlobalRootSig["gCausticsBuffer"] = 12;
        mRegisterMapGlobalRootSig["gDIReservoirBuffer"] = 13;
        mRegisterMapGlobalRootSig["gDISpatialReservoirBufferSrc"] = 14;
        mRegisterMapGlobalRootSig["gPhotonRandomCounterMap"] = 15;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap0"] = 16;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap1"] = 17;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap2"] = 18;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap3"] = 19;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap4"] = 20;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap5"] = 21;
        mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap6"] = 22;
        mRegisterMapGlobalRootSig["gScreenSpaceMaterial"] = 23;
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
        mRegisterMapGlobalRootSigPhoton["gPositionBuffer"] = 8;
        mRegisterMapGlobalRootSigPhoton["gPrevIDBuffer"] = 9;
        mRegisterMapGlobalRootSigPhoton["gDIBuffer"] = 10;
        mRegisterMapGlobalRootSigPhoton["gGIBuffer"] = 11;
        mRegisterMapGlobalRootSigPhoton["gCausticsBuffer"] = 12;
        mRegisterMapGlobalRootSigPhoton["gDIReservoirBuffer"] = 13;
        mRegisterMapGlobalRootSigPhoton["gDISpatialReservoirBufferSrc"] = 14;
        mRegisterMapGlobalRootSigPhoton["gPhotonRandomCounterMap"] = 15;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap0"] = 16;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap1"] = 17;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap2"] = 18;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap3"] = 19;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap4"] = 20;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap5"] = 21;
        mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap6"] = 22;
        mRegisterMapGlobalRootSigPhoton["gScreenSpaceMaterial"] = 23;
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
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPositionBuffer"] = 8;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPrevIDBuffer"] = 9;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIBuffer"] = 10;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gGIBuffer"] = 11;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gCausticsBuffer"] = 12;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIReservoirBuffer"] = 13;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gDISpatialReservoirBufferSrc"] = 14;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonRandomCounterMap"] = 15;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap0"] = 16;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap1"] = 17;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap2"] = 18;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap3"] = 19;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap4"] = 20;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap5"] = 21;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap6"] = 22;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gScreenSpaceMaterial"] = 23;
        mRegisterMapGlobalRootSigReservoirSpatialReuse["gReSTIRParam"] = 24;
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