#include "DXRPathTracer.h"


void DXRPathTracer::BitonicSortLDS()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    const u32 NUM_ELEMENTS = mPhotonMapSize1D * mPhotonMapSize1D;
    const u32 MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
    const u32 MATRIX_HEIGHT = (u32)NUM_ELEMENTS / MATRIX_WIDTH;

    std::vector<CD3DX12_RESOURCE_BARRIER> photonGridUavBarrier;
    photonGridUavBarrier.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonGrid.Get()));
    std::vector<CD3DX12_RESOURCE_BARRIER> photonGridTmpUavBarrier;
    photonGridTmpUavBarrier.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonGridTmp.Get()));

    mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());
    u32 count = 0;
    for (u32 level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1)//SEQUENCE : execute unit <= block size
    {
        BitonicSortCB bitonicCBStruct0;
        bitonicCBStruct0.set(level, level, MATRIX_HEIGHT, MATRIX_WIDTH);

        auto bitonicCB0 = mBitonicLDSCB0Tbl[count].Get();
        mDevice->ImmediateBufferUpdateHostVisible(bitonicCB0, &bitonicCBStruct0, sizeof(bitonicCBStruct0));

        mCommandList->SetComputeRootSignature(mRsBitonicSortLDS.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapBitonicSortLDS["gBitonicParam"], bitonicCB0->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapBitonicSortLDS["gOutput"], mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mBitonicSortLDSPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "BitonicSort");
        mCommandList->Dispatch((s32)(NUM_ELEMENTS / BITONIC_BLOCK_SIZE), 1, 1);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());

        count++;
    }

    count = 0;
    for (u32 level = (BITONIC_BLOCK_SIZE << 1); level <= NUM_ELEMENTS; level <<= 1)///SEQUENCE : execute unit > block size
    {
        BitonicSortCB bitonicCBStruct1;
        bitonicCBStruct1.set(level / BITONIC_BLOCK_SIZE, (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT);

        auto bitonicCB1 = mBitonicLDSCB1Tbl[count].Get();
        mDevice->ImmediateBufferUpdateHostVisible(bitonicCB1, &bitonicCBStruct1, sizeof(bitonicCBStruct1));

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());
        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        mCommandList->SetComputeRootSignature(mRsTranspose.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapTranspose["gBitonicParam"], bitonicCB1->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTranspose["gInput"], mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTranspose["gOutput"], mPhotonGridTmpDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mTransposePSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "BitonicSort_Transpose");
        mCommandList->Dispatch((s32)(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE), (s32)(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE), 1);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());
        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        mCommandList->SetComputeRootSignature(mRsBitonicSortLDS.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapBitonicSortLDS["gBitonicParam"], bitonicCB1->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapBitonicSortLDS["gOutput"], mPhotonGridTmpDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mBitonicSortLDSPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "BitonicSort");
        mCommandList->Dispatch((s32)(NUM_ELEMENTS / BITONIC_BLOCK_SIZE), 1, 1);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        BitonicSortCB bitonicCBStruct2;
        bitonicCBStruct2.set(BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH);

        auto bitonicCB2 = mBitonicLDSCB2Tbl[count].Get();
        mDevice->ImmediateBufferUpdateHostVisible(bitonicCB2, &bitonicCBStruct2, sizeof(bitonicCBStruct2));

        mCommandList->SetComputeRootSignature(mRsTranspose.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapTranspose["gBitonicParam"], bitonicCB2->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTranspose["gInput"], mPhotonGridTmpDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTranspose["gOutput"], mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mTransposePSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "BitonicSort_Transpose");
        mCommandList->Dispatch((s32)(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE), (s32)(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE), 1);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());
        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        mCommandList->SetComputeRootSignature(mRsBitonicSortLDS.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapBitonicSortLDS["gBitonicParam"], bitonicCB2->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapBitonicSortLDS["gOutput"], mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mBitonicSortLDSPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "BitonicSort");
        mCommandList->Dispatch((s32)(NUM_ELEMENTS / BITONIC_BLOCK_SIZE), 1, 1);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());

        count++;
    }
}

void DXRPathTracer::BitonicSortSimple()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    //Limitation Elements: 1 << 8(=256) -- 1 << 27(134,217,728)
    s32 n = mPhotonMapSize1D * mPhotonMapSize1D;

    s32 nlog = (s32)(log2(n));
    s32 inc;

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonGrid.Get()));

    mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());

    u32 count = 0;
    for (s32 i = 0; i < nlog; i++)
    {
        inc = 1 << i;//0 1 2 4 8 16...
        for (s32 j = 0; j < i + 1; j++)
        {
            BitonicSortCB2 cb;
            cb.inc = inc;
            cb.dir = 2 << i;

            auto bitonicSimpleCB = mBitonicSimpleCBTbl[count].Get();
            mDevice->ImmediateBufferUpdateHostVisible(bitonicSimpleCB, &cb, sizeof(cb));

            mCommandList->SetComputeRootSignature(mRsBitonicSortSimple.Get());
            mCommandList->SetComputeRootConstantBufferView(mRegisterMapBitonicSortSimple["gBitonicParam"], bitonicSimpleCB->GetGPUVirtualAddress());
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapBitonicSortSimple["data"], mPhotonGridDescriptorUAV.hGpu);
            mCommandList->SetPipelineState(mBitonicSortSimplePSO.Get());
            PIXBeginEvent(mCommandList.Get(), 0, "BitonicSort");
            mCommandList->Dispatch(n / 2 / BITONIC2_THREAD_NUM, 1, 1);
            PIXEndEvent(mCommandList.Get());

            mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());//MUST NEED

            inc /= 2;//...16 8 4 2 1

            count++;
        }
    }
}

