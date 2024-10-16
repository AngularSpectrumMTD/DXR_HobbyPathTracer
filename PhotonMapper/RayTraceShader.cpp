#include "DxrPhotonMapper.h"

void DxrPhotonMapper::CreateStateObject()
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
            dxilRayGen->SetDXILLibrary(&shaders[RayTracingDxlibs::RayGen]);
            dxilRayGen->DefineExport(RayTracingEntryPoints::RayGen);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[RayTracingDxlibs::Miss]);
            dxilMiss->DefineExport(RayTracingEntryPoints::Miss);
        }

        //AnyHit
        {
            auto dxilAnyHit = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilAnyHit->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialClosestHit]);
            dxilAnyHit->DefineExport(RayTracingEntryPoints::AnyHit);

            auto dxilAnyHitWithTex = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilAnyHitWithTex->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialWithTexClosestHit]);
            dxilAnyHitWithTex->DefineExport(RayTracingEntryPoints::AnyHitWithTex);
        }

        //Closest Hit
        {
            auto dxilChsMaterial = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterial->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialClosestHit]);
            dxilChsMaterial->DefineExport(RayTracingEntryPoints::ClosestHitMaterial);

            auto dxilChsMaterialWithTex = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterialWithTex->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialWithTexClosestHit]);
            dxilChsMaterialWithTex->DefineExport(RayTracingEntryPoints::ClosestHitMaterialWithTex);

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
            hitgroupGlass->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupGlass->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHit);
            hitgroupGlass->SetHitGroupExport(HitGroups::Obj0);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupMetal->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHit);
            hitgroupMetal->SetHitGroupExport(HitGroups::Obj1);

            auto hitgroupDefault = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefault->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefault->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupDefault->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHit);
            hitgroupDefault->SetHitGroupExport(HitGroups::DefaultMaterialSphere);

            auto hitgroupReflectReflact = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflact->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflact->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupReflectReflact->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHit);
            hitgroupReflectReflact->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);

            auto hitgroupDefaultBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupDefaultBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupDefaultBox->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupDefaultBox->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHit);
            hitgroupDefaultBox->SetHitGroupExport(HitGroups::DefaultMaterialBox);

            auto hitgroupReflectReflactBox = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupReflectReflactBox->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupReflectReflactBox->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupReflectReflactBox->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHit);
            hitgroupReflectReflactBox->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);

            auto hitgroupFloor = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupFloor->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialWithTex);
            hitgroupFloor->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHitWithTex);
            hitgroupFloor->SetHitGroupExport(HitGroups::Floor);

            for (const auto& instances : mOBJModel.getMaterialList())
            {
                wchar_t nameHitGroup[60];
                swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                auto hitgroupInstances = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                hitgroupInstances->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                hitgroupInstances->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialWithTex);
                hitgroupInstances->SetAnyHitShaderImport(RayTracingEntryPoints::AnyHitWithTex);
                hitgroupInstances->SetHitGroupExport(nameHitGroup);
            }
        }

        //Set Global Root Signature
        {
            auto rootsig = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            rootsig->SetRootSignature(mGlobalRootSig.Get());
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
            subobjects, IID_PPV_ARGS(mRTPSO.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("CreateStateObject Failed.");
        }
    }

    //Photon
    {
        const u32 MaxPayloadSize = sizeof(PhotonPayload);
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
            auto dxilChsMaterial = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterial->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialClosestHit]);
            dxilChsMaterial->DefineExport(RayTracingEntryPoints::ClosestHitMaterialPhoton);

            auto dxilChsMaterialWithTex = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterialWithTex->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialWithTexClosestHit]);
            dxilChsMaterialWithTex->DefineExport(RayTracingEntryPoints::ClosestHitMaterialWithTexPhoton);
        }

        //Hit Group
        {
            auto hitgroupGlass = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupGlass->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupGlass->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupGlass->SetHitGroupExport(HitGroups::Obj0);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialPhoton);
            hitgroupMetal->SetHitGroupExport(HitGroups::Obj1);

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
            hitgroupFloor->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialWithTexPhoton);
            hitgroupFloor->SetHitGroupExport(HitGroups::Floor);

            for (const auto& instances : mOBJModel.getMaterialList())
            {
                wchar_t nameHitGroup[60];
                swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                auto hitgroupInstances = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                hitgroupInstances->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                hitgroupInstances->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialWithTexPhoton);
                hitgroupInstances->SetHitGroupExport(nameHitGroup);
            }
        }

        //Set Global Root Signature
        {
            auto rootsig = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            rootsig->SetRootSignature(mGlobalRootSigPhoton.Get());
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

                auto lrsAssocMetal = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocMetal->AddExport(HitGroups::Obj1);
                lrsAssocMetal->SetSubobjectToAssociate(*bindLocalRootSig);

                auto lrsAssocGlass = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                lrsAssocGlass->AddExport(HitGroups::Obj0);
                lrsAssocGlass->SetSubobjectToAssociate(*bindLocalRootSig);
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
            subobjects, IID_PPV_ARGS(mRTPSOPhoton.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("CreateStateObject Failed.");
        }
    }

    //Reservoir Spatial Reuse(Currently using same hit group)
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
            dxilRayGen->DefineExport(RayTracingEntryPoints::RayGenSpatialReuse);
        }

        //Miss
        {
            auto dxilMiss = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilMiss->SetDXILLibrary(&shaders[RayTracingDxlibs::Miss]);
            dxilMiss->DefineExport(RayTracingEntryPoints::Miss);
        }

        //Closest Hit
        {
            auto dxilChsMaterial = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterial->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialClosestHit]);
            dxilChsMaterial->DefineExport(RayTracingEntryPoints::ClosestHitMaterial);

            auto dxilChsMaterialWithTex = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilChsMaterialWithTex->SetDXILLibrary(&shaders[RayTracingDxlibs::DefaultMaterialWithTexClosestHit]);
            dxilChsMaterialWithTex->DefineExport(RayTracingEntryPoints::ClosestHitMaterialWithTex);

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
            hitgroupGlass->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupGlass->SetHitGroupExport(HitGroups::Obj0);

            auto hitgroupMetal = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitgroupMetal->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitgroupMetal->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterial);
            hitgroupMetal->SetHitGroupExport(HitGroups::Obj1);

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
            hitgroupFloor->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialWithTex);
            hitgroupFloor->SetHitGroupExport(HitGroups::Floor);

            for (const auto& instances : mOBJModel.getMaterialList())
            {
                wchar_t nameHitGroup[60];
                swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                auto hitgroupInstances = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                hitgroupInstances->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                hitgroupInstances->SetClosestHitShaderImport(RayTracingEntryPoints::ClosestHitMaterialWithTex);
                hitgroupInstances->SetHitGroupExport(nameHitGroup);
            }
        }

        //Set Global Root Signature
        {
            auto rootsig = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            rootsig->SetRootSignature(mGlobalRootSigReservoirSpatialReuse.Get());
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
            subobjects, IID_PPV_ARGS(mRTPSOReservoirSpatialReuse.ReleaseAndGetAddressOf())
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