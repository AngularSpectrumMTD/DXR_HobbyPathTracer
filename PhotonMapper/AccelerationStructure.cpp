#include "DxrPhotonMapper.h"
#include <random>

using namespace DirectX;

#define STAGE_DIVISION 4

void DxrPhotonMapper::SetupMeshInfo(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs)
{
    D3D12_RAYTRACING_INSTANCE_DESC templateDesc{};
    templateDesc.InstanceMask = 0xFF;
    templateDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

    enum InstanceType
    {
        Reflect = 0,
        Refract = 1,
        Default = 2
    };

    auto entryOffset = 0; // use other shader table
    //NOTE: InstanceContributionToHitGroupIndex is HitGroup ID(decided in CreateShaderTable())
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), XMMatrixIdentity());
        desc.InstanceID = InstanceType::Default;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshStage.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mSpheresNormalTbl) {
        entryOffset++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = InstanceType::Default;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mBoxesNormalTbl) {
        entryOffset++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = InstanceType::Default;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshBox.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mOBJ0sNormalTbl)
    {
        entryOffset++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = InstanceType::Refract;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshOBJ0.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mOBJ1sNormalTbl)
    {
        entryOffset++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = InstanceType::Reflect;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshOBJ1.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& instances : mOBJModel.getMaterialList())
    {
        entryOffset++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), mOBJModelTRS);
        desc.InstanceID = InstanceType::Default;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = instances.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

   /* for (const auto& instances : mLightTbl)
    {
        entryOffset++;
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), instances);
        desc.InstanceID = InstanceType::Refract;
        desc.InstanceMask = 0x08;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshLightSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }*/
}

