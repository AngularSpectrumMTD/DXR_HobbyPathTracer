#include "DXRPathTracer.h"

void DXRPathTracer::CreateStateObject(ComPtr<ID3D12StateObject>& stateObject, ComPtr<ID3D12RootSignature>& globalRootSignature, const u32 maxPayloadSize, const u32 maxAttributeSize, const u32 maxRecursionDepth, const wchar_t* rayGenLibraryName, const wchar_t* rayGenShaderName, const wchar_t* missLibraryName, const wchar_t* missShaderName, const wchar_t* chLibraryName, const wchar_t* chShaderName, const wchar_t* chLibraryNameWithTex, const wchar_t* chShaderNameWithTex, const wchar_t* ahShaderName, const wchar_t* ahShaderNameWithTex)
{
    std::unordered_map<std::wstring, D3D12_SHADER_BYTECODE> shaders;
    const auto shaderFiles = {
        RayTracingDxlibs::RayGen,
        RayTracingDxlibs::Miss,
        RayTracingDxlibs::DefaultMaterialClosestHit,
        RayTracingDxlibs::DefaultMaterialWithTexClosestHit };
    //RayTracingDxlibs::LightClosestHit };

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
            dxilRayGen->SetDXILLibrary(&shaders[rayGenLibraryName]);
            dxilRayGen->DefineExport(rayGenShaderName);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[missLibraryName]);
            dxilMiss->DefineExport(missShaderName);
        }

        //AnyHit
        {
            if (ahShaderName != nullptr)
            {
                auto dxilAnyHit = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                dxilAnyHit->SetDXILLibrary(&shaders[chLibraryName]);
                dxilAnyHit->DefineExport(ahShaderName);
            }

            if (ahShaderNameWithTex != nullptr)
            {
                auto dxilAnyHitWithTex = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                dxilAnyHitWithTex->SetDXILLibrary(&shaders[chLibraryNameWithTex]);
                dxilAnyHitWithTex->DefineExport(ahShaderNameWithTex);
            }
        }

        //Closest Hit
        {
            auto dxilChsMaterial = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterial->SetDXILLibrary(&shaders[chLibraryName]);
            dxilChsMaterial->DefineExport(chShaderName);

            auto dxilChsMaterialWithTex = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterialWithTex->SetDXILLibrary(&shaders[chLibraryNameWithTex]);
            dxilChsMaterialWithTex->DefineExport(chShaderNameWithTex);

            /*   auto dxilChsLight = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
               dxilChsLight->SetDXILLibrary(&shaders[RayTracingDxlibs::LightClosestHit]);
               dxilChsLight->DefineExport(RayTracingEntryPoints::ClosestHitLight);*/
        }

        //Hit Group
        {
            /*   auto hitgroupLight = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
               hitgroupLight->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
               hitgroupLight->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitLight);
               hitgroupLight->SetHitGroupExport(HitGroups::Light);*/

            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(chShaderName);
            if (ahShaderName != nullptr)
            {
                hitgroupGlass->SetAnyHitShaderImport(ahShaderName);
            }
            hitgroupGlass->SetHitGroupExport(HitGroups::Obj0);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(chShaderName);
            if (ahShaderName != nullptr)
            {
                hitgroupMetal->SetAnyHitShaderImport(ahShaderName);
            }
            hitgroupMetal->SetHitGroupExport(HitGroups::Obj1);

            auto hitgroupDefault = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefault->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefault->SetClosestHitShaderImport(chShaderName);
            if (ahShaderName != nullptr)
            {
                hitgroupDefault->SetAnyHitShaderImport(ahShaderName);
            }
            hitgroupDefault->SetHitGroupExport(HitGroups::DefaultMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(chShaderName);
            if (ahShaderName != nullptr)
            {
                hitgroupReflectReflact->SetAnyHitShaderImport(ahShaderName);
            }
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupDefaultBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefaultBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefaultBox->SetClosestHitShaderImport(chShaderName);
            if (ahShaderName != nullptr)
            {
                hitgroupDefaultBox->SetAnyHitShaderImport(ahShaderName);
            }
            hitgroupDefaultBox->SetHitGroupExport(HitGroups::DefaultMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(chShaderName);
            if (ahShaderName != nullptr)
            {
                hitgroupReflectReflactBox->SetAnyHitShaderImport(ahShaderName);
            }
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(chShaderNameWithTex);
            if (ahShaderNameWithTex != nullptr)
            {
                hitgroupFloor->SetAnyHitShaderImport(ahShaderNameWithTex);
            }
            hitgroupFloor->SetHitGroupExport(HitGroups::Floor);

            for (const auto& instances : mOBJModel.getMaterialList())
            {
                wchar_t nameHitGroup[60];
                swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                auto hitgroupInstances = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                hitgroupInstances->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                hitgroupInstances->SetClosestHitShaderImport(chShaderNameWithTex);
                if (ahShaderNameWithTex != nullptr)
                {
                    hitgroupInstances->SetAnyHitShaderImport(ahShaderNameWithTex);
                }
                hitgroupInstances->SetHitGroupExport(nameHitGroup);
            }
        }

        //Set Global Root Signature
        {
            auto rootsig = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            rootsig->SetRootSignature(globalRootSignature.Get());
        }

        //Bind Local Root Signature For Shader
        {
            //Material
            {
                auto bindLocalRootSig = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
                bindLocalRootSig->SetRootSignature(mLocalRootSigMaterial.Get());

                auto lrsAssocReflectReflactSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocReflectReflactSphere->AddExport(HitGroups::ReflectReflactMaterialSphere);
                lrsAssocReflectReflactSphere->SetSubobjectToAssociate(*bindLocalRootSig);

                auto lrsAssocDefaultSphere = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocDefaultSphere->AddExport(HitGroups::DefaultMaterialSphere);
                lrsAssocDefaultSphere->SetSubobjectToAssociate(*bindLocalRootSig);

                auto lrsAssocReflectReflactBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocReflectReflactBox->AddExport(HitGroups::ReflectReflactMaterialBox);
                lrsAssocReflectReflactBox->SetSubobjectToAssociate(*bindLocalRootSig);

                auto lrsAssocDefaultBox = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocDefaultBox->AddExport(HitGroups::DefaultMaterialBox);
                lrsAssocDefaultBox->SetSubobjectToAssociate(*bindLocalRootSig);

                auto lrsAssocGlass = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocGlass->AddExport(HitGroups::Obj0);
                lrsAssocGlass->SetSubobjectToAssociate(*bindLocalRootSig);

                auto lrsAssocMetal = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocMetal->AddExport(HitGroups::Obj1);
                lrsAssocMetal->SetSubobjectToAssociate(*bindLocalRootSig);
            }
            //MaterialWithTex
            {
                auto bindLocalRootSig = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
                bindLocalRootSig->SetRootSignature(mLocalRootSigMaterialWithTex.Get());

                auto lrsAssocFloor = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocFloor->AddExport(HitGroups::Floor);
                lrsAssocFloor->SetSubobjectToAssociate(*bindLocalRootSig);

                for (const auto& instances : mOBJModel.getMaterialList())
                {
                    wchar_t nameHitGroup[60];
                    swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                    auto lrsAssoc = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                    lrsAssoc->AddExport(nameHitGroup);
                    lrsAssoc->SetSubobjectToAssociate(*bindLocalRootSig);
                }
            }
        }

        auto shaderConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(MaxPayloadSize, MaxAttributeSize);

        auto pipelineConfig = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(MaxRecursionDepth);

        HRESULT hr = mDevice->GetDevice()->CreateStateObject(
            subobjects, IID_PPV_ARGS(stateObject.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("CreateStateObject Failed.");
        }
    }
}

void DXRPathTracer::CreateStateObjects()
{
    CreateStateObject(mRTPSO, mGlobalRootSig, sizeof(Payload), sizeof(TriangleIntersectionAttributes), MAX_RECURSION_DEPTH, RayTracingDxlibs::RayGen, RayTracingEntryPoints::RayGen, RayTracingDxlibs::Miss, RayTracingEntryPoints::Miss, RayTracingDxlibs::DefaultMaterialClosestHit, RayTracingEntryPoints::ClosestHitMaterial, RayTracingDxlibs::DefaultMaterialWithTexClosestHit, RayTracingEntryPoints::ClosestHitMaterialWithTex, RayTracingEntryPoints::AnyHit, RayTracingEntryPoints::AnyHitWithTex);
    CreateStateObject(mRTPSOPhoton, mGlobalRootSigPhoton, sizeof(PhotonPayload), sizeof(TriangleIntersectionAttributes), MAX_RECURSION_DEPTH, RayTracingDxlibs::RayGen, RayTracingEntryPoints::RayGenPhoton, RayTracingDxlibs::Miss, RayTracingEntryPoints::MissPhoton, RayTracingDxlibs::DefaultMaterialClosestHit, RayTracingEntryPoints::ClosestHitMaterialPhoton, RayTracingDxlibs::DefaultMaterialWithTexClosestHit, RayTracingEntryPoints::ClosestHitMaterialWithTexPhoton);
    CreateStateObject(mRTPSOReservoirSpatialReuse, mGlobalRootSigReservoirSpatialReuse, sizeof(Payload), sizeof(TriangleIntersectionAttributes), MAX_RECURSION_DEPTH, RayTracingDxlibs::RayGen, RayTracingEntryPoints::RayGenSpatialReuse, RayTracingDxlibs::Miss, RayTracingEntryPoints::Miss, RayTracingDxlibs::DefaultMaterialClosestHit, RayTracingEntryPoints::ClosestHitMaterial, RayTracingDxlibs::DefaultMaterialWithTexClosestHit, RayTracingEntryPoints::ClosestHitMaterialWithTex, RayTracingEntryPoints::AnyHit, RayTracingEntryPoints::AnyHitWithTex);
    CreateStateObject(mRTPSOReservoirTemporalReuse, mGlobalRootSigReservoirTemporalReuse, sizeof(Payload), sizeof(TriangleIntersectionAttributes), MAX_RECURSION_DEPTH, RayTracingDxlibs::RayGen, RayTracingEntryPoints::RayGenTemporalReuse, RayTracingDxlibs::Miss, RayTracingEntryPoints::Miss, RayTracingDxlibs::DefaultMaterialClosestHit, RayTracingEntryPoints::ClosestHitMaterial, RayTracingDxlibs::DefaultMaterialWithTexClosestHit, RayTracingEntryPoints::ClosestHitMaterialWithTex, RayTracingEntryPoints::AnyHit, RayTracingEntryPoints::AnyHitWithTex);
}

void DXRPathTracer::CreateRaytracingRootSignatureAndPSO()
{
    CreateRootSignatureGlobal();
    CreateRootSignatureLocal();
    CreateStateObjects();
}