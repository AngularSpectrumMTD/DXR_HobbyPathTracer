#include "DxrPhotonMapper.h"

using namespace DirectX;

void DxrPhotonMapper::CreateResultBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    mDXRMainOutput = mDevice->CreateTexture2D(
        width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_HEAP_TYPE_DEFAULT,
        L"DXRMainOutput"
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mMainOutputDescriptorSRV = mDevice->CreateShaderResourceView(mDXRMainOutput.Get(), &srvDesc);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    mMainOutputDescriptorUAV = mDevice->CreateUnorderedAccessView(mDXRMainOutput.Get(), &uavDesc);

    mDXROutput = mDevice->CreateTexture2D(
        width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_HEAP_TYPE_DEFAULT,
        L"DXROutputTbl[%d]"
    );

    mOutputDescriptorUAV = mDevice->CreateUnorderedAccessView(mDXROutput.Get(), &uavDesc);
}

void DxrPhotonMapper::CreateDenoisedColorBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    mDenoisedColorBuffer = mDevice->CreateTexture2D(
        width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_HEAP_TYPE_DEFAULT,
        L"DenoisedColorBuffer"
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mDenoisedColorBufferDescriptorSRV = mDevice->CreateShaderResourceView(mDenoisedColorBuffer.Get(), &srvDesc);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    mDenoisedColorBufferDescriptorUAV = mDevice->CreateUnorderedAccessView(mDenoisedColorBuffer.Get(), &uavDesc);
}

void DxrPhotonMapper::CreatePositionBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    mPositionBuffer = mDevice->CreateTexture2D(
        width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_HEAP_TYPE_DEFAULT,
        L"PositionBuffer"
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mPositionBufferDescriptorSRV = mDevice->CreateShaderResourceView(mPositionBuffer.Get(), &srvDesc);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    mPositionBufferDescriptorUAV = mDevice->CreateUnorderedAccessView(mPositionBuffer.Get(), &uavDesc);
}

void DxrPhotonMapper::CreateNormalBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    mNormalBuffer = mDevice->CreateTexture2D(
        width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_HEAP_TYPE_DEFAULT,
        L"NormalBuffer"
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mNormalBufferDescriptorSRV = mDevice->CreateShaderResourceView(mNormalBuffer.Get(), &srvDesc);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    mNormalBufferDescriptorUAV = mDevice->CreateUnorderedAccessView(mNormalBuffer.Get(), &uavDesc);
}

void DxrPhotonMapper::CreateDepthBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    const u32 size = 2;

    mDepthBufferTbl.resize(size);
    mDepthBufferDescriptorSRVTbl.resize(size);
    mDepthBufferDescriptorUAVTbl.resize(size);

    for (u32 i = 0; i < size; i++)
    {
        wchar_t name[30];
        swprintf(name, 30, L"DepthBufferTbl[%d]", i);
        mDepthBufferTbl.at(i) = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R32_FLOAT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            name
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mDepthBufferDescriptorSRVTbl.at(i) = mDevice->CreateShaderResourceView(mDepthBufferTbl.at(i).Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mDepthBufferDescriptorUAVTbl.at(i) = mDevice->CreateUnorderedAccessView(mDepthBufferTbl.at(i).Get(), &uavDesc);
    }
}

void DxrPhotonMapper::CreateLuminanceMomentBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    const u32 size = 2;

    mLuminanceMomentBufferTbl.resize(size);
    mLuminanceMomentBufferDescriptorUAVTbl.resize(size);
    mLuminanceMomentBufferDescriptorSRVTbl.resize(size);

    for (u32 i = 0; i < size; i++)
    {
        wchar_t name[30];
        swprintf(name, 30, L"LuminanceMomentBufferTbl[%d]", i);
        mLuminanceMomentBufferTbl.at(i) = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R16G16_FLOAT,//float2
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            (i % 2 == 0) ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_UNORDERED_ACCESS,//renderframe will be 1 on first execute time
            D3D12_HEAP_TYPE_DEFAULT,
            name
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mLuminanceMomentBufferDescriptorSRVTbl.at(i) = mDevice->CreateShaderResourceView(mLuminanceMomentBufferTbl.at(i).Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mLuminanceMomentBufferDescriptorUAVTbl.at(i) = mDevice->CreateUnorderedAccessView(mLuminanceMomentBufferTbl.at(i).Get(), &uavDesc);
    }
}

