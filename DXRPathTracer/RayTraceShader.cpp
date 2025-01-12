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

    {
        CD3DX12_STATE_OBJECT_DESC subobjects;
        subobjects.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

        //Ray Gen
        {
            auto subObj = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            subObj->SetDXILLibrary(&shaders[rayGenLibraryName]);
            subObj->DefineExport(rayGenShaderName);
        }

        //Miss
        {
            auto subObj = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            subObj->SetDXILLibrary(&shaders[missLibraryName]);
            subObj->DefineExport(missShaderName);
        }

        //AnyHit
        {
            if (ahShaderName != nullptr)
            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                subObj->SetDXILLibrary(&shaders[chLibraryName]);
                subObj->DefineExport(ahShaderName);
            }

            if (ahShaderNameWithTex != nullptr)
            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                subObj->SetDXILLibrary(&shaders[chLibraryNameWithTex]);
                subObj->DefineExport(ahShaderNameWithTex);
            }
        }

        //Closest Hit
        {
            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                subObj->SetDXILLibrary(&shaders[chLibraryName]);
                subObj->DefineExport(chShaderName);
            }

            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                subObj->SetDXILLibrary(&shaders[chLibraryNameWithTex]);
                subObj->DefineExport(chShaderNameWithTex);
            }
        }

        //Hit Group
        {
            {
                for (u32 i = 0; i < NormalObjs; i++)
                {
                    auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                    subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                    subObj->SetClosestHitShaderImport(chShaderName);
                    if (ahShaderName != nullptr)
                    {
                        subObj->SetAnyHitShaderImport(ahShaderName);
                    }
                    subObj->SetHitGroupExport(HitGroups::Obj[i]);
                }
            }

            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                subObj->SetClosestHitShaderImport(chShaderName);
                if (ahShaderName != nullptr)
                {
                    subObj->SetAnyHitShaderImport(ahShaderName);
                }
                subObj->SetHitGroupExport(HitGroups::DefaultMaterialSphere);
            }

            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                subObj->SetClosestHitShaderImport(chShaderName);
                if (ahShaderName != nullptr)
                {
                    subObj->SetAnyHitShaderImport(ahShaderName);
                }
                subObj->SetHitGroupExport(HitGroups::ReflectReflactMaterialSphere);
            }

            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                subObj->SetClosestHitShaderImport(chShaderName);
                if (ahShaderName != nullptr)
                {
                    subObj->SetAnyHitShaderImport(ahShaderName);
                }
                subObj->SetHitGroupExport(HitGroups::DefaultMaterialBox);
            }

            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                subObj->SetClosestHitShaderImport(chShaderName);
                if (ahShaderName != nullptr)
                {
                    subObj->SetAnyHitShaderImport(ahShaderName);
                }
                subObj->SetHitGroupExport(HitGroups::ReflectReflactMaterialBox);
            }

            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                subObj->SetClosestHitShaderImport(chShaderNameWithTex);
                if (ahShaderNameWithTex != nullptr)
                {
                    subObj->SetAnyHitShaderImport(ahShaderNameWithTex);
                }
                subObj->SetHitGroupExport(HitGroups::Floor);
            }

            for (const auto& instances : mOBJMaterialLinkedMesh.getMaterialList())
            {
                wchar_t nameHitGroup[60];
                swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                auto subObj = subobjects.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                subObj->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                subObj->SetClosestHitShaderImport(chShaderNameWithTex);
                if (ahShaderNameWithTex != nullptr)
                {
                    subObj->SetAnyHitShaderImport(ahShaderNameWithTex);
                }
                subObj->SetHitGroupExport(nameHitGroup);
            }
        }

        //Root Signature
        {
            //Set Global Root Signature
            {
                auto subObj = subobjects.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
                subObj->SetRootSignature(globalRootSignature.Get());
            }

            //Bind Local Root Signature For Shader
            {
                //Material
                {
                    auto localRootSig = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
                    localRootSig->SetRootSignature(mLocalRootSigMaterial.Get());

                    {
                        auto subObj = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                        subObj->AddExport(HitGroups::ReflectReflactMaterialSphere);
                        subObj->SetSubobjectToAssociate(*localRootSig);
                    }
                    {
                        auto subObj = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                        subObj->AddExport(HitGroups::DefaultMaterialSphere);
                        subObj->SetSubobjectToAssociate(*localRootSig);
                    }
                    {
                        auto subObj = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                        subObj->AddExport(HitGroups::ReflectReflactMaterialBox);
                        subObj->SetSubobjectToAssociate(*localRootSig);
                    }
                    {
                        auto subObj = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                        subObj->AddExport(HitGroups::DefaultMaterialBox);
                        subObj->SetSubobjectToAssociate(*localRootSig);
                    }
                    {
                        for (u32 i = 0; i < NormalObjs; i++)
                        {
                            auto subObj = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                            subObj->AddExport(HitGroups::Obj[i]);
                            subObj->SetSubobjectToAssociate(*localRootSig);
                        }
                    }
                }

                //MaterialWithTex
                {
                    auto localRootSig = subobjects.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
                    localRootSig->SetRootSignature(mLocalRootSigMaterialWithTex.Get());

                    auto lrsAssocFloor = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                    lrsAssocFloor->AddExport(HitGroups::Floor);
                    lrsAssocFloor->SetSubobjectToAssociate(*localRootSig);

                    for (const auto& instances : mOBJMaterialLinkedMesh.getMaterialList())
                    {
                        wchar_t nameHitGroup[60];
                        swprintf(nameHitGroup, 60, L"%ls", utility::StringToWString(instances.MaterialName).c_str());
                        auto lrsAssoc = subobjects.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
                        lrsAssoc->AddExport(nameHitGroup);
                        lrsAssoc->SetSubobjectToAssociate(*localRootSig);
                    }
                }
            }
        }

        //Shader Config
        {
            auto subObj = subobjects.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
            subObj->Config(maxPayloadSize, maxAttributeSize);
        }

        //Pipeline Config
        {
            auto subObj = subobjects.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
            subObj->Config(maxRecursionDepth);
        }

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