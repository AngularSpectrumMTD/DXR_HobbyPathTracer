#include "DxrPhotonMapper.h"

void DxrPhotonMapper::CreateStateObject()
{
    const auto RayGenShader = L"raygen.dxlib";
    const auto MissShader = L"miss.dxlib";
    const auto FloorClosestHitShader = L"closestHitFloor.dxlib";
    const auto PhongMaterialClosestHitShader = L"closestHitPhong.dxlib";
    const auto ReflectReflactMaterialClosestHitShader = L"closestHitReflectReflact.dxlib";
    const auto LightClosestHitShader = L"closestHitLight.dxlib";

    std::unordered_map<std::wstring, D3D12_SHADER_BYTECODE> shaders;
    const auto shaderFiles = {
        RayGenShader, MissShader,
        FloorClosestHitShader,
        PhongMaterialClosestHitShader,
        ReflectReflactMaterialClosestHitShader,
        LightClosestHitShader };

    for (auto& filename : shaderFiles) {
        u32 fileSize = 0;
        UINT8* shaderCodePtr;
        utility::ReadDataFromFile(filename, &shaderCodePtr, &fileSize);
        shaders.emplace(filename, D3D12_SHADER_BYTECODE());
        shaders[filename] = CD3DX12_SHADER_BYTECODE((void*)shaderCodePtr, fileSize);
    }

    //Normal State Object
    {
        const u32 MaxPayloadSize = 3 * sizeof(XMFLOAT3) + 4 * sizeof(INT) + sizeof(f32);//sizeof(Payload)
        const u32 MaxAttributeSize = sizeof(XMFLOAT2);//sizeof(TriangleIntersectionAttributes)
        const u32 MaxRecursionDepth = MAX_RECURSION_DEPTH;

        CD3DX12_STATE_OBJECT_DESC subobjects;
        subobjects.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

        //Ray Gen
        {
            auto dxilRayGen = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilRayGen->SetDXILLibrary(&shaders[RayGenShader]);
            dxilRayGen->DefineExport(L"rayGen");
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[MissShader]);
            dxilMiss->DefineExport(L"miss");
        }

        //Closest Hit
        {
            auto dxilChsFloor = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsFloor->SetDXILLibrary(&shaders[FloorClosestHitShader]);
            dxilChsFloor->DefineExport(L"floorClosestHit");

            auto dxilChsSphere1 = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsSphere1->SetDXILLibrary(&shaders[PhongMaterialClosestHitShader]);
            dxilChsSphere1->DefineExport(L"phongMaterialClosestHit");

            auto dxilChsGlass = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsGlass->SetDXILLibrary(&shaders[ReflectReflactMaterialClosestHitShader]);
            dxilChsGlass->DefineExport(L"reflectReflactMaterialClosestHit");

            auto dxilChsLight = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsLight->SetDXILLibrary(&shaders[LightClosestHitShader]);
            dxilChsLight->DefineExport(L"lightClosestHit");
        }
    
        //Hit Group
        {
            auto hitgroupLight = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupLight->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupLight->SetClosestHitShaderImport(L"lightClosestHit");
            hitgroupLight->SetHitGroupExport(HitGroups::Light);

            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(L"reflectReflactMaterialClosestHit");
            hitgroupGlass->SetHitGroupExport(HitGroups::Glass);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(L"reflectReflactMaterialClosestHit");
            hitgroupMetal->SetHitGroupExport(HitGroups::Metal);

            auto hitgroupPhong = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupPhong->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupPhong->SetClosestHitShaderImport(L"phongMaterialClosestHit");
            hitgroupPhong->SetHitGroupExport(HitGroups::PhongMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(L"reflectReflactMaterialClosestHit");
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupPhongBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupPhongBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupPhongBox->SetClosestHitShaderImport(L"phongMaterialClosestHit");
            hitgroupPhongBox->SetHitGroupExport(HitGroups::PhongMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(L"reflectReflactMaterialClosestHit");
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(L"floorClosestHit");
            hitgroupFloor->SetHitGroupExport(HitGroups::Floor);
        }

        //Set Global Root Signature
        {
            auto rootsig = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            rootsig->SetRootSignature(mGrs.Get());
        }

        //Bind Local Root Signature For Shader
        {
            auto rsFloor = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsFloor->SetRootSignature(mRsFloor.Get());
            auto lrsAssocFloor = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocFloor->AddExport(HitGroups::Floor);
            lrsAssocFloor->SetSubobjectToAssociate(*rsFloor);

            auto rsReflectReflactSphere = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsReflectReflactSphere->SetRootSignature(mRsSphereRR.Get());
            auto lrsAssocReflectReflactSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocReflectReflactSphere->AddExport(HitGroups::ReflectReflactMaterialSphere);
            lrsAssocReflectReflactSphere->SetSubobjectToAssociate(*rsReflectReflactSphere);

            auto rsPhongSphere = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsPhongSphere->SetRootSignature(mRsSpherePhong.Get());
            auto lrsAssocPhongSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocPhongSphere->AddExport(HitGroups::PhongMaterialSphere);
            lrsAssocPhongSphere->SetSubobjectToAssociate(*rsPhongSphere);

            auto rsReflectReflactBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsReflectReflactBox->SetRootSignature(mRsSphereRR.Get());
            auto lrsAssocReflectReflactBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocReflectReflactBox->AddExport(HitGroups::ReflectReflactMaterialBox);
            lrsAssocReflectReflactBox->SetSubobjectToAssociate(*rsReflectReflactBox);

            auto rsPhongBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsPhongBox->SetRootSignature(mRsSpherePhong.Get());
            auto lrsAssocPhongBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocPhongBox->AddExport(HitGroups::PhongMaterialBox);
            lrsAssocPhongBox->SetSubobjectToAssociate(*rsPhongBox);

            auto rsGlass = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsGlass->SetRootSignature(mRsGlass.Get());
            auto lrsAssocGlass = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocGlass->AddExport(HitGroups::Glass);
            lrsAssocGlass->SetSubobjectToAssociate(*rsGlass);

            auto rsMetal = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsMetal->SetRootSignature(mRsGlass.Get());
            auto lrsAssocMetal = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocMetal->AddExport(HitGroups::Metal);
            lrsAssocMetal->SetSubobjectToAssociate(*rsMetal);

            auto rsLight = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsLight->SetRootSignature(mRsSphereRR.Get());
            auto lrsAssocLight = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocLight->AddExport(HitGroups::Light);
            lrsAssocLight->SetSubobjectToAssociate(*rsLight);
        }

        auto shaderConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(MaxPayloadSize, MaxAttributeSize);

        auto pipelineConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(MaxRecursionDepth);

        HRESULT hr = mDevice->GetDevice()->CreateStateObject(
            subobjects, IID_PPV_ARGS(mRTPSO.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("CreateStateObject Failed.");
        }
    }

    //Photon
    {
        const u32 MaxPayloadSize = sizeof(XMFLOAT3) + 4 * sizeof(INT) + sizeof(f32);//sizeof(PhotonPayload)
        const u32 MaxAttributeSize = sizeof(XMFLOAT2);//sizeof(TriangleIntersectionAttributes)
        const u32 MaxRecursionDepth = MAX_RECURSION_DEPTH;

        CD3DX12_STATE_OBJECT_DESC subobjects;
        subobjects.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

         //Ray Gen
        {
            auto dxilRayGen = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilRayGen->SetDXILLibrary(&shaders[RayGenShader]);
            dxilRayGen->DefineExport(L"photonEmitting");
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[MissShader]);
            dxilMiss->DefineExport(L"photonMiss");
        }

        //Closest Hit
        {
            auto dxilChsFloor = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsFloor->SetDXILLibrary(&shaders[FloorClosestHitShader]);
            dxilChsFloor->DefineExport(L"floorStorePhotonClosestHit");

            auto dxilChsSphere1 = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsSphere1->SetDXILLibrary(&shaders[PhongMaterialClosestHitShader]);
            dxilChsSphere1->DefineExport(L"phongMaterialStorePhotonClosestHit");

            auto dxilChsGlass = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsGlass->SetDXILLibrary(&shaders[ReflectReflactMaterialClosestHitShader]);
            dxilChsGlass->DefineExport(L"reflectReflactMaterialStorePhotonClosestHit");
        }

        //Hit Group
        {
            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(L"reflectReflactMaterialStorePhotonClosestHit");
            hitgroupGlass->SetHitGroupExport(HitGroups::Glass);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(L"reflectReflactMaterialStorePhotonClosestHit");
            hitgroupMetal->SetHitGroupExport(HitGroups::Metal);

            auto hitgroupPhong = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupPhong->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupPhong->SetClosestHitShaderImport(L"phongMaterialStorePhotonClosestHit");
            hitgroupPhong->SetHitGroupExport(HitGroups::PhongMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(L"reflectReflactMaterialStorePhotonClosestHit");
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupPhongBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupPhongBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupPhongBox->SetClosestHitShaderImport(L"phongMaterialStorePhotonClosestHit");
            hitgroupPhongBox->SetHitGroupExport(HitGroups::PhongMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(L"reflectReflactMaterialStorePhotonClosestHit");
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(L"floorStorePhotonClosestHit");
            hitgroupFloor->SetHitGroupExport(HitGroups::Floor);
        }

        //Set Global Root Signature
        {
            auto rootsig = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            rootsig->SetRootSignature(mGrsPhoton.Get());
        }

        //Bind Local Root Signature For Shader
        {
            auto rsFloor = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsFloor->SetRootSignature(mRsFloorPhoton.Get());
            auto lrsAssocFloor = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocFloor->AddExport(HitGroups::Floor);
            lrsAssocFloor->SetSubobjectToAssociate(*rsFloor);

            auto rsReflectReflactSphere = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsReflectReflactSphere->SetRootSignature(mRsSphereRRPhoton.Get());
            auto lrsAssocReflectReflactSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocReflectReflactSphere->AddExport(HitGroups::ReflectReflactMaterialSphere);
            lrsAssocReflectReflactSphere->SetSubobjectToAssociate(*rsReflectReflactSphere);

            auto rsPhongSphere = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsPhongSphere->SetRootSignature(mRsSpherePhongPhoton.Get());
            auto lrsAssocPhongSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocPhongSphere->AddExport(HitGroups::PhongMaterialSphere);
            lrsAssocPhongSphere->SetSubobjectToAssociate(*rsPhongSphere);

            auto rsReflectReflactBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsReflectReflactBox->SetRootSignature(mRsSphereRRPhoton.Get());
            auto lrsAssocReflectReflactBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocReflectReflactBox->AddExport(HitGroups::ReflectReflactMaterialBox);
            lrsAssocReflectReflactBox->SetSubobjectToAssociate(*rsReflectReflactBox);

            auto rsPhongBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsPhongBox->SetRootSignature(mRsSpherePhongPhoton.Get());
            auto lrsAssocPhongBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocPhongBox->AddExport(HitGroups::PhongMaterialBox);
            lrsAssocPhongBox->SetSubobjectToAssociate(*rsPhongBox);

            auto rsMetal = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsMetal->SetRootSignature(mRsGlassPhoton.Get());
            auto lrsAssocMetal = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocMetal->AddExport(HitGroups::Metal);
            lrsAssocMetal->SetSubobjectToAssociate(*rsMetal);

            auto rsGlass = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsGlass->SetRootSignature(mRsGlassPhoton.Get());
            auto lrsAssocGlass = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocGlass->AddExport(HitGroups::Glass);
            lrsAssocGlass->SetSubobjectToAssociate(*rsGlass);
        }

        auto shaderConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(MaxPayloadSize, MaxAttributeSize);

        auto pipelineConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(MaxRecursionDepth);

        HRESULT hr = mDevice->GetDevice()->CreateStateObject(
            subobjects, IID_PPV_ARGS(mRTPSOPhoton.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("CreateStateObject Failed.");
        }
    }
}

void DxrPhotonMapper::CreateRaytracingRootSignatureAndPSO()
{
    CreateRootSignatureGlobal();
    CreateRootSignatureLocal();
    CreateStateObject();
}