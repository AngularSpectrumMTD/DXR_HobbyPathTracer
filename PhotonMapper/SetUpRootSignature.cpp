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
        rsCreater.PushStaticSampler(0);
        mGrs = rsCreater.Create(mDevice, false, L"RootSignatureGlobal");
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
        rsCreater.PushStaticSampler(0);
        mGrsPhoton = rsCreater.Create(mDevice, false, L"RootSignatureGlobalPhoton");
    }
}

void DxrPhotonMapper::CreateRootSignatureLocal()
{
    CreateSphereLocalRootSignature();
    CreateFloorLocalRootSignature();
    CreateGlassLocalRootSignature();
}

void DxrPhotonMapper::CreateSphereLocalRootSignature()
{
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        mRsSphereRR = rsCreater.Create(mDevice, true, L"lrsSphere(Reflect/Refract)");
        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        mRsSphereDefault = rsCreater.Create(mDevice, true, L"lrsSphere(Default)");
    }
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        mRsSphereRRPhoton = rsCreater.Create(mDevice, true, L"lrsSphere(Reflect/Refract)Photon");
        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        mRsSphereDefaultPhoton = rsCreater.Create(mDevice, true, L"lrsSphere(Default)Photon");
    }
}

void DxrPhotonMapper::CreateGlassLocalRootSignature()
{
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        mRsGlass = rsCreater.Create(mDevice, true, L"lrsGlass(Reflect/Refract)");
    }
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0, regSpace);
        mRsGlassPhoton = rsCreater.Create(mDevice, true, L"lrsGlass(Reflect/Refract)Photon");
    }
}

void DxrPhotonMapper::CreateFloorLocalRootSignature()
{
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2, regSpace);
        mRsFloor = rsCreater.Create(mDevice, true, L"lrsFloor");
    }
    {
        const u32 regSpace = 1;
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1, regSpace);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2, regSpace);
        mRsFloorPhoton = rsCreater.Create(mDevice, true, L"lrsFloorPhoton");
    }
}