#include "DxrPhotonMapper.h"

void DxrPhotonMapper::CreateShaderTable()
{
    //Ordinal
    {
        const auto ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        u32 raygenRecordSize = 0;
        raygenRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        raygenRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);//RWTexture2D<float4> gOutput: register(u0);
        raygenRecordSize = utility::RoundUp(raygenRecordSize, ShaderRecordAlignment);

        u32 hitgroupRecordSize = 0;
        hitgroupRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);//StructuredBuffer<u32>   indexBuffer : register(t0, space1);
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);//StructuredBuffer<VertexPNT> vertexBuffer: register(t1, space1);
        hitgroupRecordSize = utility::RoundUp(hitgroupRecordSize, ShaderRecordAlignment);

        u32 missRecordSize = 0;
        missRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        missRecordSize = utility::RoundUp(missRecordSize, ShaderRecordAlignment);

        u32 raygenSize = 1 * raygenRecordSize;
        u32 missSize = 1 * missRecordSize;
        u32 hitgroupCount = 
            1 //floor
            + 1 //sphere reflect/reflact
            + 1 //box reflect/reflact
            + NormalSpheres //sphere normal
            + NormalBoxes //box normal
            + 1 //glass
            + 1 //metal
            + 1;//light
        u32 hitGroupSize = hitgroupCount * hitgroupRecordSize;

        auto tableAlign = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        auto raygenRegion = utility::RoundUp(raygenSize, tableAlign);
        auto missRegion = utility::RoundUp(missSize, tableAlign);
        auto hitgroupRegion = utility::RoundUp(hitGroupSize, tableAlign);

        auto tableSize = raygenRegion + missRegion + hitgroupRegion;
        mShaderTable = mDevice->CreateBuffer(tableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_UPLOAD, nullptr, L"ShaderTable");

        ComPtr<ID3D12StateObjectProperties> rtsoProps;
        mRTPSO.As(&rtsoProps);

        void* mappedResPtr = nullptr;
        mShaderTable->Map(0, nullptr, &mappedResPtr);
        uint8_t* startPtr = static_cast<uint8_t*>(mappedResPtr);

        auto rgsStartPtr = startPtr;
        {
            uint8_t* ptr = rgsStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(L"rayGen");
            if (idPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }
            memcpy(ptr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        }

        auto missStartPtr = startPtr + raygenRegion;
        {
            auto recordStartPtr = missStartPtr;
            uint8_t* ptr = missStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(L"miss");
            if (idPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }
            memcpy(ptr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

            recordStartPtr += missRecordSize;
        }

        auto hitgroupStart = startPtr + raygenRegion + missRegion;
        {
            auto recordStartPtr = hitgroupStart;

            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Floor);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshStage.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshStage.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mGroundTex.srv.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
           
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::ReflectReflactMaterialSphere);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshSphere.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshSphere.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
        
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::ReflectReflactMaterialBox);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshBox.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshBox.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
         
            {
                auto cbAddress = mNormalSphereMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& sphere : mSpheresNormalTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::DefaultMaterialSphere);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshSphere.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshSphere.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                    cbAddress += cbStride;
                }
            }
       
            {
                auto cbAddress = mNormalBoxMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& box : mBoxesNormalTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::DefaultMaterialBox);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshBox.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshBox.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                    cbAddress += cbStride;
                }
            }
         
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Glass);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshGlass.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshGlass.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }

            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Metal);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshMetal.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshMetal.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
          
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Light);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshLightSphere.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshLightSphere.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
        }

        mShaderTable->Unmap(0, nullptr);
        
        auto startAddress = mShaderTable->GetGPUVirtualAddress();
        auto& shaderRecordRG = mDispatchRayDesc.RayGenerationShaderRecord;
        shaderRecordRG.StartAddress = startAddress;
        shaderRecordRG.SizeInBytes = raygenRecordSize;
        startAddress += raygenRegion;

        auto& shaderRecordMS = mDispatchRayDesc.MissShaderTable;
        shaderRecordMS.StartAddress = startAddress;
        shaderRecordMS.SizeInBytes = missSize;
        shaderRecordMS.StrideInBytes = missRecordSize;
        startAddress += missRegion;

        auto& shaderRecordHG = mDispatchRayDesc.HitGroupTable;
        shaderRecordHG.StartAddress = startAddress;
        shaderRecordHG.SizeInBytes = hitGroupSize;
        shaderRecordHG.StrideInBytes = hitgroupRecordSize;
        startAddress += hitGroupSize;

        mDispatchRayDesc.Width = GetWidth();
        mDispatchRayDesc.Height = GetHeight();
        mDispatchRayDesc.Depth = 1;
    }

    //Photon
    {
        const auto ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;\
        u32 raygenRecordSize = 0;
        raygenRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;\
        raygenRecordSize = utility::RoundUp(raygenRecordSize, ShaderRecordAlignment);
        
        u32 hitgroupRecordSize = 0;
        hitgroupRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize = utility::RoundUp(hitgroupRecordSize, ShaderRecordAlignment);

        u32 missRecordSize = 0;
        missRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        missRecordSize = utility::RoundUp(missRecordSize, ShaderRecordAlignment);

        u32 raygenSize = 1 * raygenRecordSize;
        u32 missSize = 1 * missRecordSize;
        u32 hitgroupCount =
            1 //floor
            + 1 //sphere reflect/reflact
            + 1 //box reflect/reflact
            + NormalSpheres //sphere normal
            + NormalBoxes //box normal
            + 1 //glass
            + 1; //metal
        u32 hitGroupSize = hitgroupCount * hitgroupRecordSize;

        auto tableAlign = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        auto raygenRegion = utility::RoundUp(raygenSize, tableAlign);
        auto missRegion = utility::RoundUp(missSize, tableAlign);
        auto hitgroupRegion = utility::RoundUp(hitGroupSize, tableAlign);

        auto tableSize = raygenRegion + missRegion + hitgroupRegion;
        mShaderPhotonTable = mDevice->CreateBuffer(tableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_UPLOAD, nullptr, L"ShaderTablePhoton");

        ComPtr<ID3D12StateObjectProperties> rtsoProps;
        mRTPSOPhoton.As(&rtsoProps);

        void* mappedResPtr = nullptr;
        mShaderPhotonTable->Map(0, nullptr, &mappedResPtr);
        uint8_t* startPtr = static_cast<uint8_t*>(mappedResPtr);

        auto rgsStartPtr = startPtr;
        {
            uint8_t* ptr = rgsStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(L"photonEmitting");
            if (idPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }
            memcpy(ptr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        }

        auto missStartPtr = startPtr + raygenRegion;
        {
            auto recordStartPtr = missStartPtr;
            uint8_t* ptr = missStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(L"photonMiss");
            if (idPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }
            memcpy(ptr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        }

        auto hitgroustartPtr = startPtr + raygenRegion + missRegion;
        {
            auto recordStartPtr = hitgroustartPtr;

            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Floor);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshStage.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshStage.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mGroundTex.srv.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
          
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::ReflectReflactMaterialSphere);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshSphere.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshSphere.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
         
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::ReflectReflactMaterialBox);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshBox.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshBox.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
       
            {
                auto cbAddress = mNormalSphereMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& sphere : mSpheresNormalTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::DefaultMaterialSphere);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshSphere.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshSphere.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                    cbAddress += cbStride;
                }
            }
         
            {
                auto cbAddress = mNormalBoxMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& box : mBoxesNormalTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::DefaultMaterialBox);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshBox.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshBox.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                    cbAddress += cbStride;
                }
            }
          
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Glass);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshGlass.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshGlass.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }

            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Metal);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(recordStartPtr, &mMeshMetal.descriptorIB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);
                memcpy(recordStartPtr, &mMeshMetal.descriptorVB.hGpu.ptr, sizeof(UINT64));
                recordStartPtr += sizeof(UINT64);

                recordStartPtr = recordTmpPtr + hitgroupRecordSize;
            }
        }

        mShaderPhotonTable->Unmap(0, nullptr);

        auto startAddress = mShaderPhotonTable->GetGPUVirtualAddress();
        auto& shaderRecordRG = mDispatchPhotonRayDesc.RayGenerationShaderRecord;
        shaderRecordRG.StartAddress = startAddress;
        shaderRecordRG.SizeInBytes = raygenRecordSize;
        startAddress += raygenRegion;

        auto& shaderRecordMS = mDispatchPhotonRayDesc.MissShaderTable;
        shaderRecordMS.StartAddress = startAddress;
        shaderRecordMS.SizeInBytes = missSize;
        shaderRecordMS.StrideInBytes = missRecordSize;
        startAddress += missRegion;

        auto& shaderRecordHG = mDispatchPhotonRayDesc.HitGroupTable;
        shaderRecordHG.StartAddress = startAddress;
        shaderRecordHG.SizeInBytes = hitGroupSize;
        shaderRecordHG.StrideInBytes = hitgroupRecordSize;
        startAddress += hitGroupSize;

        mDispatchPhotonRayDesc.Width = mPhotonMapSize1D;
        mDispatchPhotonRayDesc.Height = mPhotonMapSize1D;
        mDispatchPhotonRayDesc.Depth = 1;
    }
}