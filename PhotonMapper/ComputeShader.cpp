#include "DxrPhotonMapper.h"
#include <fstream>
#include <wincodec.h>

void DxrPhotonMapper::GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, u32 pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    WCHAR* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

std::wstring DxrPhotonMapper::GetAssetFullPath(LPCWSTR assetName)
{
    return mAssetPath + assetName;
}

void DxrPhotonMapper::CreateComputeShaderStateObject(const LPCWSTR& compiledComputeShaderName, ComPtr<ID3D12PipelineState>& computePipelineState, ComPtr<ID3D12RootSignature> rootSig)
{
    u32 fileSize = 0;
    UINT8* shaderCodePtr;
    utility::ReadDataFromFile(GetAssetFullPath(compiledComputeShaderName).c_str(), &shaderCodePtr, &fileSize);

    D3D12_COMPUTE_PIPELINE_STATE_DESC copmputePSODesc = {};
    copmputePSODesc.pRootSignature = rootSig.Get();
    copmputePSODesc.CS = CD3DX12_SHADER_BYTECODE((void*)shaderCodePtr, fileSize);

    HRESULT hr = mDevice->GetDevice()->CreateComputePipelineState(&copmputePSODesc, IID_PPV_ARGS(&computePipelineState));

    if (FAILED(hr)) {
        throw std::runtime_error("CreateComputePipelineStateObject failed.");
    }
}

