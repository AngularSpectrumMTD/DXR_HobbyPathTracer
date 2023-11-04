#include "DxrPhotonMapper.h"

void DxrPhotonMapper::CreateStateObject()
{
    std::unordered_map<std::wstring, D3D12_SHADER_BYTECODE> shaders;
    const auto shaderFiles = {
        RayTracingDxlibs::RayGen,
        RayTracingDxlibs::Miss,
        RayTracingDxlibs::FloorClosestHit,
        RayTracingDxlibs::DefaultMaterialClosestHit,
        RayTracingDxlibs::LightClosestHit };

    for (auto& filename : shaderFiles) {
        u32 fileSize = 0;
        UINT8* shaderCodePtr;
        utility::ReadDataFromFile(filename, &shaderCodePtr, &fileSize);
        shaders.emplace(filename, D3D12_SHADER_BYTECODE());
        shaders[filename] = CD3DX12_SHADER_BYTECODE((void*)shaderCodePtr, fileSize);
    }

    //Normal State Object
    {
        const u32 MaxPayloadSize = sizeof(Payload);
        const u32 MaxAttributeSize = sizeof(TriangleIntersectionAttributes);
        const u32 MaxRecursionDepth = MAX_RECURSION_DEPTH;

        CD3DX12_STATE_OBJECT_DESC subobjects;
        subobjects.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

        //Ray Gen
        {
            auto dxilRayGen = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilRayGen->SetDXILLibrary(&shaders[RayTracingDxlibs::RayGen]);
            dxilRayGen->DefineExport(RayTracingEntryPoints::RayGen);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[RayTracingDxlibs::Miss]);
            dxilMiss->DefineExport(RayTracingEntryPoints::Miss);
        }

        //Closest Hit
        {
            auto dxilChsFloor = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsFloor->SetDXILLibrary(&shaders[RayTracingDxlibs::FloorClosestHit]);
            dxilChsFloor->DefineExport(RayTracingEntryPoints::ClosestHitFloor);

            auto dxilChsSphere = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsSphere->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialClosestHit]);
            dxilChsSphere->DefineExport(RayTracingEntryPoints::ClosestHitMaterial);

            auto dxilChsLight = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsLight->SetDXILLibrary(&shaders[RayTracingDxlibs::LightClosestHit]);
            dxilChsLight->DefineExport(RayTracingEntryPoints::ClosestHitLight);
        }
    
        //Hit Group
        {
            auto hitgroupLight = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupLight->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupLight->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitLight);
            hitgroupLight->SetHitGroupExport(HitGroups::Light);

            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupGlass->SetHitGroupExport(HitGroups::Glass);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupMetal->SetHitGroupExport(HitGroups::Metal);

            auto hitgroupDefault = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefault->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefault->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupDefault->SetHitGroupExport(HitGroups::DefaultMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupDefaultBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefaultBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefaultBox->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupDefaultBox->SetHitGroupExport(HitGroups::DefaultMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitFloor);
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
        const u32 photonPayloadSize = sizeof(PhotonPayload);
        const u32 payloadSize = sizeof(Payload);
        const u32 MaxPayloadSize = max(photonPayloadSize, payloadSize);
        const u32 MaxAttributeSize = sizeof(TriangleIntersectionAttributes);
        const u32 MaxRecursionDepth = MAX_RECURSION_DEPTH;

        CD3DX12_STATE_OBJECT_DESC subobjects;
        subobjects.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

         //Ray Gen
        {
            auto dxilRayGen = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilRayGen->SetDXILLibrary(&shaders[RayTracingDxlibs::RayGen]);
            dxilRayGen->DefineExport(RayTracingEntryPoints::RayGenPhoton);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[RayTracingDxlibs::Miss]);
            dxilMiss->DefineExport(RayTracingEntryPoints::MissPhoton);
        }

        //Closest Hit
        {
            auto dxilChsFloor = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsFloor->SetDXILLibrary(&shaders[RayTracingDxlibs::FloorClosestHit]);
            dxilChsFloor->DefineExport(RayTracingEntryPoints::ClosestHitFloorPhoton);

            auto dxilChsSphere = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsSphere->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialClosestHit]);
            dxilChsSphere->DefineExport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
        }

        //Hit Group
        {
            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupGlass->SetHitGroupExport(HitGroups::Glass);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupMetal->SetHitGroupExport(HitGroups::Metal);

            auto hitgroupDefault = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefault->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefault->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupDefault->SetHitGroupExport(HitGroups::DefaultMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupDefaultBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefaultBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefaultBox->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupDefaultBox->SetHitGroupExport(HitGroups::DefaultMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitFloorPhoton);
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