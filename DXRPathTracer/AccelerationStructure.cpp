#include "DXRPathTracer.h"
#include <random>

using namespace DirectX;

void DXRPathTracer::UpdateMeshInstances(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs)
{
    D3D12_RAYTRACING_INSTANCE_DESC templateDesc{};
    templateDesc.InstanceMask = 0xFF;
    templateDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

    auto hitGroupID = 0;
    //NOTE: InstanceContributionToHitGroupIndex is HitGroup ID(decided in CreateShaderTable())
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), XMMatrixIdentity());
        desc.InstanceID =0;//unused
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = hitGroupID;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshStage.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mSpheresNormalTbl) {
        hitGroupID++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = 0;//unused
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = hitGroupID;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mBoxesNormalTbl) {
        hitGroupID++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = 0;//unused
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = hitGroupID;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshBox.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    u32 i = 0;
    for (const auto& instances : mOBJNormalTbl)
    {
        hitGroupID++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = 0;//unused
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = hitGroupID;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshOBJTbl[i].blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
        i++;
    }

    for (const auto& instances : mOBJMaterialLinkedMesh.getMaterialList())
    {
        hitGroupID++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), mOBJMaterialLinkedMeshTRS);
        desc.InstanceID = 0;//unused
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = hitGroupID;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = instances.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }
}

void DXRPathTracer::CreateSceneInfo()
{
    s32 count = 0;
    std::mt19937 mt;
    const f32 cellSize = 2 * 0.9 * PLANE_SIZE / STAGE_DIVISION;

    //Scaling and Translation
    {
        u32 i = 0;
        for (auto& trs : mOBJNormalTbl)
        {
            f32 y = mObjYOffsetTbl[i];

            f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION);
            f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION);

            if ((count == 0) || (count == 1))
            {
                trs = XMMatrixTranslation(0, y, 0);
            }
            else
            {
                trs = XMMatrixTranslation(x, y, z);
            }
            count++;
            i++;
        }
        count = 0;

        for (auto& trs : mLightTbl)
        {
            trs = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
        }

        count = 0;

        std::uniform_real_distribution<> rndScale(1, 3);
        for (auto& trs : mSpheresNormalTbl) {
            f32 scale = (f32)rndScale(mt);
            const f32 scaledSize = SPHERE_RADIUS * scale;
            f32 y = -PLANE_SIZE * 0.99f + scaledSize;
            f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION) - PLANE_SIZE;
            f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION) - PLANE_SIZE;
            trs = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixTranslation(x, y, z));
            count++;
        }
        for (auto& trs : mBoxesNormalTbl) {
            f32 scale = (f32)rndScale(mt);
            const f32 scaledSizeX = BOX_X_LENGTH * scale;
            const f32 scaledSizeY = BOX_Y_LENGTH * scale;
            const f32 scaledSizeZ = BOX_Z_LENGTH * scale;
            f32 y = -PLANE_SIZE * 0.98f + scaledSizeY;
            f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION) - PLANE_SIZE;
            f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION) - PLANE_SIZE;
            trs = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixTranslation(x, y, z));
            count++;
        }
    }

    //Assign Material
    {
        XMVECTOR colorTbl[] = {
            XMVectorSet(1.0f, 0.2f, 1.0f, 0.0f),
            XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f),
            XMVectorSet(0.8f, 0.6f, 0.1f, 0.0f),
            XMVectorSet(0.2f, 0.8f, 0.7f, 0.0f),
            XMVectorSet(0.1f, 0.8f, 0.4f, 0.0f),
            XMVectorSet(0.3f, 0.2f, 0.8f, 0.0f),
            XMVectorSet(0.8f, 0.4f, 0.4f, 0.0f),
            XMVectorSet(0.2f, 0.8f, 0.8f, 0.0f),
            XMVectorSet(0.5f, 0.5f, 0.4f, 0.0f),
        };

        utility::MaterialParam defaultMaterial{};
        defaultMaterial.asDefault();

        utility::MaterialParam defaultSSSMaterial{};
        defaultSSSMaterial.asDefaultSSS();

        std::uniform_int_distribution rndID(0, 4);
        std::uniform_real_distribution rndF(0.f, 1.f);
        u32 albedoIndex = rndID(mt);
        u32 transIndex = rndID(mt);
        for (auto& material : mNormalSphereMaterialTbl) {
            material = defaultMaterial;
            material.albedo = colorTbl[albedoIndex % _countof(colorTbl)];
            material.metallic = rndF(mt);
            material.roughness = rndF(mt);
            material.transColor = colorTbl[transIndex % _countof(colorTbl)];
            material.transRatio = rndF(mt);
            albedoIndex++;
            transIndex++;
        }
        for (auto& material : mNormalBoxMaterialTbl) {
            material = defaultMaterial;
            material.albedo = colorTbl[albedoIndex % _countof(colorTbl)];
            material.metallic = rndF(mt);
            material.roughness = rndF(mt);
            material.transColor = colorTbl[transIndex % _countof(colorTbl)];
            material.transRatio = rndF(mt);
            albedoIndex++;
            transIndex++;
        }

        u32 i = 0;
        for (auto& material : mOBJMaterialTbl) {
            material = defaultMaterial;
            material.albedo = colorTbl[albedoIndex % _countof(colorTbl)];
            //material.metallic = rndF(mt);
            material.metallic = 0.3;
            //material.roughness = 0.1;// rndF(mt);
            //material.roughness = 0.0;// rndF(mt);
            material.roughness = 0.2;// rndF(mt);
            material.transColor = colorTbl[transIndex % _countof(colorTbl)];
            //material.transColor = (NormalOBJ0s == 1) ? XMVectorSet(1.0f, 0.8f, 1.0f, 0.0f) : colorTbl[transIndex % _countof(colorTbl)];
            if (mModelTypeTbl[0] == ModelType_CurvedMesh)
            {
                material.transColor =colorTbl[transIndex % _countof(colorTbl)];
                material.albedo =  colorTbl[albedoIndex % _countof(colorTbl)];
            }

            material.transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
            material.albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);

            material.transRatio = 0.0;

            if (i == 1)
            {
                material.transRatio = 0;
            }

            if (mSceneType == SceneType_Simple)
            {
                material.transRatio = 0;
            }

            if (mSceneType == SceneType_BistroInterior)
            {
                material.transRatio = 0;
            }

            if (material.transRatio > 0)
            {
                material.roughness = max(0.05, material.roughness * 0.1);
            }

