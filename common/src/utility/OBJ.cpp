#include "utility/OBJ.h"
#include <codecvt>

#define PREPARE_ELEMENTS 10000

namespace utility {
	OBJ_MODEL::OBJ_MODEL() {
	}
	OBJ_MODEL::OBJ_MODEL(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr) {
		OBJ_Load(device, folderPath, FileName, modelNamePtr);
	}

	bool OBJ_MODEL::OBJ_Load(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr) {
		vec4i Face[3];
		vector <DirectX::XMFLOAT3> Vertex;
		vector <DirectX::XMFLOAT3> Normal;
		vector <DirectX::XMFLOAT2> uv;

		Vertex.reserve(PREPARE_ELEMENTS);
		Normal.reserve(PREPARE_ELEMENTS);
		uv.reserve(PREPARE_ELEMENTS);

		s32 matID = 0;
		char key[255] = { 0 };

		char fileName[60];
		sprintf_s(fileName, "%s/%s", folderPath, FileName);

		FILE* fp = NULL;
		errno_t err;
		err = fopen_s(&fp, fileName, "rt");
		if (err == 0)
		{
			OutputDebugString(L"OBJ_Load() File Open Succeeded\n");
		}
		else if (err > 0)
		{
			wchar_t debugStr[256];
			swprintf_s(debugStr, L"OBJ_Load() File Open ERROR : code %d\n", err);
			OutputDebugString(debugStr);
			return false;
		}
		else
		{
			OutputDebugString(L"OBJ_Load() File Open ERROR\n");
			return false;
		}

		fseek(fp, SEEK_SET, 0);

		DirectX::XMFLOAT3 vec3d;
		DirectX::XMFLOAT2 vec2d;

		//File Read Start
		while (!feof(fp))
		{
			//key mtlib / v / vn etc...
			memset(key, 0, sizeof(char) * 255);
			fscanf_s(fp, "%s ", key, sizeof(key));

			if (strcmp(key, "mtllib") == 0) {
				fscanf_s(fp, "%s ", key, sizeof(key));
				LoadMaterialFromFile(device, folderPath, key);
			}
			if (strcmp(key, "v") == 0) {
				fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
				Vertex.push_back(vec3d);
			}
			if (strcmp(key, "vn") == 0) {
				fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
				Normal.push_back(vec3d);
			}
			if (strcmp(key, "vt") == 0) {
				fscanf_s(fp, "%f %f", &vec2d.x, &vec2d.y);
				//vec2d.x = 1 - vec2d.x;
				vec2d.y = 1 - vec2d.y;
				uv.push_back(vec2d);
			}
			if (strcmp(key, "usemtl") == 0) {
				fscanf_s(fp, "%s ", key, sizeof(key));
				for (s32 i = 0; i < (signed)MaterialTbl.size(); i++) {
					if (strcmp(key, MaterialTbl[i].MaterialName.c_str()) == 0)
					{
						matID = i;
					}
				}
			}
			//face id0=vertex 1=UV 2=normal
			if (strcmp(key, "f") == 0) {
				Face[0].x = -1; Face[0].y = -1; Face[0].z = -1; Face[0].w = -1;//VertexID
				Face[1].x = -1; Face[1].y = -1; Face[1].z = -1; Face[1].w = -1;//UVID
				Face[2].x = -1; Face[2].y = -1; Face[2].z = -1; Face[2].w = -1;//NormalID

				const s32 LineBufferLength = 1024;
				char buffer[LineBufferLength];

				if (fgets(buffer, LineBufferLength, fp) != nullptr)
				{
					char* parsePoint = strchr(buffer, ' ');
					std::vector<std::string> spaceSplit;
					Split(' ', &buffer[0], spaceSplit);
					s32 vtx_uv_nrmID[3] =
					{
						-1, -1, -1,
					};

					if (spaceSplit.size() == 3)//Triangle
					{
						for (s32 i = 0; i < 3; i++)
						{
							SlashParser(vtx_uv_nrmID, (char*)spaceSplit.at(i).c_str());
							//vertexID uvID normalID
							for (s32 i = 0; i < 3; i++)
							{
								s32 currID = vtx_uv_nrmID[i];

								if (currID == -1)
								{
									continue;
								}

								switch (i)
								{
									case 0:
										MaterialTbl[matID].TriangleVertexIDTbl.push_back(currID );
										break;
									case 1:
										MaterialTbl[matID].TriangleUVIDTbl.push_back(currID);
										break;
									case 2:
										MaterialTbl[matID].TriangleNormalIDTbl.push_back(currID);
										break;
								}
							}
						}
					}
					else if (spaceSplit.size() == 4)//Rectangle
					{
						for (s32 i = 0; i < 4; i++)
						{
							SlashParser(vtx_uv_nrmID, (char*)spaceSplit.at(i).c_str());
							//vertexID uvID normalID
							for (s32 i = 0; i < 3; i++)
							{
								s32 currID = vtx_uv_nrmID[i];

								if (currID == -1)
								{
									continue;
								}

								switch (i)
								{
								case 0:
									MaterialTbl[matID].QuadrangleVertexIDTbl.push_back(currID);
									break;
								case 1:
									MaterialTbl[matID].QuadrangleUVIDTbl.push_back(currID);
									break;
								case 2:
									MaterialTbl[matID].QuadrangleNormalIDTbl.push_back(currID);
									break;
								}
							}
						}
					}
					else
					{
						OutputDebugString(L"Invalid Vertex. This Program Handles Triangular Vertex Only.\n");
						return false;
					}
				}

			}
		}
		//File Read End

		for (s32 j = 0; j < (signed)MaterialTbl.size(); j++) {
			for (s32 i = 0; i < (signed)MaterialTbl[j].TriangleVertexIDTbl.size(); i++) {
				utility::VertexPNT Tri;
				Tri.Position = Vertex[MaterialTbl[j].TriangleVertexIDTbl[i]];
				Tri.Normal = Normal[MaterialTbl[j].TriangleNormalIDTbl[i]];
				if (uv.size() > 0)
				{
					Tri.UV = uv[MaterialTbl[j].TriangleUVIDTbl[i]];
				}
				else
				{
					Tri.UV = XMFLOAT2(0xff, 0xff);
				}
				MaterialTbl[j].TriangleVertexTbl.push_back(Tri);
			}
			for (s32 i = 0; i < (signed)MaterialTbl[j].QuadrangleVertexIDTbl.size(); i++) {
				utility::VertexPNT Quad;
				Quad.Position = Vertex[MaterialTbl[j].QuadrangleVertexIDTbl[i]];
				Quad.Normal = Normal[MaterialTbl[j].QuadrangleNormalIDTbl[i]];
				if (uv.size() > 0)
				{
					Quad.UV = uv[MaterialTbl[j].QuadrangleUVIDTbl[i]];
				}
				else
				{
					Quad.UV = XMFLOAT2(0xff, 0xff);
				}
				MaterialTbl[j].QuadrangleVertexTbl.push_back(Quad);
			}
		}

		//No Indexing
		for (s32 j = 0; j < (signed)MaterialTbl.size(); j++) {

			if (MaterialTbl[j].TriangleVertexIDTbl.size() > 0)
			{
				MaterialTbl[j].TriangleVertexIDTbl.clear();
				MaterialTbl[j].TriangleVertexIDTbl.reserve(MaterialTbl[j].TriangleVertexTbl.size()); 
				MaterialTbl[j].TriangleNormalIDTbl.clear();
				MaterialTbl[j].TriangleNormalIDTbl.reserve(MaterialTbl[j].TriangleVertexTbl.size());
				MaterialTbl[j].TriangleUVIDTbl.clear();
				MaterialTbl[j].TriangleUVIDTbl.reserve(MaterialTbl[j].TriangleVertexTbl.size());
			}

			if (MaterialTbl[j].QuadrangleVertexIDTbl.size() > 0)
			{
				MaterialTbl[j].QuadrangleVertexIDTbl.clear();
				MaterialTbl[j].QuadrangleVertexIDTbl.reserve(MaterialTbl[j].QuadrangleVertexTbl.size());
				MaterialTbl[j].QuadrangleNormalIDTbl.clear();
				MaterialTbl[j].QuadrangleNormalIDTbl.reserve(MaterialTbl[j].QuadrangleVertexTbl.size());
				MaterialTbl[j].QuadrangleUVIDTbl.clear();
				MaterialTbl[j].QuadrangleUVIDTbl.reserve(MaterialTbl[j].QuadrangleVertexTbl.size());
			}

			u32 count = 0;
			for (s32 i = 0; i < (signed)MaterialTbl[j].TriangleVertexTbl.size(); i++) {
				MaterialTbl[j].TriangleVertexIDTbl.push_back(count);
				MaterialTbl[j].TriangleNormalIDTbl.push_back(count);
				MaterialTbl[j].TriangleUVIDTbl.push_back(count);
				count++;
			}
			count = 0;
			for (s32 i = 0; i < (signed)MaterialTbl[j].QuadrangleVertexTbl.size(); i++) {
				MaterialTbl[j].QuadrangleVertexIDTbl.push_back(count);
				MaterialTbl[j].QuadrangleNormalIDTbl.push_back(count);
				MaterialTbl[j].QuadrangleUVIDTbl.push_back(count);
				count++;
			}
		}

		Vertex.clear();
		Normal.clear();
		uv.clear();
		return true;
	}

