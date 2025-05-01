#include "DXRPathTracer.h"
#include <fstream>
#include <wincodec.h>

void DXRPathTracer::GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, u32 pathSize)
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

std::wstring DXRPathTracer::GetAssetFullPath(LPCWSTR assetName)
{
    return mAssetPath + assetName;
}

void DXRPathTracer::CreateComputeShaderStateObject(const LPCWSTR& compiledComputeShaderName, ComPtr<ID3D12PipelineState>& computePipelineState, ComPtr<ID3D12RootSignature> rootSig)
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

void DXRPathTracer::CreateComputeRootSignatureAndPSO()
{
    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsBitonicSortLDS = rsCreater.Create(mDevice, false, L"rsBitonic");
        CreateComputeShaderStateObject(ComputeShaders::BitonicSort, mBitonicSortLDSPSO, mRsBitonicSortLDS);
        mRegisterMapBitonicSortLDS.clear();
        mRegisterMapBitonicSortLDS["gBitonicParam"] = mRegisterMapBitonicSortLDS.size();
        mRegisterMapBitonicSortLDS["gOutput"] = mRegisterMapBitonicSortLDS.size();

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsTranspose = rsCreater.Create(mDevice, false, L"rsTranspose");
        CreateComputeShaderStateObject(ComputeShaders::Transpose, mTransposePSO, mRsTranspose);
        mRegisterMapTranspose.clear();
        mRegisterMapTranspose["gBitonicParam"] = mRegisterMapTranspose.size();
        mRegisterMapTranspose["gInput"] = mRegisterMapTranspose.size();
        mRegisterMapTranspose["gOutput"] = mRegisterMapTranspose.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsBitonicSortSimple = rsCreater.Create(mDevice, false, L"rsBitonic2");
        CreateComputeShaderStateObject(ComputeShaders::BitonicSort2, mBitonicSortSimplePSO, mRsBitonicSortSimple);
        mRegisterMapBitonicSortSimple.clear();
        mRegisterMapBitonicSortSimple["gBitonicParam"] = mRegisterMapBitonicSortSimple.size();
        mRegisterMapBitonicSortSimple["data"] = mRegisterMapBitonicSortSimple.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsBuildGrid = rsCreater.Create(mDevice, false, L"rsBuildGrid");
        CreateComputeShaderStateObject(ComputeShaders::BuildGrid, mBuildGridPSO, mRsBuildGrid);
        mRegisterMapBuildGrid.clear();
        mRegisterMapBuildGrid["gGridParam"] = mRegisterMapBuildGrid.size();
        mRegisterMapBuildGrid["gPhotonMapRead"] = mRegisterMapBuildGrid.size();
        mRegisterMapBuildGrid["gPhotonGridBufferWrite"] = mRegisterMapBuildGrid.size();

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsBuildGridIndices = rsCreater.Create(mDevice, false, L"rsBuildGridIndices");
        CreateComputeShaderStateObject(ComputeShaders::BuildGridIndices, mBuildGridIndicesPSO, mRsBuildGridIndices);
        mRegisterMapBuildGridIndices.clear();
        mRegisterMapBuildGridIndices["gGridParam"] = mRegisterMapBuildGridIndices.size();
        mRegisterMapBuildGridIndices["gPhotonGridBufferRead"] = mRegisterMapBuildGridIndices.size();
        mRegisterMapBuildGridIndices["gPhotonGridBufferWrite"] = mRegisterMapBuildGridIndices.size();

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsCopy = rsCreater.Create(mDevice, false, L"rsCopy");
        CreateComputeShaderStateObject(ComputeShaders::Copy, mCopyPSO, mRsCopy);
        mRegisterMapCopy.clear();
        mRegisterMapCopy["gGridParam"] = mRegisterMapCopy.size();
        mRegisterMapCopy["gPhotonGridBufferRead"] = mRegisterMapCopy.size();
        mRegisterMapCopy["gPhotonGridBufferWrite"] = mRegisterMapCopy.size();

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsClearGridIndices = rsCreater.Create(mDevice, false, L"rsClearGridIndices");
        CreateComputeShaderStateObject(ComputeShaders::ClearGridIndices, mClearGridIndicesPSO, mRsClearGridIndices);
        mRegisterMapClearGridIndices.clear();
        mRegisterMapClearGridIndices["gGridParam"] = mRegisterMapClearGridIndices.size();
        mRegisterMapClearGridIndices["gPhotonGridIdBufferWrite"] = mRegisterMapClearGridIndices.size();

        rsCreater.Clear();
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        mRsRearrangePhoton = rsCreater.Create(mDevice, false, L"rsRearrangePhoton");
        CreateComputeShaderStateObject(ComputeShaders::RearrangePhoton, mRearrangePhotonPSO, mRsRearrangePhoton);
        mRegisterMapRearrangePhoton.clear();
        mRegisterMapRearrangePhoton["gGridParam"] = mRegisterMapRearrangePhoton.size();
        mRegisterMapRearrangePhoton["gPhotonMapRead"] = mRegisterMapRearrangePhoton.size();
        mRegisterMapRearrangePhoton["gPhotonMapWrite"] = mRegisterMapRearrangePhoton.size();
        mRegisterMapRearrangePhoton["gPhotonGridBufferRead"] = mRegisterMapRearrangePhoton.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        mRsComputeVariance = rsCreater.Create(mDevice, false, L"rsComputeVariance");
        CreateComputeShaderStateObject(ComputeShaders::ComputeVariance, mComputeVariancePSO, mRsComputeVariance);
        mRegisterMapComputeVariance.clear();
        mRegisterMapComputeVariance["depthBuffer"] = mRegisterMapComputeVariance.size();
        mRegisterMapComputeVariance["normalBuffer"] = mRegisterMapComputeVariance.size();
        mRegisterMapComputeVariance["luminanceMomentBuffer"] = mRegisterMapComputeVariance.size();
        mRegisterMapComputeVariance["varianceBuffer"] = mRegisterMapComputeVariance.size();
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
        mRegisterMapA_TrousWaveletFilter.clear();
        mRegisterMapA_TrousWaveletFilter["gWaveletParam"] = mRegisterMapA_TrousWaveletFilter.size();
        mRegisterMapA_TrousWaveletFilter["colorBufferSrc"] = mRegisterMapA_TrousWaveletFilter.size();
        mRegisterMapA_TrousWaveletFilter["depthBuffer"] = mRegisterMapA_TrousWaveletFilter.size();
        mRegisterMapA_TrousWaveletFilter["normalBuffer"] = mRegisterMapA_TrousWaveletFilter.size();
        mRegisterMapA_TrousWaveletFilter["varianceBufferSrc"] = mRegisterMapA_TrousWaveletFilter.size();
        mRegisterMapA_TrousWaveletFilter["colorBufferDst"] = mRegisterMapA_TrousWaveletFilter.size();
        mRegisterMapA_TrousWaveletFilter["varianceBufferDst"] = mRegisterMapA_TrousWaveletFilter.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        mRsDebugView = rsCreater.Create(mDevice, false, L"rsDebugView");
        CreateComputeShaderStateObject(ComputeShaders::DebugView, mDebugViewPSO, mRsDebugView);
        mRegisterMapDebugView.clear();
        mRegisterMapDebugView["screenSpaceMaterial"] = mRegisterMapDebugView.size();
        mRegisterMapDebugView["normalDepthBuffer"] = mRegisterMapDebugView.size();
        mRegisterMapDebugView["finalColor"] = mRegisterMapDebugView.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsAccumulatePhotonEmissionGuideMap = rsCreater.Create(mDevice, false, L"rsAccumulatePhotonEmissionGuideMap");
        CreateComputeShaderStateObject(ComputeShaders::AccumulatePhotonEmissionGuideMap, mAccumulatePhotonEmissionGuideMapPSO, mRsAccumulatePhotonEmissionGuideMap);
        mRegisterMapAccumulatePhotonEmissionGuideMap.clear();
        mRegisterMapAccumulatePhotonEmissionGuideMap["gPhotonEmissionGuideMapPrev"] = mRegisterMapAccumulatePhotonEmissionGuideMap.size();
        mRegisterMapAccumulatePhotonEmissionGuideMap["gPhotonEmissionGuideMapCurr"] = mRegisterMapAccumulatePhotonEmissionGuideMap.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsCopyPhotonEmissionGuideMap = rsCreater.Create(mDevice, false, L"rsCopyPhotonEmissionGuideMap");
        CreateComputeShaderStateObject(ComputeShaders::CopyPhotonEmissionGuideMap, mCopyPhotonEmissionGuideMapPSO, mRsCopyPhotonEmissionGuideMap);
        mRegisterMapCopyPhotonEmissionGuideMap.clear();
        mRegisterMapCopyPhotonEmissionGuideMap["gPhotonEmissionGuideMapSrc"] = mRegisterMapCopyPhotonEmissionGuideMap.size();
        mRegisterMapCopyPhotonEmissionGuideMap["gPhotonEmissionGuideMapDst"] = mRegisterMapCopyPhotonEmissionGuideMap.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsGeneratePhotonEmissionGuideMap = rsCreater.Create(mDevice, false, L"rsGeneratePhotonEmissionGuideMap");
        CreateComputeShaderStateObject(ComputeShaders::GeneratePhotonEmissionGuideMap, mGeneratePhotonEmissionGuideMapPSO, mRsGeneratePhotonEmissionGuideMap);
        mRegisterMapGeneratePhotonEmissionGuideMap.clear();
        mRegisterMapGeneratePhotonEmissionGuideMap["gPhotonRandomCounterMap"] = mRegisterMapGeneratePhotonEmissionGuideMap.size();
        mRegisterMapGeneratePhotonEmissionGuideMap["gPhotonEmissionGuideMap"] = mRegisterMapGeneratePhotonEmissionGuideMap.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 5);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 6);
        mRsClearPhotonEmissionGuideMap = rsCreater.Create(mDevice, false, L"rsClearPhotonEmissionGuideMipMap");
        CreateComputeShaderStateObject(ComputeShaders::ClearPhotonEmissionGuideMap, mClearPhotonEmissionGuideMapPSO, mRsClearPhotonEmissionGuideMap);
        mRegisterMapClearPhotonEmissionGuideMap.clear();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap0"] = mRegisterMapClearPhotonEmissionGuideMap.size();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap1"] = mRegisterMapClearPhotonEmissionGuideMap.size();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap2"] = mRegisterMapClearPhotonEmissionGuideMap.size();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap3"] = mRegisterMapClearPhotonEmissionGuideMap.size();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap4"] = mRegisterMapClearPhotonEmissionGuideMap.size();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap5"] = mRegisterMapClearPhotonEmissionGuideMap.size();
        mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap6"] = mRegisterMapClearPhotonEmissionGuideMap.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        mRsGeneratePhotonEmissionGuideMipMap = rsCreater.Create(mDevice, false, L"rsGeneratePhotonEmissionGuideMipMap");
        CreateComputeShaderStateObject(ComputeShaders::GeneratePhotonEmissionGuideMipMap, mGeneratePhotonEmissionGuideMipMapPSO, mRsGeneratePhotonEmissionGuideMipMap);
        mRegisterMapGeneratePhotonEmissionGuideMipMap.clear();
        mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap0"] = mRegisterMapGeneratePhotonEmissionGuideMipMap.size();
        mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap1"] = mRegisterMapGeneratePhotonEmissionGuideMipMap.size();
        mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap2"] = mRegisterMapGeneratePhotonEmissionGuideMipMap.size();
        mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap3"] = mRegisterMapGeneratePhotonEmissionGuideMipMap.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        mRsGeneratePhotonEmissionGuideMipMap2x2 = rsCreater.Create(mDevice, false, L"rsGeneratePhotonEmissionGuideMipMap2x2");
        CreateComputeShaderStateObject(ComputeShaders::GeneratePhotonEmissionGuideMipMap2x2, mGeneratePhotonEmissionGuideMipMap2x2PSO, mRsGeneratePhotonEmissionGuideMipMap2x2);
        mRegisterMapGeneratePhotonEmissionGuideMipMap2x2.clear();
        mRegisterMapGeneratePhotonEmissionGuideMipMap2x2["gPhotonEmissionGuideMap0"] = mRegisterMapGeneratePhotonEmissionGuideMipMap2x2.size();
        mRegisterMapGeneratePhotonEmissionGuideMipMap2x2["gPhotonEmissionGuideMap1"] = mRegisterMapGeneratePhotonEmissionGuideMipMap2x2.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        mRsGeneratePhotonEmissionGuideMipMap4x4 = rsCreater.Create(mDevice, false, L"rsGeneratePhotonEmissionGuideMipMap4x4");
        CreateComputeShaderStateObject(ComputeShaders::GeneratePhotonEmissionGuideMipMap4x4, mGeneratePhotonEmissionGuideMipMap4x4PSO, mRsGeneratePhotonEmissionGuideMipMap4x4);
        mRegisterMapGeneratePhotonEmissionGuideMipMap4x4.clear();
        mRegisterMapGeneratePhotonEmissionGuideMipMap4x4["gPhotonEmissionGuideMap0"] = mRegisterMapGeneratePhotonEmissionGuideMipMap4x4.size();
        mRegisterMapGeneratePhotonEmissionGuideMipMap4x4["gPhotonEmissionGuideMap1"] = mRegisterMapGeneratePhotonEmissionGuideMipMap4x4.size();
        mRegisterMapGeneratePhotonEmissionGuideMipMap4x4["gPhotonEmissionGuideMap2"] = mRegisterMapGeneratePhotonEmissionGuideMipMap4x4.size();
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
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 4);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 5);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 6);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 7);
        mRsTemporalAccumulation = rsCreater.Create(mDevice, false, L"rsTemporalAccumulation");
        CreateComputeShaderStateObject(ComputeShaders::TemporalAccumulation, mTemporalAccumulationPSO, mRsTemporalAccumulation);
        mRegisterMapTemporalAccumulation.clear();
        mRegisterMapTemporalAccumulation["gSceneParam"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["HistoryDIBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["HistoryGIBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["HistoryCausticsBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["NormalDepthBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["PrevNormalDepthBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["PrevIDBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["LuminanceMomentBufferSrc"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["DIReservoirBufferSrc"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["PositionBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["PrevPositionBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["GIReservoirBufferSrc"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["CurrentDIBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["CurrentGIBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["CurrentCausticsBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["DIGIBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["AccumulationCountBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["LuminanceMomentBufferDst"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["PrevAccumulationCountBuffer"] = mRegisterMapTemporalAccumulation.size();
        mRegisterMapTemporalAccumulation["ScreenSpaceMaterial"] = mRegisterMapTemporalAccumulation.size();
    }

    {
        utility::RootSignatureCreater rsCreater;
        rsCreater.Push(utility::RootSignatureCreater::RootType::CBV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::SRV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 0);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 1);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 2);
        rsCreater.Push(utility::RootSignatureCreater::RangeType::UAV, 3);
        mRsFinalizePathtracedResult = rsCreater.Create(mDevice, false, L"rsFinalizePathtracedResult");
        CreateComputeShaderStateObject(ComputeShaders::FinalizePathtracedResult, mFinalizePathtracedResultPSO, mRsFinalizePathtracedResult);
        mRegisterMapFinalizePathtracedResult.clear();
        mRegisterMapFinalizePathtracedResult["gSceneParam"] = mRegisterMapFinalizePathtracedResult.size();
        mRegisterMapFinalizePathtracedResult["NormalDepthBuffer"] = mRegisterMapFinalizePathtracedResult.size();
        mRegisterMapFinalizePathtracedResult["ScreenSpaceMaterial"] = mRegisterMapFinalizePathtracedResult.size();
        mRegisterMapFinalizePathtracedResult["AccumulationCountBuffer"] = mRegisterMapFinalizePathtracedResult.size();
        mRegisterMapFinalizePathtracedResult["NativePathtracedResult"] = mRegisterMapFinalizePathtracedResult.size();
        mRegisterMapFinalizePathtracedResult["FinalizedPathtracedResult"] = mRegisterMapFinalizePathtracedResult.size();
    }
}