void DXRPathTracer::Grid3DSort()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();
    const u32 photonNum = mPhotonMapSize1D * mPhotonMapSize1D;
    const u32 gridNum = GRID_DIMENSION * GRID_DIMENSION * GRID_DIMENSION;

    GridCB gridSortCBStruct;
    gridSortCBStruct.numPhotons = photonNum;
    gridSortCBStruct.gridDimensions = XMFLOAT3(GRID_DIMENSION, GRID_DIMENSION, GRID_DIMENSION);//x * y * z = numGrids
    gridSortCBStruct.gridH = GRID_DIMENSION;
    gridSortCBStruct.photonExistRange = 2 * PLANE_SIZE;

    auto gridCB = mGridSortCB.Get();
    mDevice->ImmediateBufferUpdateHostVisible(gridCB, &gridSortCBStruct, sizeof(gridSortCBStruct));

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriersGridId;
    uavBarriersGridId.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonGridId.Get()));
    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriersGrid;
    uavBarriersGrid.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonGrid.Get()));
    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriersPhotonMap;
    uavBarriersPhotonMap.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonMap.Get()));
    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriersPhotonMapSorted;
    uavBarriersPhotonMapSorted.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonMapSorted.Get()));

    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMap.size()), uavBarriersPhotonMap.data());
    mCommandList->ResourceBarrier(u32(uavBarriersGrid.size()), uavBarriersGrid.data());

    mCommandList->SetComputeRootSignature(mRsBuildGrid.Get());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapBuildGrid["gGridParam"], gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapBuildGrid["gPhotonMapRead"], mPhotonMapDescriptorUAV.hGpu);//Src PhotonMap
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapBuildGrid["gPhotonGridBufferWrite"], mPhotonGridDescriptorUAV.hGpu);//Record Photon Hash & Serial Index
    mCommandList->SetPipelineState(mBuildGridPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "BuildGrid");
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);
    PIXEndEvent(mCommandList.Get());

    //Photon Hash / Serial Index Pair Sorting By Photon Hash
    BitonicSortLDS();
    //BitonicSortSimple();

    mCommandList->ResourceBarrier(u32(uavBarriersGridId.size()), uavBarriersGridId.data());

    mCommandList->SetComputeRootSignature(mRsClearGridIndices.Get());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapClearGridIndices["gGridParam"], gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearGridIndices["gPhotonGridIdBufferWrite"], mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetPipelineState(mClearGridIndicesPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "ClearGrid");
    mCommandList->Dispatch((s32)(gridNum / GRID_SORT_THREAD_NUM), 1, 1);
    PIXEndEvent(mCommandList.Get());

    mCommandList->ResourceBarrier(u32(uavBarriersGridId.size()), uavBarriersGridId.data());
    mCommandList->ResourceBarrier(u32(uavBarriersGrid.size()), uavBarriersGrid.data());

    mCommandList->SetComputeRootSignature(mRsBuildGridIndices.Get());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapBuildGridIndices["gGridParam"], gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapBuildGridIndices["gPhotonGridBufferRead"], mPhotonGridDescriptorUAV.hGpu);//Sorted ID
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapBuildGridIndices["gPhotonGridBufferWrite"], mPhotonGridIdDescriptorUAV.hGpu);//Record Serial Range(Photon Hash)
    mCommandList->SetPipelineState(mBuildGridIndicesPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "BuildGridIndices");
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);
    PIXEndEvent(mCommandList.Get());

    mCommandList->ResourceBarrier(u32(uavBarriersGridId.size()), uavBarriersGridId.data());
    mCommandList->ResourceBarrier(u32(uavBarriersGrid.size()), uavBarriersGrid.data());
    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMap.size()), uavBarriersPhotonMap.data());
    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMapSorted.size()), uavBarriersPhotonMapSorted.data());

    mCommandList->SetComputeRootSignature(mRsRearrangePhoton.Get());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapRearrangePhoton["gGridParam"], gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapRearrangePhoton["gPhotonMapRead"], mPhotonMapDescriptorUAV.hGpu);//Src Photon Map
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapRearrangePhoton["gPhotonMapWrite"], mPhotonMapSortedDescriptorUAV.hGpu);//Sorted Photon Map According to Serial Range
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapRearrangePhoton["gPhotonGridBufferRead"], mPhotonGridDescriptorUAV.hGpu);//Sorted ID
    mCommandList->SetPipelineState(mRearrangePhotonPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "RearrangePhoton");
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);
    PIXEndEvent(mCommandList.Get());

    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMap.size()), uavBarriersPhotonMap.data());
    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMapSorted.size()), uavBarriersPhotonMapSorted.data());
}