	vector<MATERIAL> OBJ_MODEL::getMaterialList() const
	{
		return MaterialTbl;
	}

	void MATERIAL::setDummyDiffuseTexture(std::unique_ptr<dx12::RenderDeviceDX12>& device)
	{
		DiffuseTextureName = "DUMMY_DIFFUSE";
		DiffuseTexture.res = device->CreateTexture2D(
			1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_HEAP_TYPE_DEFAULT,
			StringToWString(DiffuseTextureName).c_str()
		);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DiffuseTexture.srv = device->CreateShaderResourceView(DiffuseTexture.res.Get(), &srvDesc);
	}

	void MATERIAL::setDummyAlphaMask(std::unique_ptr<dx12::RenderDeviceDX12>& device)
	{
		AlphaMaskName = "DUMMY_ALPHA_MASK";
		AlphaMask.res = device->CreateTexture2D(
			1, 1, DXGI_FORMAT_R16_FLOAT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_HEAP_TYPE_DEFAULT,
			StringToWString(AlphaMaskName).c_str()
		);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		AlphaMask.srv = device->CreateShaderResourceView(AlphaMask.res.Get(), &srvDesc);
	}

	bool OBJ_MODEL::LoadMaterialFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName) {
		FILE* fp = NULL;
		char fileName[256];
		sprintf_s(fileName, "%s/%s", folderPath, FileName);
		errno_t err;
		err = fopen_s(&fp, fileName, "rt");

		bool isFileValid = false;
		if (err == 0)
		{
			OutputDebugString(L"LoadMaterialFromFile() File Open Succeeded\n");
			isFileValid = true;
		}
		else if (err > 0)
		{
			wchar_t debugStr[256];
			swprintf_s(debugStr, L"LoadMaterialFromFile() File Open ERROR : code %d\n", err);
			OutputDebugString(debugStr);
		}
		else
		{
			OutputDebugString(L"LoadMaterialFromFile() File Open ERROR\n");
		}

		char key[512] = { 0 };
		bool isMaterialIncluded = false;
		bool isTextureIncluded = false;
		DirectX::XMFLOAT4 vec4d;
		vec4d.x = 0.0f;
		vec4d.y = 0.0f;
		vec4d.z = 0.0f;
		vec4d.w = 0.0f;
		MATERIAL mtl;

		mtl.Reflection4Color.emission = vec4d;
		mtl.Reflection4Color.ambient = vec4d;
		mtl.Reflection4Color.diffuse = vec4d;
		mtl.Reflection4Color.specular = vec4d;

		mtl.Shininess = 0.0f;
		mtl.TexID = 0;

		if (isFileValid)
		{
			fseek(fp, SEEK_SET, 0);

			while (!feof(fp))
			{
				//key newmtl / Ka / Kd etc...
				fscanf_s(fp, "%s ", key, sizeof(key));
				if (strcmp(key, "newmtl") == 0)
				{
					if (isMaterialIncluded) 
					{ 
						MaterialTbl.push_back(mtl); 
						mtl.TexID = 0; 
					}
					isMaterialIncluded = true;
					fscanf_s(fp, "%s ", key, sizeof(key));
					mtl.MaterialName = key;
					isTextureIncluded = false;
					mtl.DiffuseTextureName = "";
					mtl.AlphaMaskName = "";
				}
				if (strcmp(key, "Ka") == 0)
				{
					fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
					mtl.Reflection4Color.ambient = vec4d;
				}
				if (strcmp(key, "Kd") == 0)
				{
					fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
					mtl.Reflection4Color.diffuse = vec4d;
				}
				if (strcmp(key, "Ks") == 0)
				{
					fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
					mtl.Reflection4Color.specular = vec4d;
				}
				if (strcmp(key, "Ns") == 0)
				{
					fscanf_s(fp, "%f", &vec4d.x);
					mtl.Shininess = vec4d.x;
				}
				if (strcmp(key, "map_Kd") == 0)
				{
					//fscanf_s(fp, "%s ", key, sizeof(key));
					fgets(key, 512, fp);
					char* p;
					p = strchr(key, '\n');
					if (p)
					{
						*p = '\0';
					}
					mtl.DiffuseTextureName = key;
				}
				if (strcmp(key, "map_d") == 0)
				{
					//fscanf_s(fp, "%s ", key, sizeof(key));
					fgets(key, 512, fp);
					char* p;
					p = strchr(key, '\n');
					if (p)
					{
						*p = '\0';
					}
					mtl.AlphaMaskName = key;
				}
			}

			fclose(fp);
		}

		if (isMaterialIncluded)
		{
			MaterialTbl.push_back(mtl);
		}
		else
		{
			mtl.MaterialName = "dummyMTL";
			mtl.Reflection4Color.diffuse = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
			mtl.Reflection4Color.ambient = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
			mtl.Reflection4Color.emission = DirectX::XMFLOAT4(0.0, 0.0, 0.0, 0.0);
			mtl.Reflection4Color.specular = DirectX::XMFLOAT4(0.5, 0.5, 0.5, 0.5);
			mtl.setDummyDiffuseTexture(device);
			mtl.setDummyAlphaMask(device);
			MaterialTbl.push_back(mtl);
		}

		int count = 0;
		for (auto & mat : MaterialTbl)
		{
			if (strcmp(mat.DiffuseTextureName.c_str(), "") == 0)
			{
				mat.setDummyDiffuseTexture(device);
			}
			else
			{
				if (
					mat.Reflection4Color.diffuse.x == 0 &&
					mat.Reflection4Color.diffuse.y == 0 &&
					mat.Reflection4Color.diffuse.z == 0 &&
					mat.Reflection4Color.diffuse.w == 0
					)
				{
					mat.Reflection4Color.diffuse = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
				}
				wchar_t nameTex[512];
				swprintf(nameTex, 512, L"%ls/%ls", StringToWString(folderPath).c_str(), StringToWString(mat.DiffuseTextureName).c_str());
				//mtl.DiffuseTexture = utility::LoadTextureFromFile(device, StringToWString(mtl.TextureName));
				mat.DiffuseTexture = utility::LoadTextureFromFile(device, nameTex, true);

				if (mat.DiffuseTexture.res == nullptr)
				{
					mat.setDummyDiffuseTexture(device);
				}
			}

			if (strcmp(mat.AlphaMaskName.c_str(), "") == 0)
			{
				mat.setDummyAlphaMask(device);
			}
			else
			{
				if (
					mat.Reflection4Color.diffuse.x == 0 &&
					mat.Reflection4Color.diffuse.y == 0 &&
					mat.Reflection4Color.diffuse.z == 0 &&
					mat.Reflection4Color.diffuse.w == 0
					)
				{
					mat.Reflection4Color.diffuse = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
				}
				wchar_t nameTex[512];
				swprintf(nameTex, 512, L"%ls/%ls", StringToWString(folderPath).c_str(), StringToWString(mat.AlphaMaskName).c_str());
				//mtl.DiffuseTexture = utility::LoadTextureFromFile(device, StringToWString(mtl.TextureName));
				mat.AlphaMask = utility::LoadTextureFromFile(device, nameTex, true);

				if (mat.AlphaMask.res == nullptr)
				{
					mat.setDummyAlphaMask(device);
				}
			}

			count++;
		}

		return true;
	}

	void OBJ_MODEL::CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, MATERIAL& mat, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName)
	{
		u32 ibStride = u32(sizeof(u32)), vbStride = u32(sizeof(utility::VertexPNT));
		const auto flags = D3D12_RESOURCE_FLAG_NONE;
		const auto heapType = D3D12_HEAP_TYPE_DEFAULT;

		auto vbsize = vbStride * mat.TriangleVertexTbl.size();
		auto ibsize = ibStride * mat.TriangleVertexIDTbl.size();
		mat.TriangleVertexBuffer = device->CreateBuffer(vbsize, flags, D3D12_RESOURCE_STATE_COPY_DEST, heapType, mat.TriangleVertexTbl.data(), vbName);
		mat.TriangleIndexBuffer = device->CreateBuffer(ibsize, flags, D3D12_RESOURCE_STATE_COPY_DEST, heapType, mat.TriangleVertexIDTbl.data(), ibName);
		mat.TriangleVertexCount = u32(mat.TriangleVertexTbl.size());
		mat.TriangleIndexCount = u32(mat.TriangleVertexIDTbl.size());
		mat.TriangleVertexStride = vbStride;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescVB{};
		srvDescVB.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDescVB.Format = DXGI_FORMAT_UNKNOWN;
		srvDescVB.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescVB.Buffer.NumElements = mat.TriangleVertexCount;
		srvDescVB.Buffer.FirstElement = 0;
		srvDescVB.Buffer.StructureByteStride = vbStride;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescIB{};
		srvDescIB.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDescIB.Format = DXGI_FORMAT_UNKNOWN;
		srvDescIB.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescIB.Buffer.NumElements = mat.TriangleIndexCount;
		srvDescIB.Buffer.FirstElement = 0;
		srvDescIB.Buffer.StructureByteStride = ibStride;

		mat.descriptorTriangleVB = device->CreateShaderResourceView(mat.TriangleVertexBuffer.Get(), &srvDescVB);
		mat.descriptorTriangleIB = device->CreateShaderResourceView(mat.TriangleIndexBuffer.Get(), &srvDescIB);

		mat.shaderName = shaderName;
	}

	void OBJ_MODEL::CreateMeshBuffers(std::unique_ptr<dx12::RenderDeviceDX12>& device, const wchar_t* modelNamePtr)
	{
		u32 count = 0;

		for (auto& m : MaterialTbl)
		{
			wchar_t nameVB[60];
			swprintf(nameVB, 60, L"VB : %ls %ls", modelNamePtr, StringToWString(m.MaterialName).c_str());

			wchar_t nameIB[60];
			swprintf(nameIB, 60, L"IB : %ls %ls", modelNamePtr, StringToWString(m.MaterialName).c_str());
			
			CreateMeshBuffer(device, m, nameVB, nameIB, L"");

			MaterialParam mparams;
			mparams.albedo = XMVectorSet(m.Reflection4Color.diffuse.x, m.Reflection4Color.diffuse.y, m.Reflection4Color.diffuse.z, m.Reflection4Color.diffuse.w);
			mparams.specular = (m.Reflection4Color.specular.x + m.Reflection4Color.specular.y + m.Reflection4Color.specular.z + m.Reflection4Color.specular.w) / 4.0f;
			mparams.metallic = mparams.specular;
			mparams.roughness = 1 - mparams.metallic;
			mparams.transRatio = 0;
			mparams.emission = XMVectorSet(m.Reflection4Color.emission.x, m.Reflection4Color.emission.y, m.Reflection4Color.emission.z, m.Reflection4Color.emission.w);
			m.materialCB = device->CreateConstantBuffer(sizeof(MaterialParam));
			device->ImmediateBufferUpdateHostVisible(m.materialCB.Get(), &mparams, sizeof(MaterialParam));

			count++;
		}
	}

	void OBJ_MODEL::CreateBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device, MATERIAL& mat, const wchar_t* blaslNamePtr)
	{
		auto command = device->CreateCommandList();
		dx12::AccelerationStructureBuffers ASBuffer;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = asDesc.Inputs;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc{};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		{
			auto& triangles = geomDesc.Triangles;
			triangles.VertexBuffer.StartAddress = mat.TriangleVertexBuffer->GetGPUVirtualAddress();
			triangles.VertexBuffer.StrideInBytes = mat.TriangleVertexStride;
			triangles.VertexCount = mat.TriangleVertexCount;
			triangles.IndexBuffer = mat.TriangleIndexBuffer->GetGPUVirtualAddress();
			triangles.IndexCount = mat.TriangleIndexCount;
			triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		}

		inputs.NumDescs = 1;
		inputs.pGeometryDescs = &geomDesc;
		ASBuffer = device->CreateAccelerationStructure(asDesc);
		ASBuffer.ASBuffer->SetName(blaslNamePtr);
		asDesc.ScratchAccelerationStructureData = ASBuffer.scratchBuffer->GetGPUVirtualAddress();
		asDesc.DestAccelerationStructureData = ASBuffer.ASBuffer->GetGPUVirtualAddress();
		command->BuildRaytracingAccelerationStructure(
			&asDesc, 0, nullptr);

		std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
		uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer.ASBuffer.Get()));
		command->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());
		command->Close();
		device->ExecuteCommandList(command);

		mat.blas = ASBuffer.ASBuffer;
		device->WaitForCompletePipe();
	}

	void OBJ_MODEL::CreateBLASs(std::unique_ptr<dx12::RenderDeviceDX12>& device)
	{
		for (auto& m : MaterialTbl)
		{
			wchar_t nameBLAS[60];
			swprintf(nameBLAS, 60, L"BLAS : %ls", StringToWString(m.MaterialName).c_str());

			CreateBLAS(device, m, nameBLAS);
		}
	}


	u32 OBJ_MODEL::getTriangleVertexTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleVertexTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getQuadrangleVertexTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleVertexTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getTriangleVertexIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleVertexIDTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getTriangleNormalIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleNormalIDTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getTriangleUVIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleUVIDTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getQuadrangleVertexIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleVertexIDTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getQuadrangleNormalIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleNormalIDTbl.size();
		}
		return count;
	}

	u32 OBJ_MODEL::getQuadrangleUVIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleUVIDTbl.size();
		}
		return count;
	}
}
