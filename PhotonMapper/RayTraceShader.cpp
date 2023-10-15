#include "DxrPhotonMapper.h"

void DxrPhotonMapper::CreateStateObject()
{
    //ShaderOBJ
    const auto RayGenShader = L"raygen.dxlib";
    const auto MissShader = L"miss.dxlib";
    const auto FloorClosestHitShader = L"closestHitFloor.dxlib";
    const auto DefaultMaterialClosestHitShader = L"closestHitMaterial.dxlib";
    const auto ReflectReflactMaterialClosestHitShader = L"closestHitReflectReflact.dxlib";
    const auto LightClosestHitShader = L"closestHitLight.dxlib";

    //ShaderEntryPoints
    const auto EntryPoint_RayGen_Normal = L"rayGen";
    const auto EntryPoint_Miss_Normal = L"miss";
    const auto EntryPoint_ClosestHit_Floor_Normal = L"floorClosestHit";
    const auto EntryPoint_ClosestHit_Material_Normal = L"materialClosestHit";
    const auto EntryPoint_ClosestHit_ReflectRefract_Normal = L"reflectReflactMaterialClosestHit";
    const auto EntryPoint_ClosestHit_Light_Normal = L"lightClosestHit";

    const auto EntryPoint_RayGen_Photon = L"photonEmitting";
    const auto EntryPoint_Miss_Photon = L"photonMiss";
    const auto EntryPoint_ClosestHit_Floor_Photon = L"floorStorePhotonClosestHit";
    const auto EntryPoint_ClosestHit_Material_Photon = L"materialStorePhotonClosestHit";
    const auto EntryPoint_ClosestHit_ReflectRefract_Photon = L"reflectReflactMaterialStorePhotonClosestHit";

    std::unordered_map<std::wstring, D3D12_SHADER_BYTECODE> shaders;
    const auto shaderFiles = {
        RayGenShader, MissShader,
        FloorClosestHitShader,
        DefaultMaterialClosestHitShader,
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
            dxilRayGen->DefineExport(EntryPoint_RayGen_Normal);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[MissShader]);
            dxilMiss->DefineExport(EntryPoint_Miss_Normal);
        }

        //Closest Hit
        {
            auto dxilChsFloor = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsFloor->SetDXILLibrary(&shaders[FloorClosestHitShader]);
            dxilChsFloor->DefineExport(EntryPoint_ClosestHit_Floor_Normal);

            auto dxilChsSphere1 = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsSphere1->SetDXILLibrary(&shaders[DefaultMaterialClosestHitShader]);
            dxilChsSphere1->DefineExport(EntryPoint_ClosestHit_Material_Normal);

            auto dxilChsGlass = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsGlass->SetDXILLibrary(&shaders[ReflectReflactMaterialClosestHitShader]);
            dxilChsGlass->DefineExport(EntryPoint_ClosestHit_ReflectRefract_Normal);

            auto dxilChsLight = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsLight->SetDXILLibrary(&shaders[LightClosestHitShader]);
            dxilChsLight->DefineExport(EntryPoint_ClosestHit_Light_Normal);
        }
    
        //Hit Group
        {
            auto hitgroupLight = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupLight->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupLight->SetClosestHitShaderImport(EntryPoint_ClosestHit_Light_Normal);
            hitgroupLight->SetHitGroupExport(HitGroups::Light);

            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Normal);
            hitgroupGlass->SetHitGroupExport(HitGroups::Glass);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Normal);
            hitgroupMetal->SetHitGroupExport(HitGroups::Metal);

            auto hitgroupDefault = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefault->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefault->SetClosestHitShaderImport(EntryPoint_ClosestHit_Material_Normal);
            hitgroupDefault->SetHitGroupExport(HitGroups::DefaultMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Normal);
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupDefaultBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefaultBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefaultBox->SetClosestHitShaderImport(EntryPoint_ClosestHit_Material_Normal);
            hitgroupDefaultBox->SetHitGroupExport(HitGroups::DefaultMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Normal);
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(EntryPoint_ClosestHit_Floor_Normal);
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

            auto rsDefaultSphere = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsDefaultSphere->SetRootSignature(mRsSphereDefault.Get());
            auto lrsAssocDefaultSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocDefaultSphere->AddExport(HitGroups::DefaultMaterialSphere);
            lrsAssocDefaultSphere->SetSubobjectToAssociate(*rsDefaultSphere);

            auto rsReflectReflactBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsReflectReflactBox->SetRootSignature(mRsSphereRR.Get());
            auto lrsAssocReflectReflactBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocReflectReflactBox->AddExport(HitGroups::ReflectReflactMaterialBox);
            lrsAssocReflectReflactBox->SetSubobjectToAssociate(*rsReflectReflactBox);

            auto rsDefaultBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsDefaultBox->SetRootSignature(mRsSphereDefault.Get());
            auto lrsAssocDefaultBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocDefaultBox->AddExport(HitGroups::DefaultMaterialBox);
            lrsAssocDefaultBox->SetSubobjectToAssociate(*rsDefaultBox);

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
        const u32 photonPayloadSize = sizeof(XMFLOAT3) + 3 * sizeof(INT) + sizeof(f32);
        const u32 basicTracePayloadSize = 3 * sizeof(XMFLOAT3) + 4 * sizeof(INT) + sizeof(f32);
        const u32 MaxPayloadSize = max(photonPayloadSize, basicTracePayloadSize);
        const u32 MaxAttributeSize = sizeof(XMFLOAT2);//sizeof(TriangleIntersectionAttributes)
        const u32 MaxRecursionDepth = MAX_RECURSION_DEPTH;

        CD3DX12_STATE_OBJECT_DESC subobjects;
        subobjects.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

         //Ray Gen
        {
            auto dxilRayGen = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilRayGen->SetDXILLibrary(&shaders[RayGenShader]);
            dxilRayGen->DefineExport(EntryPoint_RayGen_Photon);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[MissShader]);
            dxilMiss->DefineExport(EntryPoint_Miss_Photon);
        }

        //Closest Hit
        {
            auto dxilChsFloor = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsFloor->SetDXILLibrary(&shaders[FloorClosestHitShader]);
            dxilChsFloor->DefineExport(EntryPoint_ClosestHit_Floor_Photon);

            auto dxilChsSphere1 = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsSphere1->SetDXILLibrary(&shaders[DefaultMaterialClosestHitShader]);
            dxilChsSphere1->DefineExport(EntryPoint_ClosestHit_Material_Photon);

            auto dxilChsGlass = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsGlass->SetDXILLibrary(&shaders[ReflectReflactMaterialClosestHitShader]);
            dxilChsGlass->DefineExport(EntryPoint_ClosestHit_ReflectRefract_Photon);
        }

        //Hit Group
        {
            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Photon);
            hitgroupGlass->SetHitGroupExport(HitGroups::Glass);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Photon);
            hitgroupMetal->SetHitGroupExport(HitGroups::Metal);

            auto hitgroupDefault = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefault->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefault->SetClosestHitShaderImport(EntryPoint_ClosestHit_Material_Photon);
            hitgroupDefault->SetHitGroupExport(HitGroups::DefaultMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Photon);
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupDefaultBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefaultBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefaultBox->SetClosestHitShaderImport(EntryPoint_ClosestHit_Material_Photon);
            hitgroupDefaultBox->SetHitGroupExport(HitGroups::DefaultMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(EntryPoint_ClosestHit_ReflectRefract_Photon);
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(EntryPoint_ClosestHit_Floor_Photon);
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

            auto rsDefaultSphere = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsDefaultSphere->SetRootSignature(mRsSphereDefaultPhoton.Get());
            auto lrsAssocDefaultSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocDefaultSphere->AddExport(HitGroups::DefaultMaterialSphere);
            lrsAssocDefaultSphere->SetSubobjectToAssociate(*rsDefaultSphere);

            auto rsReflectReflactBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsReflectReflactBox->SetRootSignature(mRsSphereRRPhoton.Get());
            auto lrsAssocReflectReflactBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocReflectReflactBox->AddExport(HitGroups::ReflectReflactMaterialBox);
            lrsAssocReflectReflactBox->SetSubobjectToAssociate(*rsReflectReflactBox);

            auto rsDefaultBox = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            rsDefaultBox->SetRootSignature(mRsSphereDefaultPhoton.Get());
            auto lrsAssocDefaultBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            lrsAssocDefaultBox->AddExport(HitGroups::DefaultMaterialBox);
            lrsAssocDefaultBox->SetSubobjectToAssociate(*rsDefaultBox);

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