#ifdef GI_TEST
            material.transRatio = 0;// rndF(mt);
#endif

            //sss test
#ifdef USE_SSS
            material = defaultSSSMaterial;
#endif

            albedoIndex++;
            transIndex++;
            //material.emission = XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
            mMaterialParamTbl[i] = material;

            i++;
        }

        mStageMaterial.albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        mStageMaterial.metallic = 0.1;
        mStageMaterial.roughness = 0.9;
        mStageMaterial.specular = 1;
        mStageMaterial.transRatio = 0;
        mStageMaterial.transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        mStageMaterial.emission = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        mStageMaterial.isSSSExecutable = 0u;
    }

    //upload the data of material
    {
        auto bufferSize = sizeof(utility::MaterialParam) * mNormalSphereMaterialTbl.size();
        mDevice->ImmediateBufferUpdateHostVisible(mNormalSphereMaterialCB.Get(), mNormalSphereMaterialTbl.data(), bufferSize);

        bufferSize = sizeof(utility::MaterialParam) * mNormalBoxMaterialTbl.size();
        mDevice->ImmediateBufferUpdateHostVisible(mNormalBoxMaterialCB.Get(), mNormalBoxMaterialTbl.data(), bufferSize);

        bufferSize = sizeof(utility::MaterialParam) * mOBJMaterialTbl.size();
        mDevice->ImmediateBufferUpdateHostVisible(mOBJMaterialCB.Get(), mOBJMaterialTbl.data(), bufferSize);

        bufferSize = sizeof(utility::MaterialParam);
        mDevice->ImmediateBufferUpdateHostVisible(mStageMaterialCB.Get(), &mStageMaterial, bufferSize);
    }
}

void DXRPathTracer::CreateSceneBLAS()
{
    mMeshStage.CreateBLAS(mDevice, L"Plane-BLAS");
    mMeshSphere.CreateBLAS(mDevice, L"Sphere-BLAS");
    mMeshOBJTbl[0].CreateBLAS(mDevice, L"Glass-BLAS");
    mMeshOBJTbl[1].CreateBLAS(mDevice, L"Metal-BLAS");
    //mMeshLightSphere.CreateBLAS(mDevice, L"LightSphere-BLAS");
    mMeshBox.CreateBLAS(mDevice, L"Box-BLAS");
    mOBJMaterialLinkedMesh.CreateBLAS(mDevice);
}

void DXRPathTracer::CreateSceneTLAS()
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    UpdateMeshInstances(instanceDescs);

    size_t sizeOfInstanceDescs = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

    mInstanceDescsBuffer = mDevice->CreateBuffer(sizeOfInstanceDescs, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, nullptr, L"InstanceDescsBuffer");
    mDevice->ImmediateBufferUpdateHostVisible(mInstanceDescsBuffer.Get(), instanceDescs.data(), sizeOfInstanceDescs);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
    auto& inputs = asDesc.Inputs;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;//FOR UPDATE
    inputs.NumDescs = u32(instanceDescs.size());
    inputs.InstanceDescs = mInstanceDescsBuffer.Get()->GetGPUVirtualAddress();

    auto sceneASB = mDevice->CreateAccelerationStructure(asDesc);
    mTLAS = sceneASB.ASBuffer;
    mTLASUpdate = sceneASB.updateBuffer;
    sceneASB.ASBuffer->SetName(L"TLAS");

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = mTLAS->GetGPUVirtualAddress();
    mTLASDescriptor = mDevice->CreateShaderResourceView(nullptr, &srvDesc);

    asDesc.ScratchAccelerationStructureData = sceneASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = mTLAS->GetGPUVirtualAddress();

    auto command = mDevice->CreateCommandList();
    command->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(
        mTLAS.Get()
    );
    command->ResourceBarrier(1, &barrier);
    command->Close();
    mDevice->ExecuteCommandList(command);
    // wait, cuz scratchBuffers are destructed after exit this function
    mDevice->WaitForCompletePipe();
}

void DXRPathTracer::UpdateSceneTLAS()
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    UpdateMeshInstances(instanceDescs);

    size_t sizeOfInstanceDescs = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    mDevice->ImmediateBufferUpdateHostVisible(mInstanceDescsBuffer.Get(), instanceDescs.data(), sizeOfInstanceDescs);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
    auto& inputs = asDesc.Inputs;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = u32(instanceDescs.size());
    inputs.InstanceDescs = mInstanceDescsBuffer.Get()->GetGPUVirtualAddress();

    // set flags for TLAS update
    inputs.Flags =
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

    asDesc.SourceAccelerationStructureData = mTLAS->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = mTLAS->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData = mTLASUpdate->GetGPUVirtualAddress();

    mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(mTLAS.Get());
    mCommandList->ResourceBarrier(1, &barrier);
}

void DXRPathTracer::CreateAccelerationStructure()
{
    CreateSceneInfo();
    CreateSceneBLAS();
    CreateSceneTLAS();
}