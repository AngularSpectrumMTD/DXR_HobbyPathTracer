#include "DxrPhotonMapper.h"


void DxrPhotonMapper::BitonicSortLDS()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    const u32 NUM_ELEMENTS = mPhotonMapSize1D * mPhotonMapSize1D * 2;//[2] is for Reflection at Refraction
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
        mCommandList->SetComputeRootConstantBufferView(0, bitonicCB0->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mBitonicSortLDSPSO.Get());
        mCommandList->Dispatch((s32)(NUM_ELEMENTS / BITONIC_BLOCK_SIZE), 1, 1);

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
        mCommandList->SetComputeRootConstantBufferView(0, bitonicCB1->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(2, mPhotonGridTmpDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mTransposePSO.Get());
        mCommandList->Dispatch((s32)(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE), (s32)(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE), 1);

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());
        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        mCommandList->SetComputeRootSignature(mRsBitonicSortLDS.Get());
        mCommandList->SetComputeRootConstantBufferView(0, bitonicCB1->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridTmpDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mBitonicSortLDSPSO.Get());
        mCommandList->Dispatch((s32)(NUM_ELEMENTS / BITONIC_BLOCK_SIZE), 1, 1);

        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        BitonicSortCB bitonicCBStruct2;
        bitonicCBStruct2.set(BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH);

        auto bitonicCB2 = mBitonicLDSCB2Tbl[count].Get();
        mDevice->ImmediateBufferUpdateHostVisible(bitonicCB2, &bitonicCBStruct2, sizeof(bitonicCBStruct2));

        mCommandList->SetComputeRootSignature(mRsTranspose.Get());
        mCommandList->SetComputeRootConstantBufferView(0, bitonicCB2->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridTmpDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(2, mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mTransposePSO.Get());
        mCommandList->Dispatch((s32)(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE), (s32)(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE), 1);

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());
        mCommandList->ResourceBarrier(u32(photonGridTmpUavBarrier.size()), photonGridTmpUavBarrier.data());

        mCommandList->SetComputeRootSignature(mRsBitonicSortLDS.Get());
        mCommandList->SetComputeRootConstantBufferView(0, bitonicCB2->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mBitonicSortLDSPSO.Get());
        mCommandList->Dispatch((s32)(NUM_ELEMENTS / BITONIC_BLOCK_SIZE), 1, 1);

        mCommandList->ResourceBarrier(u32(photonGridUavBarrier.size()), photonGridUavBarrier.data());

        count++;
    }
}

void DxrPhotonMapper::BitonicSortSimple()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    //Limitation Elements: 1 << 8(=256) -- 1 << 27(134,217,728)
    s32 n = mPhotonMapSize1D * mPhotonMapSize1D * 2;;//[2] is for Reflection at Refraction

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
            mCommandList->SetComputeRootConstantBufferView(0, bitonicSimpleCB->GetGPUVirtualAddress());
            mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridDescriptorUAV.hGpu);
            mCommandList->SetPipelineState(mBitonicSortSimplePSO.Get());
            mCommandList->Dispatch(n / 2 / BITONIC2_THREAD_NUM, 1, 1);

            mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());//MUST NEED

            inc /= 2;//...16 8 4 2 1

            count++;
        }
    }
}

void DxrPhotonMapper::Grid3DSort()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();
    const u32 photonNum = mPhotonMapSize1D * mPhotonMapSize1D * 2;
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
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mPhotonMapDescriptorUAV.hGpu);//Src PhotonMap
    mCommandList->SetComputeRootDescriptorTable(2, mPhotonGridDescriptorUAV.hGpu);//Record Photon Hash & Serial Index
    mCommandList->SetPipelineState(mBuildGridPSO.Get());
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);

    //Photon Hash / Serial Index Pair Sorting By Photon Hash
    //BitonicSortLDS();
    BitonicSortSimple();

    mCommandList->ResourceBarrier(u32(uavBarriersGridId.size()), uavBarriersGridId.data());

    mCommandList->SetComputeRootSignature(mRsClearGridIndices.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetPipelineState(mClearGridIndicesPSO.Get());
    mCommandList->Dispatch((s32)(gridNum / GRID_SORT_THREAD_NUM), 1, 1);

    mCommandList->ResourceBarrier(u32(uavBarriersGridId.size()), uavBarriersGridId.data());
    mCommandList->ResourceBarrier(u32(uavBarriersGrid.size()), uavBarriersGrid.data());

    mCommandList->SetComputeRootSignature(mRsBuildGridIndices.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mPhotonGridDescriptorUAV.hGpu);//Sorted ID
    mCommandList->SetComputeRootDescriptorTable(2, mPhotonGridIdDescriptorUAV.hGpu);//Record Serial Range(Photon Hash)
    mCommandList->SetPipelineState(mBuildGridIndicesPSO.Get());
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);

    mCommandList->ResourceBarrier(u32(uavBarriersGridId.size()), uavBarriersGridId.data());
    mCommandList->ResourceBarrier(u32(uavBarriersGrid.size()), uavBarriersGrid.data());
    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMap.size()), uavBarriersPhotonMap.data());
    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMapSorted.size()), uavBarriersPhotonMapSorted.data());

    mCommandList->SetComputeRootSignature(mRsRearrangePhoton.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mPhotonMapDescriptorUAV.hGpu);//Src Photon Map
    mCommandList->SetComputeRootDescriptorTable(2, mPhotonMapSortedDescriptorUAV.hGpu);//Sorted Photon Map According to Serial Range
    mCommandList->SetComputeRootDescriptorTable(3, mPhotonGridDescriptorUAV.hGpu);//Sorted ID
    mCommandList->SetPipelineState(mRearrangePhotonPSO.Get());
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);

    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMap.size()), uavBarriersPhotonMap.data());
    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMapSorted.size()), uavBarriersPhotonMapSorted.data());

    mCommandList->SetComputeRootSignature(mRsCopy.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mPhotonMapSortedDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(2, mPhotonMapDescriptorUAV.hGpu);
    mCommandList->SetPipelineState(mCopyPSO.Get());
    mCommandList->Dispatch((s32)(photonNum / GRID_SORT_THREAD_NUM), 1, 1);

    mCommandList->ResourceBarrier(u32(uavBarriersPhotonMap.size()), uavBarriersPhotonMap.data());
}