void DxrPhotonMapper::SetupMeshMaterialAndPos()
{
    std::vector<utility::VertexPNT> verticesPNT;
    std::vector<u32> indices;

    if (mStageType == StageType_Box)
    {
        utility::CreateOpenedCube(verticesPNT, indices, PLANE_SIZE * 0.99f);
    }
    else
    {
        utility::CreatePlane(verticesPNT, indices, PLANE_SIZE * 0.99f);
        for (auto& v : verticesPNT)
        {
            v.Position.y = -PLANE_SIZE * 0.99f;
        }
    }

    mMeshStage.CreateMeshBuffer(mDevice, verticesPNT, indices, L"PlaneVB", L"PlaneIB", L"");

    const f32 sphereRadius = 5;
    const f32 boxYLength = 5;
    const f32 boxXLength = 3;
    const f32 boxZLength = 3;

    std::vector<utility::VertexPN> verticesPN;
    utility::CreateSphere(verticesPN, indices, sphereRadius, 100, 100);
    mMeshSphere.CreateMeshBuffer(mDevice, verticesPN, indices, L"SphereVB", L"SphereIB", L"");

    std::string strValue;
    strValue.assign(mOBJ0FileName.begin(), mOBJ0FileName.end());
    const char* charValue = strValue.c_str();
    bool isLoaded = utility::CreateMesh(charValue, verticesPN, indices, mGlassObjScale);
    if (!isLoaded)
    {
        OutputDebugString(L"OBJ Load was Failed\n");
        throw std::runtime_error("OBJ Load was Failed");
    }
    mMeshOBJ0.CreateMeshBuffer(mDevice, verticesPN, indices, L"GlassVB", L"GlassIB", L"");

    strValue.assign(mOBJ1FileName.begin(), mOBJ1FileName.end());
    charValue = strValue.c_str();
    isLoaded = utility::CreateMesh(charValue, verticesPN, indices, mMetalObjScale);
    if (!isLoaded)
    {
        OutputDebugString(L"OBJ Load was Failed\n");
        throw std::runtime_error("OBJ Load was Failed");
    }
    mMeshOBJ1.CreateMeshBuffer(mDevice, verticesPN, indices, L"MetalVB", L"MetalIB", L"");

    /*utility::CreateSphere(verticesPN, indices, 0.1f, 30, 30);
    mMeshLightSphere.CreateMeshBuffer(mDevice, verticesPN, indices, L"SphereLightVB", L"SphereLightIB", L"");*/

    utility::CreateCube(verticesPN, indices, boxXLength, boxYLength, boxZLength);
    mMeshBox.CreateMeshBuffer(mDevice, verticesPN, indices, L"BoxVB", L"BoxIB", L"");
    
    if (!mOBJModel.OBJ_Load(mDevice, mOBJFolderName.c_str(), mOBJFileName.c_str(), L""))
    {
        OutputDebugString(L"OBJ Load was Failed\n");
        throw std::runtime_error("OBJ Load was Failed");
    }
    mOBJModel.CreateMeshBuffers(mDevice, L"");

    s32 count = 0;
    std::mt19937 mt;
    const f32 cellSize = 2 * 0.9 * PLANE_SIZE / STAGE_DIVISION;

    for (auto& trs : mOBJ0sNormalTbl)
    {
        f32 y = mGlassObjYOfsset;

        f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION);
        f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION);

        if (count == 0 && NormalOBJ0s == 1)
        {
            trs = XMMatrixTranslation(0, y, 0);
        }
        else
        {
            trs = XMMatrixTranslation(x, y, z);
        }
        count++;
    }
    count = 0;
    for (auto& trs : mOBJ1sNormalTbl)
    {
        f32 y = mMetalObjYOfsset;

        f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION) - PLANE_SIZE;
        f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION) - PLANE_SIZE;

        if (count == 0 && NormalOBJ1s == 1)
        {
            //trs = XMMatrixTranslation(-PLANE_SIZE * 0.02f, y, -PLANE_SIZE * 0.02f);
            trs = XMMatrixTranslation(0, y, 0);
        }
        else
        {
            trs = XMMatrixTranslation(x, y, z);
        }
        count++;
    }

    for (auto& trs : mLightTbl)
    {
        trs = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
    }

    std::uniform_real_distribution<> rndScale(1, 3);
    for (auto& trs : mSpheresNormalTbl) {
        f32 scale = (f32)rndScale(mt);
        const f32 scaledSize = sphereRadius * scale;
        f32 y = -PLANE_SIZE * 0.99f + scaledSize;
        f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION) - PLANE_SIZE;
        f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION) - PLANE_SIZE;
        trs = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixTranslation(x, y, z));
        count++;
    }
    for (auto& trs : mBoxesNormalTbl) {
        f32 scale = (f32)rndScale(mt) ;
        const f32 scaledSizeY = boxYLength * scale;
        const f32 scaledSizeX = boxXLength * scale;
        const f32 scaledSizeZ = boxZLength * scale;
        f32 y = -PLANE_SIZE * 0.98f + scaledSizeY;
        f32 x = cellSize * 0.5 + cellSize * (count / STAGE_DIVISION) - PLANE_SIZE;
        f32 z = cellSize * 0.5 + cellSize * (count % STAGE_DIVISION) - PLANE_SIZE;
        trs = XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixTranslation(x, y, z));
        count++;
    }

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
    defaultMaterial.albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    defaultMaterial.metallic = 0;//blend diffuse specular at specTrans == 0
    defaultMaterial.roughness = 0;
    defaultMaterial.specular = 0;//spec power
    defaultMaterial.transRatio = 1;//0:diffuse  1:trans
    defaultMaterial.transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    defaultMaterial.emission = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

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
    for (auto& material : mOBJ1MaterialTbl) {
        material = defaultMaterial;
        //material.albedo = (NormalOBJ1s == 1) ? colorTbl[0] : colorTbl[albedoIndex % _countof(colorTbl)];
        material.albedo = (NormalOBJ1s == 1) ? XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f) : colorTbl[albedoIndex % _countof(colorTbl)];//test
        
        //material.transColor = (NormalOBJ1s == 1) ? colorTbl[0] : colorTbl[transIndex % _countof(colorTbl)];
        material.transColor = (NormalOBJ1s == 1) ? XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f) : colorTbl[transIndex % _countof(colorTbl)];//test

        material.metallic = 0;// rndF(mt);
        material.roughness = 0.3;// rndF(mt);
        material.transRatio = 1;// rndF(mt);

        albedoIndex++;
        transIndex++;

        mMaterialParam1 = material;
    }
    for (auto& material : mOBJ0MaterialTbl) {
        material = defaultMaterial;
        material.albedo = (NormalOBJ0s == 1) ? colorTbl[2] : colorTbl[albedoIndex % _countof(colorTbl)];
        //material.metallic = rndF(mt);
        material.metallic = 0.3;
        //material.roughness = 0.1;// rndF(mt);
        //material.roughness = 0.0;// rndF(mt);
        material.roughness = 0.05;// rndF(mt);
        material.transColor = (NormalOBJ0s == 1) ? colorTbl[2] : colorTbl[transIndex % _countof(colorTbl)];
        //material.transColor = (NormalOBJ0s == 1) ? XMVectorSet(1.0f, 0.8f, 1.0f, 0.0f) : colorTbl[transIndex % _countof(colorTbl)];
        
        if (mGlassModelType == ModelType_Dragon)
        {
            material.roughness = 0.3;
            material.transColor = (NormalOBJ0s == 1) ? XMVectorSet(0.5f, 1.0f, 1.0f, 0.0f) : colorTbl[transIndex % _countof(colorTbl)];
            material.albedo = (NormalOBJ0s == 1) ? XMVectorSet(0.5f, 1.0f, 1.0f, 0.0f) : colorTbl[albedoIndex % _countof(colorTbl)];
        }
        else if (mGlassModelType == ModelType_CurvedMesh)
        {
            material.transColor = (NormalOBJ0s == 1) ? XMVectorSet(0.1f, 1.0f, 0.4f, 0.0f) : colorTbl[transIndex % _countof(colorTbl)];
            material.albedo = (NormalOBJ0s == 1) ? XMVectorSet(0.1f, 1.0f, 0.4f, 0.0f) : colorTbl[albedoIndex % _countof(colorTbl)];
        }
        else if(mSceneType == SceneType_Sponza)
        {
            material.transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
            material.albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        }

        //material.transRatio = 0.6;// rndF(mt);
        //material.transRatio = 1;// rndF(mt);
        material.transRatio = 1;// rndF(mt);
        if (mSceneType == SceneType_Simple)
        {
            material.transRatio = 0;
        }
        albedoIndex++;
        transIndex++;
        //material.emission = XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
        mMaterialParam0 = material;
    }

    mStageMaterial.albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    mStageMaterial.metallic = 0.9;
    mStageMaterial.roughness = 0.3;
    mStageMaterial.specular = 1;
    mStageMaterial.transRatio = 0;
    mStageMaterial.transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    mStageMaterial.emission = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

    auto bufferSize = sizeof(utility::MaterialParam) * mNormalSphereMaterialTbl.size();
    mNormalSphereMaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mNormalSphereMaterialCB.Get(), mNormalSphereMaterialTbl.data(), bufferSize);

    bufferSize = sizeof(utility::MaterialParam) * mNormalBoxMaterialTbl.size();
    mNormalBoxMaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mNormalBoxMaterialCB.Get(), mNormalBoxMaterialTbl.data(), bufferSize);

    bufferSize = sizeof(utility::MaterialParam) * mOBJ1MaterialTbl.size();
    mOBJ1MaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mOBJ1MaterialCB.Get(), mOBJ1MaterialTbl.data(), bufferSize);

    bufferSize = sizeof(utility::MaterialParam) * mOBJ0MaterialTbl.size();
    mOBJ0MaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mOBJ0MaterialCB.Get(), mOBJ0MaterialTbl.data(), bufferSize);

    bufferSize = sizeof(utility::MaterialParam);
    mStageMaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mStageMaterialCB.Get(), &mStageMaterial, bufferSize);
}

void DxrPhotonMapper::CreateSceneBLAS()
{
    mMeshStage.CreateBLAS(mDevice, L"Plane-BLAS");
    mMeshSphere.CreateBLAS(mDevice, L"Sphere-BLAS");
    mMeshOBJ0.CreateBLAS(mDevice, L"Glass-BLAS");
    mMeshOBJ1.CreateBLAS(mDevice, L"Metal-BLAS");
    //mMeshLightSphere.CreateBLAS(mDevice, L"LightSphere-BLAS");
    mMeshBox.CreateBLAS(mDevice, L"Box-BLAS");
    mOBJModel.CreateBLASs(mDevice);
}

void DxrPhotonMapper::CreateSceneTLAS()
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    SetupMeshInfo(instanceDescs);

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
    sceneASB.ASBuffer->SetName(L"Scene-Tlas");

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

void DxrPhotonMapper::UpdateSceneTLAS()
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    SetupMeshInfo(instanceDescs);

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

void DxrPhotonMapper::CreateAccelerationStructure()
{
    SetupMeshMaterialAndPos();
    CreateSceneBLAS();
    CreateSceneTLAS();
}