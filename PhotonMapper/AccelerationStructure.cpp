#include "DxrPhotonMapper.h"
#include <random>

using namespace DirectX;

void DxrPhotonMapper::SetupMeshInfo(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs)
{
    D3D12_RAYTRACING_INSTANCE_DESC templateDesc{};
    templateDesc.InstanceMask = 0xFF;
    templateDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

    enum InstanceType
    {
        Reflect = 0,
        Refract = 1,
        Phong = 2
    };

    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), XMMatrixIdentity());
        desc.InstanceID = InstanceType::Phong;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 0;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshStage.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& spherePos : mSpheresReflectTbl) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), spherePos);
        desc.InstanceID = InstanceType::Reflect;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 1;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& spherePos : mSpheresRefractTbl) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), spherePos);
        desc.InstanceID = InstanceType::Refract;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 1;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& spherePos : mBoxesReflectTbl) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), spherePos);
        desc.InstanceID = InstanceType::Reflect;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 2;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshBox.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    for (const auto& spherePos : mBoxesRefractTbl) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), spherePos);
        desc.InstanceID = InstanceType::Refract;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 2;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshBox.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }

    auto entryOffset = 3; // use other shader table
    for (const auto& spherePos : mSpheresNormalTbl) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), spherePos);
        desc.InstanceID = InstanceType::Phong;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
        entryOffset++;
    }

    for (const auto& spherePos : mBoxesNormalTbl) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), spherePos);
        desc.InstanceID = InstanceType::Phong;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshBox.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
        entryOffset++;
    }

    for (const auto& glassPos : mGlasssNormalTbl)
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), glassPos);
        desc.InstanceID = InstanceType::Refract;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;// use other shader table
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshGlass.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
        entryOffset++;
    }

    for (const auto& metalPos : mMetalsNormalTbl)
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), metalPos);
        desc.InstanceID = InstanceType::Reflect;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = entryOffset;// use other shader table
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshMetal.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
        entryOffset++;
    }

    for (const auto& lightPos : mLightTbl)
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        XMStoreFloat3x4(
            reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), lightPos);
        desc.InstanceID = InstanceType::Refract;
        desc.InstanceMask = 0x08;
        desc.InstanceContributionToHitGroupIndex = entryOffset;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = mMeshLightSphere.blas->GetGPUVirtualAddress();
        instanceDescs.push_back(desc);
    }
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
            v.Position.y = -PLANE_SIZE * 0.99f * 0.5;
        }
    }

    mMeshStage.CreateMeshBuffer(mDevice, verticesPNT, indices, L"PlaneVB", L"PlaneIB", L"");

    const f32 sphereRadius = 5;
    const f32 boxYLength = 2;

    std::vector<utility::VertexPN> verticesPN;
    utility::CreateSphere(verticesPN, indices, sphereRadius, 30, 30);
    mMeshSphere.CreateMeshBuffer(mDevice, verticesPN, indices, L"SphereVB", L"SphereIB", L"");

    std::string strValue;
    strValue.assign(mGlassFileName.begin(), mGlassFileName.end());
    const char* charValue = strValue.c_str();
    bool isLoaded = utility::CreateMesh(charValue, verticesPN, indices, mGlassObjScale);
    mMeshGlass.CreateMeshBuffer(mDevice, verticesPN, indices, L"GlassVB", L"GlassIB", L"");

    strValue.assign(mMetalFileName.begin(), mMetalFileName.end());
    charValue = strValue.c_str();
    isLoaded = utility::CreateMesh(charValue, verticesPN, indices, mMetalObjScale);
    mMeshMetal.CreateMeshBuffer(mDevice, verticesPN, indices, L"MetalVB", L"MetalIB", L"");

    utility::CreateSphere(verticesPN, indices, 1.f, 30, 30);
    mMeshLightSphere.CreateMeshBuffer(mDevice, verticesPN, indices, L"SphereLightVB", L"SphereLightIB", L"");

    utility::CreateCube(verticesPN, indices, 7, boxYLength, 7);
    mMeshBox.CreateMeshBuffer(mDevice, verticesPN, indices, L"BoxVB", L"BoxIB", L"");

    std::mt19937 mt;
    s32 splatRange = 30;

    std::uniform_int_distribution rnd(-splatRange, splatRange);

    const f32 sphereY = -PLANE_SIZE * 0.99f * (mStageType == StageType_Box ? 1: 0.5) + sphereRadius;

    for (auto& type : mSpheresRefractTbl) {
        f32 y = sphereY;
        f32 x = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd(mt));
        f32 z = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd(mt));

        type = XMMatrixTranslation(x, y, z);
    }
    for (auto& type : mSpheresReflectTbl) {
        f32 y = sphereY;
        f32 x = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd(mt));
        f32 z = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd(mt));

        type = XMMatrixTranslation(x, y, z);
    }
    for (auto& type : mSpheresNormalTbl) {
        f32 y = sphereY;
        f32 x = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd(mt));
        f32 z = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd(mt));

        type = XMMatrixTranslation(x, y, z);
    }

    splatRange = 200;
    std::uniform_int_distribution rnd2(-splatRange, splatRange);
    std::uniform_int_distribution rnd3(-2, 2);
    s32 dcount = 0;
    for (auto& pos : mGlasssNormalTbl)
    {
        f32 y = mGlassObjYOfsset;

        f32 x = 0.01f * rnd2(mt) + 5 * rnd3(mt);
        f32 z = 0.01f * rnd2(mt) + 5 * rnd3(mt);

        if (dcount == 0 && NormalGlasses == 1)
        {
            pos = XMMatrixTranslation(0, y, 0);
        }
        else
        {
            pos = XMMatrixTranslation(x, y, z);
        }
        dcount++;
    }
    dcount = 0;
    for (auto& pos : mMetalsNormalTbl)
    {
        f32 y = mMetalObjYOfsset;

        f32 x = 0.01f * rnd2(mt) + 5 * rnd3(mt);
        f32 z = 0.01f * rnd2(mt) + 5 * rnd3(mt);

        if (dcount == 0 && NormalMetals == 1)
        {
            pos = XMMatrixTranslation(-PLANE_SIZE * 0.5f, y, -PLANE_SIZE * 0.5f);
        }
        else
        {
            pos = XMMatrixTranslation(x, y, z);
        }
        dcount++;
    }

    for (auto& pos : mLightTbl)
    {
        pos = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
    }

    const f32 BoxY = -PLANE_SIZE * 0.98f * (mStageType == StageType_Box ? 1 : 0.5) + boxYLength;
    std::uniform_int_distribution rnd4(-20, -5);
    std::uniform_int_distribution rnd5(5, 30);
    std::uniform_int_distribution rndz(0, 30);

    for (auto& type : mBoxesRefractTbl) {
        f32 y = BoxY;
        f32 x = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd4(mt));
        f32 z = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rndz(mt) + 15);

        type = XMMatrixTranslation(x, y, z);
    }
    for (auto& type : mBoxesReflectTbl) {
        f32 y = BoxY;
        f32 x = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd5(mt));
        f32 z = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rndz(mt) + 5);

        type = XMMatrixTranslation(x, y, z);
    }
    for (auto& type : mBoxesNormalTbl) {
        f32 y = BoxY;
        f32 x = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rnd4(mt));
        f32 z = Clamp(-PLANE_SIZE * 0.99f + sphereRadius, PLANE_SIZE * 0.99f - sphereRadius, (f32)rndz(mt) + 2);

        type = XMMatrixTranslation(x, y, z);
    }

    MaterialParam defaultMaterial{};
    defaultMaterial.albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    defaultMaterial.specular = XMVectorSet(1.0f, 1.0f, 1.0f, 40.0f);

    XMVECTOR colorTbl[] = {
        XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f),
        XMVectorSet(0.1f, 0.8f, 0.4f, 0.0f),
        XMVectorSet(0.2f, 0.8f, 0.7f, 0.0f),
        XMVectorSet(0.8f, 0.6f, 0.1f, 0.0f),
        XMVectorSet(0.3f, 0.2f, 0.8f, 0.0f),
        XMVectorSet(0.8f, 0.4f, 0.4f, 0.0f),
        XMVectorSet(1.0f, 0.2f, 1.0f, 0.0f),
        XMVectorSet(0.1f, 0.2f, 0.8f, 0.0f),
        XMVectorSet(0.5f, 0.5f, 0.4f, 0.0f),
    };

    u32 index = 0;
    for (auto& material : mNormalSphereMaterialTbl) {
        material = defaultMaterial;
        material.albedo = colorTbl[index % _countof(colorTbl)];
        index++;
    }
    index = 0;
    for (auto& material : mNormalBoxMaterialTbl) {
        material = defaultMaterial;
        material.albedo = colorTbl[(index + 5) % _countof(colorTbl)];
        index++;
    }

    auto bufferSize = sizeof(MaterialParam) * mNormalSphereMaterialTbl.size();
    mNormalSphereMaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mNormalSphereMaterialCB.Get(), mNormalSphereMaterialTbl.data(), bufferSize);

    bufferSize = sizeof(MaterialParam) * mNormalBoxMaterialTbl.size();
    mNormalBoxMaterialCB = mDevice->CreateConstantBuffer(bufferSize);
    mDevice->ImmediateBufferUpdateHostVisible(mNormalBoxMaterialCB.Get(), mNormalBoxMaterialTbl.data(), bufferSize);
}