void DxrPhotonMapper::CreateLuminanceVarianceBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();

    const u32 size = 2;

    mLuminanceVarianceBufferTbl.resize(size);
    mLuminanceVarianceBufferDescriptorUAVTbl.resize(size);
    mLuminanceVarianceBufferDescriptorSRVTbl.resize(size);

    for (u32 i = 0; i < size; i++)
    {
        wchar_t name[30];
        swprintf(name, 30, L"LuminanceVarianceBufferTbl[%d]", i);
        mLuminanceVarianceBufferTbl.at(i) = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R16_FLOAT,//float
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            (i % 2 == 0) ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            name
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mLuminanceVarianceBufferDescriptorSRVTbl.at(i) = mDevice->CreateShaderResourceView(mLuminanceVarianceBufferTbl.at(i).Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mLuminanceVarianceBufferDescriptorUAVTbl.at(i) = mDevice->CreateUnorderedAccessView(mLuminanceVarianceBufferTbl.at(i).Get(), &uavDesc);
    }
}

void DxrPhotonMapper::CreatePhotonMappingBuffer()
{
    const u32 photonMapElements = mPhotonMapSize1D * mPhotonMapSize1D;
    const u32 photonMapSizeInBytes = photonMapElements * sizeof(PhotonInfo);
    const u32 gridElements = GRID_DIMENSION * GRID_DIMENSION * GRID_DIMENSION;
    const u32 gridSizeInBytes = gridElements * sizeof(XMUINT2);

    {
        mPhotonMap = mDevice->CreateBuffer(photonMapSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, L"PhotonMap");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = photonMapElements;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.StructureByteStride = sizeof(PhotonInfo);
        mPhotonMapDescriptorSRV = mDevice->CreateShaderResourceView(mPhotonMap.Get(), &srvDesc);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = photonMapElements;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = sizeof(PhotonInfo);
        mPhotonMapDescriptorUAV = mDevice->CreateUnorderedAccessView(mPhotonMap.Get(), &uavDesc);
    }

    {
        mPhotonMapSorted = mDevice->CreateBuffer(photonMapSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, L"PhotonMapSorted");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = photonMapElements;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.StructureByteStride = sizeof(PhotonInfo);
        mPhotonMapSortedDescriptorSRV = mDevice->CreateShaderResourceView(mPhotonMapSorted.Get(), &srvDesc);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = photonMapElements;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = sizeof(PhotonInfo);
        mPhotonMapSortedDescriptorUAV = mDevice->CreateUnorderedAccessView(mPhotonMapSorted.Get(), &uavDesc);
    }
    
    {
        mPhotonGrid = mDevice->CreateBuffer(photonMapSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, L"PhotonGrid");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = photonMapElements;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.StructureByteStride = sizeof(XMUINT2);
        mPhotonGridDescriptorSRV = mDevice->CreateShaderResourceView(mPhotonGrid.Get(), &srvDesc);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = photonMapElements;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = sizeof(XMUINT2);
        mPhotonGridDescriptorUAV = mDevice->CreateUnorderedAccessView(mPhotonGrid.Get(), &uavDesc);
    }

    {
        mPhotonGridTmp = mDevice->CreateBuffer(photonMapSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, L"PhotonGridTmp");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = photonMapElements;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.StructureByteStride = sizeof(XMUINT2);
        mPhotonGridTmpDescriptorSRV = mDevice->CreateShaderResourceView(mPhotonGridTmp.Get(), &srvDesc);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = photonMapElements;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = sizeof(XMUINT2);
        mPhotonGridTmpDescriptorUAV = mDevice->CreateUnorderedAccessView(mPhotonGridTmp.Get(), &uavDesc);
    }
   
    {
        mPhotonGridId = mDevice->CreateBuffer(gridSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, L"PhotonGridId");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = gridElements;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.StructureByteStride = sizeof(XMUINT2);
        mPhotonGridIdDescriptorSRV = mDevice->CreateShaderResourceView(mPhotonGridId.Get(), &srvDesc);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = gridElements;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = sizeof(XMUINT2);
        mPhotonGridIdDescriptorUAV = mDevice->CreateUnorderedAccessView(mPhotonGridId.Get(), &uavDesc);
    }
   
}