void DxrPhotonMapper::CreateComputeRootSignatureAndPSO()
{
    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsBitonicSortLDS = rsCreater.Create(mDevice, false, L"rsBitonic");
        CreateComputeShaderStateObject(ComputeShaders::BitonicSort, mBitonicSortLDSPSO, mRsBitonicSortLDS);
        mRegisterMapBitonicSortLDS["gBitonicParam"] = 0;
        mRegisterMapBitonicSortLDS["gOutput"] = 1;

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsTranspose = rsCreater.Create(mDevice, false, L"rsTranspose");
        CreateComputeShaderStateObject(ComputeShaders::Transpose, mTransposePSO, mRsTranspose);
        mRegisterMapTranspose["gBitonicParam"] = 0;
        mRegisterMapTranspose["gInput"] = 1;
        mRegisterMapTranspose["gOutput"] = 2;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsBitonicSortSimple = rsCreater.Create(mDevice, false, L"rsBitonic2");
        CreateComputeShaderStateObject(ComputeShaders::BitonicSort2, mBitonicSortSimplePSO, mRsBitonicSortSimple);
        mRegisterMapBitonicSortSimple["gBitonicParam"] = 0;
        mRegisterMapBitonicSortSimple["data"] = 1;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsBuildGrid = rsCreater.Create(mDevice, false, L"rsBuildGrid");
        CreateComputeShaderStateObject(ComputeShaders::BuildGrid, mBuildGridPSO, mRsBuildGrid);
        mRegisterMapBuildGrid["gGridParam"] = 0;
        mRegisterMapBuildGrid["gPhotonMapRead"] = 1;
        mRegisterMapBuildGrid["gPhotonGridBufferWrite"] = 2;

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsBuildGridIndices = rsCreater.Create(mDevice, false, L"rsBuildGridIndices");
        CreateComputeShaderStateObject(ComputeShaders::BuildGridIndices, mBuildGridIndicesPSO, mRsBuildGridIndices);
        mRegisterMapBuildGridIndices["gGridParam"] = 0;
        mRegisterMapBuildGridIndices["gPhotonGridBufferRead"] = 1;
        mRegisterMapBuildGridIndices["gPhotonGridBufferWrite"] = 2;

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsCopy = rsCreater.Create(mDevice, false, L"rsCopy");
        CreateComputeShaderStateObject(ComputeShaders::Copy, mCopyPSO, mRsCopy);
        mRegisterMapCopy["gGridParam"] = 0;
        mRegisterMapCopy["gPhotonGridBufferRead"] = 1;
        mRegisterMapCopy["gPhotonGridBufferWrite"] = 2;

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsClearGridIndices = rsCreater.Create(mDevice, false, L"rsClearGridIndices");
        CreateComputeShaderStateObject(ComputeShaders::ClearGridIndices, mClearGridIndicesPSO, mRsClearGridIndices);
        mRegisterMapClearGridIndices["gGridParam"] = 0;
        mRegisterMapClearGridIndices["gPhotonGridIdBufferWrite"] = 1;

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        mRsRearrangePhoton = rsCreater.Create(mDevice, false, L"rsRearrangePhoton");
        CreateComputeShaderStateObject(ComputeShaders::RearrangePhoton, mRearrangePhotonPSO, mRsRearrangePhoton);
        mRegisterMapRearrangePhoton["gGridParam"] = 0;
        mRegisterMapRearrangePhoton["gPhotonMapRead"] = 1;
        mRegisterMapRearrangePhoton["gPhotonMapWrite"] = 2;
        mRegisterMapRearrangePhoton["gPhotonGridBufferRead"] = 3;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsComputeVariance = rsCreater.Create(mDevice, false, L"rsComputeVariance");
        CreateComputeShaderStateObject(ComputeShaders::ComputeVariance, mComputeVariancePSO, mRsComputeVariance);
        mRegisterMapComputeVariance["depthBuffer"] = 0;
        mRegisterMapComputeVariance["normalBuffer"] = 1;
        mRegisterMapComputeVariance["luminanceMomentBuffer"] = 2;
        mRegisterMapComputeVariance["varianceBuffer"] = 3;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsA_TrousWaveletFilter = rsCreater.Create(mDevice, false, L"rsA_TrousWaveletFilter");
        CreateComputeShaderStateObject(ComputeShaders::A_Trous, mA_TrousWaveletFilterPSO, mRsA_TrousWaveletFilter);
        mRegisterMapA_TrousWaveletFilter["gWaveletParam"] = 0;
        mRegisterMapA_TrousWaveletFilter["colorBufferSrc"] = 1;
        mRegisterMapA_TrousWaveletFilter["depthBuffer"] = 2;
        mRegisterMapA_TrousWaveletFilter["normalBuffer"] = 3;
        mRegisterMapA_TrousWaveletFilter["varianceBufferSrc"] = 4;
        mRegisterMapA_TrousWaveletFilter["colorBufferDst"] = 5;
        mRegisterMapA_TrousWaveletFilter["varianceBufferDst"] = 6;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        mRsDebugView = rsCreater.Create(mDevice, false, L"rsDebugView");
        CreateComputeShaderStateObject(ComputeShaders::DebugView, mDebugViewPSO, mRsDebugView);
        mRegisterMapDebugView["diffuseAlbedoBuffer"] = 0;
        mRegisterMapDebugView["normalDepthBuffer"] = 1;
        mRegisterMapDebugView["idRoughnessBuffer"] = 2;
        mRegisterMapDebugView["finalColor"] = 3;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 5);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 6);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 7);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsTemporalReuse = rsCreater.Create(mDevice, false, L"rsTemporalReuse");
        CreateComputeShaderStateObject(ComputeShaders::TemporalReuse, mTemporalReusePSO, mRsTemporalReuse);
        mRegisterMapTemporalReuse["gSceneParam"] = 0;
        mRegisterMapTemporalReuse["DIReservoirBufferSrc"] = 1;
        mRegisterMapTemporalReuse["NormalDepthBuffer"] = 2;
        mRegisterMapTemporalReuse["PrevNormalDepthBuffer"] = 3;
        mRegisterMapTemporalReuse["PrevIDBuffer"] = 4;
        mRegisterMapTemporalReuse["IDRoughnessBuffer"] = 5;
        mRegisterMapTemporalReuse["PrevIDRoughnessBuffer"] = 6;
        mRegisterMapTemporalReuse["PositionBuffer"] = 7;
        mRegisterMapTemporalReuse["PrevPositionBuffer"] = 8;
        mRegisterMapTemporalReuse["DIReservoirBufferDst"] = 9;
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 5);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 6);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 7);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 8);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 9);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 10);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 11);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 5);
        mRsTemporalAccumulation = rsCreater.Create(mDevice, false, L"rsTemporalAccumulation");
        CreateComputeShaderStateObject(ComputeShaders::TemporalAccumulation, mTemporalAccumulationPSO, mRsTemporalAccumulation);
        mRegisterMapTemporalAccumulation["gSceneParam"] = 0;
        mRegisterMapTemporalAccumulation["HistoryDIBuffer"] = 1;
        mRegisterMapTemporalAccumulation["HistoryGIBuffer"] = 2;
        mRegisterMapTemporalAccumulation["HistoryCausticsBuffer"] = 3;
        mRegisterMapTemporalAccumulation["NormalDepthBuffer"] = 4;
        mRegisterMapTemporalAccumulation["PrevNormalDepthBuffer"] = 5;
        mRegisterMapTemporalAccumulation["PrevIDBuffer"] = 6;
        mRegisterMapTemporalAccumulation["LuminanceMomentBufferSrc"] = 7;
        mRegisterMapTemporalAccumulation["DIReservoirBufferSrc"] = 8;
        mRegisterMapTemporalAccumulation["IDRoughnessBuffer"] = 9;
        mRegisterMapTemporalAccumulation["PrevIDRoughnessBuffer"] = 10;
        mRegisterMapTemporalAccumulation["PositionBuffer"] = 11;
        mRegisterMapTemporalAccumulation["PrevPositionBuffer"] = 12;
        mRegisterMapTemporalAccumulation["CurrentDIBuffer"] = 13;
        mRegisterMapTemporalAccumulation["CurrentGIBuffer"] = 14;
        mRegisterMapTemporalAccumulation["CurrentCausticsBuffer"] = 15;
        mRegisterMapTemporalAccumulation["DIGIBuffer"] = 16;
        mRegisterMapTemporalAccumulation["AccumulationCountBuffer"] = 17;
        mRegisterMapTemporalAccumulation["LuminanceMomentBufferDst"] = 18;
    }
}