void DxrPhotonMapper::CreateSceneBLAS()
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = asDesc.Inputs;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    auto command = mDevice->CreateCommandList();

    D3D12_RAYTRACING_GEOMETRY_DESC planeGeomDesc{};
    planeGeomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    planeGeomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    {
        auto& triangles = planeGeomDesc.Triangles;
        triangles.VertexBuffer.StartAddress = mMeshStage.vertexBuffer->GetGPUVirtualAddress();
        triangles.VertexBuffer.StrideInBytes = mMeshStage.vertexStride;
        triangles.VertexCount = mMeshStage.vertexCount;
        triangles.IndexBuffer = mMeshStage.indexBuffer->GetGPUVirtualAddress();
        triangles.IndexCount = mMeshStage.indexCount;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    }

    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &planeGeomDesc;
    auto planeASB = mDevice->CreateAccelerationStructure(asDesc);
    planeASB.ASBuffer->SetName(L"Plane-BLAS");
    asDesc.ScratchAccelerationStructureData = planeASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = planeASB.ASBuffer->GetGPUVirtualAddress();
    command->BuildRaytracingAccelerationStructure(
        &asDesc, 0, nullptr);

    std::vector<dx12::AccelerationStructureBuffers> asbuffers;
    D3D12_RAYTRACING_GEOMETRY_DESC sphereGeomDesc{};
    sphereGeomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    sphereGeomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    {
        auto& triangles = sphereGeomDesc.Triangles;
        triangles.VertexBuffer.StartAddress = mMeshSphere.vertexBuffer->GetGPUVirtualAddress();
        triangles.VertexBuffer.StrideInBytes = mMeshSphere.vertexStride;
        triangles.VertexCount = mMeshSphere.vertexCount;
        triangles.IndexBuffer = mMeshSphere.indexBuffer->GetGPUVirtualAddress();
        triangles.IndexCount = mMeshSphere.indexCount;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    }

    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &sphereGeomDesc;
    auto sphereASB = mDevice->CreateAccelerationStructure(asDesc);
    sphereASB.ASBuffer->SetName(L"Sphere-BLAS");
    asDesc.ScratchAccelerationStructureData = sphereASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = sphereASB.ASBuffer->GetGPUVirtualAddress();

    command->BuildRaytracingAccelerationStructure(
        &asDesc, 0, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC glassGeomDesc{};
    glassGeomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    glassGeomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    {
        auto& triangles = glassGeomDesc.Triangles;
        triangles.VertexBuffer.StartAddress = mMeshGlass.vertexBuffer->GetGPUVirtualAddress();
        triangles.VertexBuffer.StrideInBytes = mMeshGlass.vertexStride;
        triangles.VertexCount = mMeshGlass.vertexCount;
        triangles.IndexBuffer = mMeshGlass.indexBuffer->GetGPUVirtualAddress();
        triangles.IndexCount = mMeshGlass.indexCount;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    }

    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &glassGeomDesc;
    auto glassASB = mDevice->CreateAccelerationStructure(asDesc);
    glassASB.ASBuffer->SetName(L"Glass-BLAS");
    asDesc.ScratchAccelerationStructureData = glassASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = glassASB.ASBuffer->GetGPUVirtualAddress();

    command->BuildRaytracingAccelerationStructure(
        &asDesc, 0, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC metalGeomDesc{};
    metalGeomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    metalGeomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    {
        auto& triangles = metalGeomDesc.Triangles;
        triangles.VertexBuffer.StartAddress = mMeshMetal.vertexBuffer->GetGPUVirtualAddress();
        triangles.VertexBuffer.StrideInBytes = mMeshMetal.vertexStride;
        triangles.VertexCount = mMeshMetal.vertexCount;
        triangles.IndexBuffer = mMeshMetal.indexBuffer->GetGPUVirtualAddress();
        triangles.IndexCount = mMeshMetal.indexCount;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    }

    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &metalGeomDesc;
    auto metalASB = mDevice->CreateAccelerationStructure(asDesc);
    metalASB.ASBuffer->SetName(L"Metal-BLAS");
    asDesc.ScratchAccelerationStructureData = metalASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = metalASB.ASBuffer->GetGPUVirtualAddress();

    command->BuildRaytracingAccelerationStructure(
        &asDesc, 0, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC lightSphereGeomDesc{};
    lightSphereGeomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    lightSphereGeomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    {
        auto& triangles = lightSphereGeomDesc.Triangles;
        triangles.VertexBuffer.StartAddress = mMeshLightSphere.vertexBuffer->GetGPUVirtualAddress();
        triangles.VertexBuffer.StrideInBytes = mMeshLightSphere.vertexStride;
        triangles.VertexCount = mMeshLightSphere.vertexCount;
        triangles.IndexBuffer = mMeshLightSphere.indexBuffer->GetGPUVirtualAddress();
        triangles.IndexCount = mMeshLightSphere.indexCount;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    }

    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &lightSphereGeomDesc;
    auto lightSphereASB = mDevice->CreateAccelerationStructure(asDesc);
    lightSphereASB.ASBuffer->SetName(L"LightSphere-BLAS");
    asDesc.ScratchAccelerationStructureData = lightSphereASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = lightSphereASB.ASBuffer->GetGPUVirtualAddress();

    command->BuildRaytracingAccelerationStructure(
        &asDesc, 0, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC boxGeomDesc{};
    boxGeomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    boxGeomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    {
        auto& triangles = boxGeomDesc.Triangles;
        triangles.VertexBuffer.StartAddress = mMeshBox.vertexBuffer->GetGPUVirtualAddress();
        triangles.VertexBuffer.StrideInBytes = mMeshBox.vertexStride;
        triangles.VertexCount = mMeshBox.vertexCount;
        triangles.IndexBuffer = mMeshBox.indexBuffer->GetGPUVirtualAddress();
        triangles.IndexCount = mMeshBox.indexCount;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    }

    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &boxGeomDesc;
    auto boxASB = mDevice->CreateAccelerationStructure(asDesc);
    boxASB.ASBuffer->SetName(L"Box-BLAS");
    asDesc.ScratchAccelerationStructureData = boxASB.scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = boxASB.ASBuffer->GetGPUVirtualAddress();

    command->BuildRaytracingAccelerationStructure(
        &asDesc, 0, nullptr);

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(planeASB.ASBuffer.Get()));
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(sphereASB.ASBuffer.Get()));
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(boxASB.ASBuffer.Get()));
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(glassASB.ASBuffer.Get()));
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(lightSphereASB.ASBuffer.Get()));

    command->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());
    command->Close();

    mDevice->ExecuteCommandList(command);

    // after exit this function,  local ASBs are destructed, so copy to member variables
    mMeshStage.blas = planeASB.ASBuffer;
    mMeshSphere.blas = sphereASB.ASBuffer;
    mMeshGlass.blas = glassASB.ASBuffer;
    mMeshMetal.blas = metalASB.ASBuffer;
    mMeshLightSphere.blas = lightSphereASB.ASBuffer;
    mMeshBox.blas = boxASB.ASBuffer;

    // wait, cuz scratchBuffers are destructed after exit this function
    mDevice->WaitForCompletePipe();
}

void DxrPhotonMapper::CreateSceneTLAS()
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    SetupMeshInfo(instanceDescs);

    ComPtr<ID3D12Resource> instanceDescBuffer;
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

    ComPtr<ID3D12Resource> instanceDescBuffer;
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