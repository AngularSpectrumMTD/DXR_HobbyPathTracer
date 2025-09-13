#include "DXRPathTracer.h"

void DXRPathTracer::CreateShaderTable(ComPtr<ID3D12Resource>& shaderTable, ComPtr<ID3D12StateObject>& stateObject, D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc, const u32 maxRootSigSizeRayGen, const u32 maxRootSigSizeMiss, const u32 maxRootSigSizeHitGroup, const wchar_t* shaderTableName, const wchar_t* rayGenShaderName, const wchar_t* missShaderName)
{
    const auto ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
    u32 raygenRecordSize = 0;
    raygenRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    raygenRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * maxRootSigSizeRayGen;
    raygenRecordSize = utility::RoundUp(raygenRecordSize, ShaderRecordAlignment);

    u32 hitgroupRecordSize = 0;
    hitgroupRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * maxRootSigSizeHitGroup;
    hitgroupRecordSize = utility::RoundUp(hitgroupRecordSize, ShaderRecordAlignment);

    u32 missRecordSize = 0;
    missRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    missRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * maxRootSigSizeMiss;
    missRecordSize = utility::RoundUp(missRecordSize, ShaderRecordAlignment);

    u32 raygenSize = 1 * raygenRecordSize;
    u32 missSize = 1 * missRecordSize;
    u32 hitgroupCount =
        1 //floor
        + NormalSpheres
        + NormalBoxes
        + NormalObjs
        + mOBJMaterialLinkedMesh.getMaterialList().size();
    //+ 1;//light
    u32 hitGroupSize = hitgroupCount * hitgroupRecordSize;

    auto tableAlign = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    auto raygenRegion = utility::RoundUp(raygenSize, tableAlign);
    auto missRegion = utility::RoundUp(missSize, tableAlign);
    auto hitgroupRegion = utility::RoundUp(hitGroupSize, tableAlign);

    auto tableSize = raygenRegion + missRegion + hitgroupRegion;
    shaderTable = mDevice->CreateBuffer(tableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_UPLOAD, nullptr, shaderTableName);

    ComPtr<ID3D12StateObjectProperties> RTProperties;
    stateObject.As(&RTProperties);

    void* mappedResPtr = nullptr;
    shaderTable->Map(0, nullptr, &mappedResPtr);
    uint8_t* shaderTablePtr = static_cast<uint8_t*>(mappedResPtr);

    auto rayGenShaderAddressOffset = shaderTablePtr;
    {
        auto recordAddressOffset = rayGenShaderAddressOffset;
        uint8_t* ptr = rayGenShaderAddressOffset;
        auto shaderIDPtr = RTProperties->GetShaderIdentifier(rayGenShaderName);
        if (shaderIDPtr == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        memcpy(ptr, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        recordAddressOffset += raygenRecordSize;
    }

    auto missShaderAddressOffset = shaderTablePtr + raygenRegion;
    {
        auto recordAddressOffset = missShaderAddressOffset;
        uint8_t* ptr = missShaderAddressOffset;
        auto shaderIDPtr = RTProperties->GetShaderIdentifier(missShaderName);
        if (shaderIDPtr == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        memcpy(ptr, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        recordAddressOffset += missRecordSize;
    }

    auto hitgroupAddressOffset = shaderTablePtr + raygenRegion + missRegion;
    {
        auto recordAddressOffset = hitgroupAddressOffset;
        {
            auto cbAddress = mStageMaterialCB->GetGPUVirtualAddress();
            auto shaderIDPtr = RTProperties->GetShaderIdentifier(HitGroups::Floor);
            if (shaderIDPtr == nullptr) {
                throw std::logic_error("Not found ShaderIdentifier");
            }

            auto registerAddressOffset = recordAddressOffset;
            memcpy(registerAddressOffset, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            registerAddressOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
            memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["constantBuffer"], &cbAddress, sizeof(UINT64));
            memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["indexBuffer"], &mMeshStage.descriptorIB.hGpu.ptr, sizeof(UINT64));
            memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["vertexBuffer"], &mMeshStage.descriptorVB.hGpu.ptr, sizeof(UINT64));
            memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["diffuseTex"], &mGroundTex.srv.hGpu.ptr, sizeof(UINT64));
            memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["alphaMask"], &mDummyAlphaMask.srv.hGpu.ptr, sizeof(UINT64));

            recordAddressOffset += hitgroupRecordSize;
        }

        {
            auto cbAddress = mNormalSphereMaterialCB->GetGPUVirtualAddress();
            auto cbStride = sizeof(utility::MaterialParam);
            for (auto& instances : mSpheresNormalTbl) {
                auto shaderIDPtr = RTProperties->GetShaderIdentifier(HitGroups::DefaultMaterialSphere);
                if (shaderIDPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }

                auto registerAddressOffset = recordAddressOffset;
                memcpy(registerAddressOffset, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                registerAddressOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["constantBuffer"], &cbAddress, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["indexBuffer"], &mMeshSphere.descriptorIB.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["vertexBuffer"], &mMeshSphere.descriptorVB.hGpu.ptr, sizeof(UINT64));

                cbAddress += cbStride;
                recordAddressOffset += hitgroupRecordSize;
            }
        }

        {
            auto cbAddress = mNormalBoxMaterialCB->GetGPUVirtualAddress();
            auto cbStride = sizeof(utility::MaterialParam);
            for (auto& instances : mBoxesNormalTbl) {
                auto shaderIDPtr = RTProperties->GetShaderIdentifier(HitGroups::DefaultMaterialBox);
                if (shaderIDPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }

                auto registerAddressOffset = recordAddressOffset;
                memcpy(registerAddressOffset, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                registerAddressOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["constantBuffer"], &cbAddress, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["indexBuffer"], &mMeshBox.descriptorIB.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["vertexBuffer"], &mMeshBox.descriptorVB.hGpu.ptr, sizeof(UINT64));

                cbAddress += cbStride;
                recordAddressOffset += hitgroupRecordSize;
            }
        }

        {
            auto cbAddress = mOBJMaterialCB->GetGPUVirtualAddress();
            auto cbStride = sizeof(utility::MaterialParam);
            u32 count = 0;
            for (auto& instances : mOBJMaterialTbl) {
                auto shaderIDPtr = RTProperties->GetShaderIdentifier(HitGroups::Obj[count]);
                if (shaderIDPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto registerAddressOffset = recordAddressOffset;
                memcpy(registerAddressOffset, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                registerAddressOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["constantBuffer"], &cbAddress, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["indexBuffer"], &mMeshOBJTbl[count].descriptorIB.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterial["vertexBuffer"], &mMeshOBJTbl[count].descriptorVB.hGpu.ptr, sizeof(UINT64));

                cbAddress += cbStride;
                recordAddressOffset += hitgroupRecordSize;
                count++;
            }
        }

        {
            auto cbStride = sizeof(utility::MaterialParam);

            for (const auto& instances : mOBJMaterialLinkedMesh.getMaterialList())
            {
                wchar_t nameHitGroup[60];
                swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());

                auto shaderIDPtr = RTProperties->GetShaderIdentifier(nameHitGroup);
                if (shaderIDPtr == nullptr) {
                    throw std::logic_error("Not found ShaderIdentifier");
                }
                auto cbAddress = instances.materialCB->GetGPUVirtualAddress();

                auto registerAddressOffset = recordAddressOffset;
                memcpy(registerAddressOffset, shaderIDPtr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                registerAddressOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["constantBuffer"], &cbAddress, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["indexBuffer"], &instances.descriptorTriangleIB.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["vertexBuffer"], &instances.descriptorTriangleVB.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["diffuseTex"], &instances.DiffuseTexture.srv.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["alphaMask"], &instances.AlphaMask.srv.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["bumpMap"], &instances.BumpMap.srv.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["normalMap"], &instances.NormalMap.srv.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["roughnessMap"], &instances.RoughnessMap.srv.hGpu.ptr, sizeof(UINT64));
                memcpy(registerAddressOffset + sizeof(UINT64) * mRegisterMapGlobalLocalRootSigMaterialWithTex["metalnessMap"], &instances.MetallnessMap.srv.hGpu.ptr, sizeof(UINT64));

                cbAddress += cbStride;
                recordAddressOffset += hitgroupRecordSize;
            }
        }
    }

    shaderTable->Unmap(0, nullptr);

    auto startAddress = shaderTable->GetGPUVirtualAddress();
    auto& shaderRecordRG = dispatchRaysDesc.RayGenerationShaderRecord;
    shaderRecordRG.StartAddress = startAddress;
    shaderRecordRG.SizeInBytes = raygenRecordSize;
    startAddress += raygenRegion;

    auto& shaderRecordMS = dispatchRaysDesc.MissShaderTable;
    shaderRecordMS.StartAddress = startAddress;
    shaderRecordMS.SizeInBytes = missSize;
    shaderRecordMS.StrideInBytes = missRecordSize;
    startAddress += missRegion;

    auto& shaderRecordHG = dispatchRaysDesc.HitGroupTable;
    shaderRecordHG.StartAddress = startAddress;
    shaderRecordHG.SizeInBytes = hitGroupSize;
    shaderRecordHG.StrideInBytes = hitgroupRecordSize;
    startAddress += hitGroupSize;

    dispatchRaysDesc.Width = GetWidth();
    dispatchRaysDesc.Height = GetHeight();
    dispatchRaysDesc.Depth = 1;
}

