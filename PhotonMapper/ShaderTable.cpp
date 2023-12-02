#include "DxrPhotonMapper.h"

void DxrPhotonMapper::CreateShaderTable()
{
    //Ordinal
    {
        const auto ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        u32 raygenRecordSize = 0;
        raygenRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        //raygenRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);//RWTexture2D<float4> gOutput: register(u0);
        raygenRecordSize = utility::RoundUp(raygenRecordSize, ShaderRecordAlignment);

        u32 hitgroupRecordSize = 0;
        hitgroupRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
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
            + NormalSpheres
            + NormalBoxes
            + NormalOBJ0s
            + NormalOBJ1s;
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

        auto rgStartPtr = startPtr;
        {
            auto recordStartPtr = rgStartPtr;
            uint8_t* ptr = rgStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(RayTracingEntryPoints::RayGen);
            if (idPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }
            memcpy(ptr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

            recordStartPtr += raygenRecordSize;
        }

        auto missStartPtr = startPtr + raygenRegion;
        {
            auto recordStartPtr = missStartPtr;
            uint8_t* ptr = missStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(RayTracingEntryPoints::Miss);
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
                auto cbAddress = mNormalSphereMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& instances : mSpheresNormalTbl) {
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

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }
       
            {
                auto cbAddress = mNormalBoxMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& instances : mBoxesNormalTbl) {
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

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }
         
            {
                auto cbAddress = mOBJ0MaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);

                for (auto& instances : mOBJ0MaterialTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Obj0);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshOBJ0.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshOBJ0.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }

            {
                auto cbAddress = mOBJ1MaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);

                for (auto& instances : mOBJ1MaterialTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Obj1);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshOBJ1.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshOBJ1.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }
          
            {
                auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Light);
                if (idPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto recordTmpPtr = recordStartPtr;
                memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
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
        const auto ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        u32 raygenRecordSize = 0;
        raygenRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        raygenRecordSize = utility::RoundUp(raygenRecordSize, ShaderRecordAlignment);
        
        u32 hitgroupRecordSize = 0;
        hitgroupRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
        hitgroupRecordSize = utility::RoundUp(hitgroupRecordSize, ShaderRecordAlignment);

        u32 missRecordSize = 0;
        missRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        missRecordSize = utility::RoundUp(missRecordSize, ShaderRecordAlignment);

        u32 raygenSize = 1 * raygenRecordSize;
        u32 missSize = 1 * missRecordSize;
        //combination of shader and resource(vertivces)
        u32 hitgroupCount =
            1 //floor
            + NormalSpheres
            + NormalBoxes
            + NormalOBJ0s
            + NormalOBJ1s;
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

        auto rgStartPtr = startPtr;
        {
            auto recordStartPtr = rgStartPtr;
            uint8_t* ptr = rgStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(RayTracingEntryPoints::RayGenPhoton);
            if (idPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }
            memcpy(ptr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

            recordStartPtr += raygenRecordSize;
        }

        auto missStartPtr = startPtr + raygenRegion;
        {
            auto recordStartPtr = missStartPtr;
            uint8_t* ptr = missStartPtr;
            auto idPtr = rtsoProps->GetShaderIdentifier(RayTracingEntryPoints::MissPhoton);
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
                auto cbAddress = mNormalSphereMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& instances : mSpheresNormalTbl) {
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

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }
         
            {
                auto cbAddress = mNormalBoxMaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);
                for (auto& instances : mBoxesNormalTbl) {
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

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }
          
            {
                auto cbAddress = mOBJ0MaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);

                for (auto& instances : mOBJ0MaterialTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Obj0);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshOBJ0.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshOBJ0.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
            }

            {
                auto cbAddress = mOBJ1MaterialCB->GetGPUVirtualAddress();
                auto cbStride = sizeof(MaterialParam);

                for (auto& instances : mOBJ1MaterialTbl) {
                    auto idPtr = rtsoProps->GetShaderIdentifier(HitGroups::Obj1);
                    if (idPtr == nullptr) {
                        throw std::logic_error("Not found ShaderIdentifier");
                    }
                    auto recordTmpPtr = recordStartPtr;
                    memcpy(recordStartPtr, idPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    recordStartPtr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                    memcpy(recordStartPtr, &mMeshOBJ1.descriptorIB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &mMeshOBJ1.descriptorVB.hGpu.ptr, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);
                    memcpy(recordStartPtr, &cbAddress, sizeof(UINT64));
                    recordStartPtr += sizeof(UINT64);

                    cbAddress += cbStride;
                    recordStartPtr = recordTmpPtr + hitgroupRecordSize;
                }
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