#include "DxrPhotonMapper.h"

using namespace DirectX;

void DxrPhotonMapper::CreateLightGenerationBuffer()
{
    const u32 lightBufferSizeInBites = mLightGenerationParamTbl.size() * sizeof(LightGenerateParam);
    mLightGenerationParamBuffer = mDevice->CreateBuffer(lightBufferSizeInBites, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_HEAP_TYPE_UPLOAD, nullptr, L"LightGenerateParams");
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = mLightGenerationParamTbl.size();
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(LightGenerateParam);
    mLightGenerationParamSRV = mDevice->CreateShaderResourceView(mLightGenerationParamBuffer.Get(), &srvDesc);
}

void DxrPhotonMapper::CreateRegularBuffer()
{
    auto width = GetWidth();
    auto height = GetHeight();
    //RayTraced Result
    {
        mFinalRenderResult = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_HEAP_TYPE_DEFAULT,
            L"FinalRenderResult"
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mFinalRenderResultDescriptorSRV = mDevice->CreateShaderResourceView(mFinalRenderResult.Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mFinalRenderResultDescriptorUAV = mDevice->CreateUnorderedAccessView(mFinalRenderResult.Get(), &uavDesc);

        mAccumulationBuffer = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            L"AccumulationBuffer"
        );

        mAccumulationBufferDescriptorUAV = mDevice->CreateUnorderedAccessView(mAccumulationBuffer.Get(), &uavDesc);
    }
    //Denoised Color
    {
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
    //Prev ID
    {
        mPrevIDBuffer = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R16G16_FLOAT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            L"PrevIDBuffer"
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mPrevIDBufferDescriptorSRV = mDevice->CreateShaderResourceView(mPrevIDBuffer.Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mPrevIDBufferDescriptorUAV = mDevice->CreateUnorderedAccessView(mPrevIDBuffer.Get(), &uavDesc);
    }
    //Ping Pong
    {
        const u32 size = 2;

        mNormalDepthBufferTbl.resize(size);
        mNormalDepthBufferDescriptorSRVTbl.resize(size);
        mNormalDepthBufferDescriptorUAVTbl.resize(size);

        mPositionBufferTbl.resize(size);
        mPositionBufferDescriptorSRVTbl.resize(size);
        mPositionBufferDescriptorUAVTbl.resize(size);

        mAccumulationCountBufferTbl.resize(size);
        mAccumulationCountBufferDescriptorSRVTbl.resize(size);
        mAccumulationCountBufferDescriptorUAVTbl.resize(size);

        for (u32 i = 0; i < size; i++)
        {
             //Accumulation Count
            { 
                wchar_t name[30];
                swprintf(name, 30, L"AccumulationCountBufferTbl[%d]", i);
                mAccumulationCountBufferTbl.at(i) = mDevice->CreateTexture2D(
                    width, height, DXGI_FORMAT_R32_UINT,
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
                mAccumulationCountBufferDescriptorSRVTbl.at(i) = mDevice->CreateShaderResourceView(mAccumulationCountBufferTbl.at(i).Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mAccumulationCountBufferDescriptorUAVTbl.at(i) = mDevice->CreateUnorderedAccessView(mAccumulationCountBufferTbl.at(i).Get(), &uavDesc);
            }
            //Normal Depth
            {
                wchar_t name[30];
                swprintf(name, 30, L"NormalDepthBufferTbl[%d]", i);
                mNormalDepthBufferTbl.at(i) = mDevice->CreateTexture2D(
                    width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
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
                mNormalDepthBufferDescriptorSRVTbl.at(i) = mDevice->CreateShaderResourceView(mNormalDepthBufferTbl.at(i).Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mNormalDepthBufferDescriptorUAVTbl.at(i) = mDevice->CreateUnorderedAccessView(mNormalDepthBufferTbl.at(i).Get(), &uavDesc);
            }
            //Position
            {
                wchar_t name[30];
                swprintf(name, 30, L"PositionBufferTbl[%d]", i);
                mPositionBufferTbl.at(i) = mDevice->CreateTexture2D(
                    width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
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
                mPositionBufferDescriptorSRVTbl.at(i) = mDevice->CreateShaderResourceView(mPositionBufferTbl.at(i).Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mPositionBufferDescriptorUAVTbl.at(i) = mDevice->CreateUnorderedAccessView(mPositionBufferTbl.at(i).Get(), &uavDesc);
            }
        }
    }
    //Moment
    {
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
    //Variance
    {
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
    //Light Buffer
    {
        CreateLightGenerationBuffer();
    }
    //Photon Map
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

        //photon guiding
        {
            //counter map
            {
                mPhotonRandomCounterMap = mDevice->CreateTexture2D(
                    PHOTON_RANDOM_COUNTER_MAP_SIZE_1D, PHOTON_RANDOM_COUNTER_MAP_SIZE_1D, DXGI_FORMAT_R16_UINT,
                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_HEAP_TYPE_DEFAULT,
                    L"PhotonRandomCounterMap"
                );

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                mPhotonRandomCounterMapDescriptorSRV = mDevice->CreateShaderResourceView(mPhotonRandomCounterMap.Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mPhotonRandomCounterMapDescriptorUAV = mDevice->CreateUnorderedAccessView(mPhotonRandomCounterMap.Get(), &uavDesc);
            }

            //guide map
            {
                for (u32 i = 0; i < PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL; i++)
                {
                    wchar_t name[30];
                    swprintf(name, 30, L"PhotonEmissionGuideMipMap[%d]", i);

                    mPhotonEmissionGuideMipMapTbl[i] = mDevice->CreateTexture2D(
                        PHOTON_EMISSION_GUIDE_MAP_SIZE_1D >> i, PHOTON_EMISSION_GUIDE_MAP_SIZE_1D >> i, DXGI_FORMAT_R16_FLOAT,
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
                    mPhotonEmissionGuideMipMapDescriptorSRVTbl[i] = mDevice->CreateShaderResourceView(mPhotonEmissionGuideMipMapTbl[i].Get(), &srvDesc);

                    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    mPhotonEmissionGuideMipMapDescriptorUAVTbl[i] = mDevice->CreateUnorderedAccessView(mPhotonEmissionGuideMipMapTbl[i].Get(), &uavDesc);
                }
            }
        }
    }
    // DI Gi
    {
        const u32 size = 2;

        mDIBufferPingPongTbl.resize(size);
        mDIBufferDescriptorSRVPingPongTbl.resize(size);
        mDIBufferDescriptorUAVPingPongTbl.resize(size);

        mGIBufferPingPongTbl.resize(size);
        mGIBufferDescriptorSRVPingPongTbl.resize(size);
        mGIBufferDescriptorUAVPingPongTbl.resize(size);

        mCausticsBufferPingPongTbl.resize(size);
        mCausticsBufferDescriptorSRVPingPongTbl.resize(size);
        mCausticsBufferDescriptorUAVPingPongTbl.resize(size);

        mDIReservoirPingPongTbl.resize(size);
        mDIReservoirDescriptorSRVPingPongTbl.resize(size);
        mDIReservoirDescriptorUAVPingPongTbl.resize(size);

        mDISpatialReservoirPingPongTbl.resize(size);
        mDISpatialReservoirDescriptorSRVPingPongTbl.resize(size);
        mDISpatialReservoirDescriptorUAVPingPongTbl.resize(size);

        for (u32 i = 0; i < size; i++)
        {
            {
                wchar_t name[30];
                swprintf(name, 30, L"DIBufferTbl[%d]", i);
                mDIBufferPingPongTbl.at(i) = mDevice->CreateTexture2D(
                    width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
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
                mDIBufferDescriptorSRVPingPongTbl.at(i) = mDevice->CreateShaderResourceView(mDIBufferPingPongTbl.at(i).Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mDIBufferDescriptorUAVPingPongTbl.at(i) = mDevice->CreateUnorderedAccessView(mDIBufferPingPongTbl.at(i).Get(), &uavDesc);
            }
            {
                wchar_t name[30];
                swprintf(name, 30, L"GIBufferTbl[%d]", i);
                mGIBufferPingPongTbl.at(i) = mDevice->CreateTexture2D(
                    width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
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
                mGIBufferDescriptorSRVPingPongTbl.at(i) = mDevice->CreateShaderResourceView(mGIBufferPingPongTbl.at(i).Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mGIBufferDescriptorUAVPingPongTbl.at(i) = mDevice->CreateUnorderedAccessView(mGIBufferPingPongTbl.at(i).Get(), &uavDesc);
            }
            {
                wchar_t name[30];
                swprintf(name, 30, L"CausticsBufferTbl[%d]", i);
                mCausticsBufferPingPongTbl.at(i) = mDevice->CreateTexture2D(
                    width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
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
                mCausticsBufferDescriptorSRVPingPongTbl.at(i) = mDevice->CreateShaderResourceView(mCausticsBufferPingPongTbl.at(i).Get(), &srvDesc);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mCausticsBufferDescriptorUAVPingPongTbl.at(i) = mDevice->CreateUnorderedAccessView(mCausticsBufferPingPongTbl.at(i).Get(), &uavDesc);
            }
            {
                wchar_t name[30];
                swprintf(name, 30, L"DIReservoirTbl[%d]", i);

                const u32 DIReservoirElements = GetWidth() * GetHeight();
                const u32 DIReservoirSizeInBytes = DIReservoirElements * sizeof(DIReservoir);

                mDIReservoirPingPongTbl.at(i) = mDevice->CreateBuffer(DIReservoirSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, name);
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Buffer.NumElements = DIReservoirElements;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.StructureByteStride = sizeof(DIReservoir);
                mDIReservoirDescriptorSRVPingPongTbl.at(i) = mDevice->CreateShaderResourceView(mDIReservoirPingPongTbl.at(i).Get(), &srvDesc);
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                uavDesc.Buffer.NumElements = DIReservoirElements;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.StructureByteStride = sizeof(DIReservoir);
                mDIReservoirDescriptorUAVPingPongTbl.at(i) = mDevice->CreateUnorderedAccessView(mDIReservoirPingPongTbl.at(i).Get(), &uavDesc);
            }
            {
                wchar_t name[30];
                swprintf(name, 30, L"DISpatialReservoirTbl[%d]", i);

                const u32 DIReservoirElements = GetWidth() * GetHeight();
                const u32 DIReservoirSizeInBytes = DIReservoirElements * sizeof(DIReservoir);

                mDISpatialReservoirPingPongTbl.at(i) = mDevice->CreateBuffer(DIReservoirSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, name);
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Buffer.NumElements = DIReservoirElements;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.StructureByteStride = sizeof(DIReservoir);
                mDISpatialReservoirDescriptorSRVPingPongTbl.at(i) = mDevice->CreateShaderResourceView(mDISpatialReservoirPingPongTbl.at(i).Get(), &srvDesc);
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                uavDesc.Buffer.NumElements = DIReservoirElements;
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.StructureByteStride = sizeof(DIReservoir);
                mDISpatialReservoirDescriptorUAVPingPongTbl.at(i) = mDevice->CreateUnorderedAccessView(mDISpatialReservoirPingPongTbl.at(i).Get(), &uavDesc);
            }
        }
    }
    //Screen Space Material
    {
        const u32 ScreenSpaceMaterialElements = GetWidth() * GetHeight();
        const u32 ScreenSpaceMaterialSizeInBytes = ScreenSpaceMaterialElements * sizeof(CompressedMaterialParams);

        mScreenSpaceMaterialBuffer = mDevice->CreateBuffer(ScreenSpaceMaterialSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, nullptr, L"ScreenSpaceMaterial");
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = ScreenSpaceMaterialElements;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.StructureByteStride = sizeof(CompressedMaterialParams);
        mScreenSpaceMaterialBufferDescriptorSRV = mDevice->CreateShaderResourceView(mScreenSpaceMaterialBuffer.Get(), &srvDesc);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.NumElements = ScreenSpaceMaterialElements;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.StructureByteStride = sizeof(CompressedMaterialParams);
        mScreenSpaceMaterialBufferDescriptorUAV = mDevice->CreateUnorderedAccessView(mScreenSpaceMaterialBuffer.Get(), &uavDesc);
    }
    //DebugTexture
    {
        mDebugTexture = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            L"DebugTexture"
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mDebugTextureDescriptorSRV = mDevice->CreateShaderResourceView(mDebugTexture.Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mDebugTextureDescriptorUAV = mDevice->CreateUnorderedAccessView(mDebugTexture.Get(), &uavDesc);
    }
    {
        mDebugTexture0 = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            L"DebugTexture0"
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mDebugTexture0DescriptorSRV = mDevice->CreateShaderResourceView(mDebugTexture0.Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mDebugTexture0DescriptorUAV = mDevice->CreateUnorderedAccessView(mDebugTexture0.Get(), &uavDesc);
    }
    {
        mDebugTexture1 = mDevice->CreateTexture2D(
            width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_HEAP_TYPE_DEFAULT,
            L"DebugTexture1"
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mDebugTexture1DescriptorSRV = mDevice->CreateShaderResourceView(mDebugTexture1.Get(), &srvDesc);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        mDebugTexture1DescriptorUAV = mDevice->CreateUnorderedAccessView(mDebugTexture1.Get(), &uavDesc);
    }
}

void DxrPhotonMapper::CreateConstantBuffer()
{
    //Photon Grid Sorting
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

        mGridSortCB = mDevice->CreateConstantBuffer(sizeof(GridCB), nullptr, L"GridSortCB");
    }
    //Wavelet
    {
        mDenoiseCBTbl.resize(DENOISE_ITE);

        for (u32 i = 0; i < DENOISE_ITE; i++)
        {
            wchar_t name[30];
            swprintf(name, 30, L"DenoiseCB[%d]", i);
            mDenoiseCBTbl.at(i) = mDevice->CreateConstantBuffer(sizeof(DenoiseCB), nullptr, name);
        }
    }
    //Scene
    mSceneCB = mDevice->CreateConstantBuffer(sizeof(SceneParam), nullptr, L"SceneCB");

    for (u32 i = 0; i < MAX_SPATIAL_REUSE_TAP; i++)
    {
        wchar_t name[30];
        swprintf(name, 30, L"ReSTIRParamCB[%d]", i);
        mReSTIRParamCBTbl.push_back(mDevice->CreateConstantBuffer(sizeof(ReSTIRParam), nullptr, name));
    }
}