void DXRPathTracer::CreateShaderTables()
{
    CreateShaderTable(mShaderTable, mRTPSO, mDispatchRayDesc, 0, 0, max(mRegisterMapGlobalLocalRootSigMaterial.size(), mRegisterMapGlobalLocalRootSigMaterialWithTex.size()), L"ShaderTable", RayTracingEntryPoints::RayGen, RayTracingEntryPoints::Miss);
    CreateShaderTable(mShaderTablePhoton, mRTPSOPhoton, mDispatchPhotonRayDesc, 0, 0, max(mRegisterMapGlobalLocalRootSigMaterial.size(), mRegisterMapGlobalLocalRootSigMaterialWithTex.size()), L"ShaderTablePhoton", RayTracingEntryPoints::RayGenPhoton, RayTracingEntryPoints::MissPhoton);
    CreateShaderTable(mShaderTableReservoirSpatialReuse, mRTPSOReservoirSpatialReuse, mDispatchReservoirSpatialReuseRayDesc, 0, 0, max(mRegisterMapGlobalLocalRootSigMaterial.size(), mRegisterMapGlobalLocalRootSigMaterialWithTex.size()), L"ShaderTableReservoirSpatialReuse", RayTracingEntryPoints::RayGenSpatialReuse, RayTracingEntryPoints::Miss);
    CreateShaderTable(mShaderTableReservoirTemporalReuse, mRTPSOReservoirTemporalReuse, mDispatchReservoirTemporalReuseRayDesc, 0, 0, max(mRegisterMapGlobalLocalRootSigMaterial.size(), mRegisterMapGlobalLocalRootSigMaterialWithTex.size()), L"ShaderTableReservoirTemporalReuse", RayTracingEntryPoints::RayGenTemporalReuse, RayTracingEntryPoints::Miss);
}