void DxrPhotonMapper::CreateBitonicSortCBs()
{
    const u32 NUM_ELEMENTS = mPhotonMapSize1D * mPhotonMapSize1D;;//[2] is for Reflection at Refraction
    const u32 MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
    const u32 MATRIX_HEIGHT = (u32)NUM_ELEMENTS / MATRIX_WIDTH;

    u32 count = 0;
    //BitonicSortLDS
    {
        for (u32 level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1)
        {
            count++;
        }
        mBitonicLDSCB0Tbl.resize(count);
        for (s32 count = 0; count < mBitonicLDSCB0Tbl.size(); count++)
        {
            wchar_t name[30];
            swprintf(name, 30, L"BitonicLDSCB0[%d]", count);

            mBitonicLDSCB0Tbl.at(count) = mDevice->CreateConstantBuffer(sizeof(BitonicSortCB), nullptr, name);
        }
        count = 0;
        for (u32 level = (BITONIC_BLOCK_SIZE << 1); level <= NUM_ELEMENTS; level <<= 1)
        {
            count++;
        }
        const u32 size = count;
        mBitonicLDSCB1Tbl.resize(size);
        mBitonicLDSCB2Tbl.resize(size);
        for (u32 count = 0; count < size; count++)
        {
            wchar_t name[30];

            swprintf(name, 30, L"BitonicLDSCB1[%d]", count);
            mBitonicLDSCB1Tbl.at(count) = mDevice->CreateConstantBuffer(sizeof(BitonicSortCB), nullptr, name);
            swprintf(name, 30, L"BitonicLDSCB2[%d]", count);
            mBitonicLDSCB2Tbl.at(count) = mDevice->CreateConstantBuffer(sizeof(BitonicSortCB), nullptr, name);
        }
    }

    //BitonicSortSimple
    {
        s32 nlog = (s32)(log2(NUM_ELEMENTS));
        count = 0;
        for (s32 i = 0; i < nlog; i++)
        {
            for (s32 j = 0; j < i + 1; j++)
            {
                wchar_t name[30];
                swprintf(name, 30, L"BitonicSortSimpleCB[%d]", count);

                mBitonicSimpleCBTbl.push_back(mDevice->CreateConstantBuffer(sizeof(BitonicSortCB2), nullptr, name));

                count++;
            }
        }
    }
}

void DxrPhotonMapper::CreateGridSortCB()
{
    mGridSortCB = mDevice->CreateConstantBuffer(sizeof(GridCB), nullptr, L"GridSortCB");
}

void DxrPhotonMapper::CreateDenoiseCBs()
{
    mDenoiseCBTbl.resize(DENOISE_ITE);

    for (u32 i = 0; i < DENOISE_ITE; i++)
    {
        wchar_t name[30];
        swprintf(name, 30, L"DenoiseCB[%d]", i);
        mDenoiseCBTbl.at(i) = mDevice->CreateConstantBuffer(sizeof(DenoiseCB), nullptr, name);
    }
}

void DxrPhotonMapper::CreateRegularBuffer()
{
    CreateResultBuffer();
    CreateDenoisedColorBuffer();
    CreatePositionBuffer();
    CreateNormalBuffer();
    CreateDepthBuffer();
    CreateLuminanceMomentBuffer();
    CreateLuminanceVarianceBuffer();
    CreatePhotonMappingBuffer();
    mGroundTex = utility::LoadTextureFromFile(mDevice, mStageTextureFileName);
    mCubeMapTex = utility::LoadTextureFromFile(mDevice, mCubeMapTextureFileName);
}

void DxrPhotonMapper::CreateConstantBuffer()
{
    CreateBitonicSortCBs();
    CreateGridSortCB();
    CreateDenoiseCBs();
    mSceneCB = mDevice->CreateConstantBuffer(sizeof(SceneParam), nullptr, L"SceneCB");
}