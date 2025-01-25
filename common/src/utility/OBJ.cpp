#include "utility/OBJ.h"
#include <codecvt>
#include <fstream>

#define PREPARE_ELEMENTS 10000
#define MIN_ROUGHNESS 0.01f

bool isSameWord(const char* v0, const char* v1)
{
	return strcmp(v0, v1) == 0;
}

namespace utility {
	OBJMaterialLinkedMesh::OBJMaterialLinkedMesh() {
	}
	OBJMaterialLinkedMesh::OBJMaterialLinkedMesh(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr) {
		loadObjFile(device, folderPath, FileName, modelNamePtr);
	}

	bool OBJMaterialLinkedMesh::loadObjFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr) {
		vector <DirectX::XMFLOAT3> Vertex;
		vector <DirectX::XMFLOAT3> Normal;
		vector <DirectX::XMFLOAT2> uv;

		s32 matID = 0;
		char key[255] = { 0 };

		char fileName[60];
		sprintf_s(fileName, "%s/%s", folderPath, FileName);

		errno_t err;

		DirectX::XMFLOAT3 vec3d;
		DirectX::XMFLOAT2 vec2d;

		u32 vertexCounter = 0;
		u32 normalCounter = 0;
		u32 uvCounter = 0;

		//1st time File Reading
		{
			ifstream ifs(fileName);
			if (ifs.fail())
			{
				OutputDebugString(L"loadObjFile() File Open ERROR\n");
				return false;
			}

			string bufferStr;

			//File Read Start
			while (getline(ifs, bufferStr))
			{
				//key mtlib / v / vn etc...
				memset(key, 0, sizeof(char) * 255);
				sscanf_s(bufferStr.data(), "%s ", key, sizeof(key));

				if (isSameWord(key, "v")) {
					vertexCounter++;
				}
				if (isSameWord(key, "vn")) {
					normalCounter++;
				}
				if (isSameWord(key, "vt")) {
					uvCounter++;
				}
			}

			ifs.close();
		}

		//intermediate buffer construction
		{
			Vertex.resize(vertexCounter);
			Normal.resize(normalCounter);
			uv.resize(uvCounter);
		}

		//reset index
		vertexCounter = 0;
		normalCounter = 0;
		uvCounter = 0;

		//2nd time File Reading
		{
			FILE* fp = NULL;
			fopen_s(&fp, fileName, "rt");
			fseek(fp, SEEK_SET, 0);

			//File Read Start
			while (!feof(fp))
			{
				//key mtlib / v / vn etc...
				memset(key, 0, sizeof(char) * 255);
				fscanf_s(fp, "%s ", key, sizeof(key));

				if (isSameWord(key, "mtllib")) {
					fscanf_s(fp, "%s ", key, sizeof(key));
					LoadMaterialFromFile(device, folderPath, key);
				}
				if (isSameWord(key, "v")) {
					fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
					Vertex[vertexCounter] = vec3d;
					vertexCounter++;
				}
				if (isSameWord(key, "vn")) {
					fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
					Normal[normalCounter] = vec3d;
					normalCounter++;
				}
				if (isSameWord(key, "vt")) {
					fscanf_s(fp, "%f %f", &vec2d.x, &vec2d.y);
					vec2d.y *= -1;
					uv[uvCounter] = vec2d;
					uvCounter++;
				}
				if (isSameWord(key, "usemtl")) {
					fscanf_s(fp, "%s ", key, sizeof(key));
					for (s32 i = 0; i < (signed)MaterialTbl.size(); i++) {
						if (isSameWord(key, MaterialTbl[i].MaterialName.c_str()))
						{
							matID = i;
						}
					}
				}
				//face id0=vertex 1=UV 2=normal
				if (isSameWord(key, "f")) {
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
							//block x 3
							for (s32 i = 0; i < 3; i++)
							{
								SlashParser(vtx_uv_nrmID, (char*)spaceSplit.at(i).c_str());
								//vertexID uvID normalID
								for (s32 j = 0; j < 3; j++)
								{
									s32 currID = vtx_uv_nrmID[j];

									if (currID == -1)
									{
										continue;
									}

									switch (j)
									{
									case 0:
									{
										MaterialTbl[matID].TriangleVertexIDTbl.push_back(currID);
									}
									break;
									case 1:
									{
										MaterialTbl[matID].TriangleUVIDTbl.push_back(currID);
									}
									break;
									case 2:
									{
										MaterialTbl[matID].TriangleNormalIDTbl.push_back(currID);
									}
									break;
									}
								}
							}
						}
						else if (spaceSplit.size() == 4)//Rectangle
						{
							OutputDebugString(L"Invalid Vertex. This Program Handles Triangular Vertex Only.\n");
							//return true;
							//for (s32 i = 0; i < 4; i++)
							//{
							//	SlashParser(vtx_uv_nrmID, (char*)spaceSplit.at(i).c_str());
							//	//vertexID uvID normalID
							//	for (s32 i = 0; i < 3; i++)
							//	{
							//		s32 currID = vtx_uv_nrmID[i];

							//		if (currID == -1)
							//		{
							//			continue;
							//		}

							//		switch (i)
							//		{
							//		case 0:
							//			MaterialTbl[matID].QuadrangleVertexIDTbl.push_back(currID);
							//			break;
							//		case 1:
							//			MaterialTbl[matID].QuadrangleUVIDTbl.push_back(currID);
							//			break;
							//		case 2:
							//			MaterialTbl[matID].QuadrangleNormalIDTbl.push_back(currID);
							//			break;
							//		}
							//	}
							//}
						}
						else
						{
							OutputDebugString(L"Invalid Vertex. This Program Handles Triangular Vertex Only.\n");
							//return true;
						}
					}

				}
			}
			//finish 2nd time File Reading
			fclose(fp);
			//File Read End
		}

		//Construct Vertices, Normals, uv
		{
			for (s32 j = 0; j < (signed)MaterialTbl.size(); j++) {
				MaterialTbl[j].TriangleVertexTbl.resize(MaterialTbl[j].TriangleVertexIDTbl.size());
				MaterialTbl[j].QuadrangleVertexTbl.resize(MaterialTbl[j].QuadrangleVertexIDTbl.size());

				//Triangles
				for (s32 i = 0; i < (signed)MaterialTbl[j].TriangleVertexIDTbl.size(); i++) {
					utility::VertexPNT Tri;
					Tri.Position = Vertex[MaterialTbl[j].TriangleVertexIDTbl[i]];
					Tri.Normal = Normal[MaterialTbl[j].TriangleNormalIDTbl[i]];
					if (uv.size() > 0 && MaterialTbl[j].TriangleUVIDTbl.size() > 0 && (i < MaterialTbl[j].TriangleUVIDTbl.size()))
					{
						Tri.UV = uv[MaterialTbl[j].TriangleUVIDTbl[i]];
					}
					else
					{
						MaterialTbl[j].hasDiffuseTex = false;
						MaterialTbl[j].hasAlphaMask = false;
						Tri.UV = XMFLOAT2(0xff, 0xff);
					}
					MaterialTbl[j].TriangleVertexTbl[i] = Tri;
				}

				//Quadrangles
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
						MaterialTbl[j].hasDiffuseTex = false;
						MaterialTbl[j].hasAlphaMask = false;
						Quad.UV = XMFLOAT2(0xff, 0xff);
					}
					MaterialTbl[j].QuadrangleVertexTbl[i] = Quad;
				}
			}
		}
	
		//Construct ID (No Indexing)
		{
			for (s32 j = 0; j < (signed)MaterialTbl.size(); j++) {

				if (MaterialTbl[j].TriangleVertexIDTbl.size() > 0)
				{
					MaterialTbl[j].TriangleVertexIDTbl.clear();
					MaterialTbl[j].TriangleVertexIDTbl.resize(MaterialTbl[j].TriangleVertexTbl.size());
					MaterialTbl[j].TriangleNormalIDTbl.clear();
					MaterialTbl[j].TriangleNormalIDTbl.resize(MaterialTbl[j].TriangleVertexTbl.size());
					MaterialTbl[j].TriangleUVIDTbl.clear();
					MaterialTbl[j].TriangleUVIDTbl.resize(MaterialTbl[j].TriangleVertexTbl.size());
				}

				if (MaterialTbl[j].QuadrangleVertexIDTbl.size() > 0)
				{
					MaterialTbl[j].QuadrangleVertexIDTbl.clear();
					MaterialTbl[j].QuadrangleVertexIDTbl.resize(MaterialTbl[j].QuadrangleVertexTbl.size());
					MaterialTbl[j].QuadrangleNormalIDTbl.clear();
					MaterialTbl[j].QuadrangleNormalIDTbl.resize(MaterialTbl[j].QuadrangleVertexTbl.size());
					MaterialTbl[j].QuadrangleUVIDTbl.clear();
					MaterialTbl[j].QuadrangleUVIDTbl.resize(MaterialTbl[j].QuadrangleVertexTbl.size());
				}

				for (s32 i = 0; i < (signed)MaterialTbl[j].TriangleVertexTbl.size(); i++) {
					MaterialTbl[j].TriangleVertexIDTbl[i] = i;
					MaterialTbl[j].TriangleNormalIDTbl[i] = i;
					MaterialTbl[j].TriangleUVIDTbl[i] = i;
				}

				for (s32 i = 0; i < (signed)MaterialTbl[j].QuadrangleVertexTbl.size(); i++) {
					MaterialTbl[j].QuadrangleVertexIDTbl[i] = i;
					MaterialTbl[j].QuadrangleNormalIDTbl[i] = i;
					MaterialTbl[j].QuadrangleUVIDTbl[i] = i;
				}
			}
		}

		Vertex.clear();
		Normal.clear();
		uv.clear();

		return true;
	}

	vector<OBJMaterial> OBJMaterialLinkedMesh::getMaterialList() const
	{
		return MaterialTbl;
	}

	void OBJMaterial::setDummyDiffuseTexture(std::unique_ptr<dx12::RenderDeviceDX12>& device)
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

	void OBJMaterial::setDummyAlphaMask(std::unique_ptr<dx12::RenderDeviceDX12>& device)
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

	bool OBJMaterialLinkedMesh::LoadMaterialFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName) {
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

		bool isMaterialIncluded = false;
		bool isTextureIncluded = false;

		{
			char key[512] = { 0 };
			DirectX::XMFLOAT4 vec4d;
			vec4d.x = 0.0f;
			vec4d.y = 0.0f;
			vec4d.z = 0.0f;
			vec4d.w = 0.0f;
			OBJMaterial tempMaterial;

			tempMaterial.Reflection4Color.emission = vec4d;
			tempMaterial.Reflection4Color.ambient = vec4d;
			tempMaterial.Reflection4Color.diffuse = vec4d;
			tempMaterial.Reflection4Color.specular = vec4d;

			tempMaterial.Shininess = 0.0f;
			tempMaterial.TexID = 0;

			if (isFileValid)
			{
				fseek(fp, SEEK_SET, 0);

				while (!feof(fp))
				{
					//key newmtl / Ka / Kd etc...
					fscanf_s(fp, "%s ", key, sizeof(key));
					if (isSameWord(key, "newmtl"))
					{
						if (isMaterialIncluded)
						{
							MaterialTbl.push_back(tempMaterial);
							tempMaterial.TexID = 0;
						}
						isMaterialIncluded = true;
						fscanf_s(fp, "%s ", key, sizeof(key));
						tempMaterial.MaterialName = key;
						isTextureIncluded = false;
						tempMaterial.DiffuseTextureName = "";
						tempMaterial.AlphaMaskName = "";
					}
					if (isSameWord(key, "Ka"))
					{
						fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
						tempMaterial.Reflection4Color.ambient = vec4d;
					}
					if (isSameWord(key, "Kd"))
					{
						fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
						tempMaterial.Reflection4Color.diffuse = vec4d;
					}
					if (isSameWord(key, "Ks"))
					{
						fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
						tempMaterial.Reflection4Color.specular = vec4d;
					}
					if (isSameWord(key, "Ke"))
					{
						fscanf_s(fp, "%f %f %f", &vec4d.x, &vec4d.y, &vec4d.z);
						tempMaterial.Reflection4Color.emission = vec4d;
					}
					if (isSameWord(key, "Ns"))
					{
						fscanf_s(fp, "%f", &vec4d.x);
						tempMaterial.Shininess = vec4d.x;
					}
					if (isSameWord(key, "Ni"))
					{
						fscanf_s(fp, "%f", &vec4d.x);
						tempMaterial.Ni = vec4d.x;
					}
					if (isSameWord(key, "d"))
					{
						fscanf_s(fp, "%f", &vec4d.x);
						tempMaterial.opacity = vec4d.x;
					}
					if (isSameWord(key, "map_Kd"))
					{
						//fscanf_s(fp, "%s ", key, sizeof(key));
						fgets(key, 512, fp);
						char* p;
						p = strchr(key, '\n');
						if (p)
						{
							*p = '\0';
						}
						tempMaterial.DiffuseTextureName = key;
					}
					if (isSameWord(key, "map_d"))
					{
						//fscanf_s(fp, "%s ", key, sizeof(key));
						fgets(key, 512, fp);
						char* p;
						p = strchr(key, '\n');
						if (p)
						{
							*p = '\0';
						}
						tempMaterial.AlphaMaskName = key;
					}
				}

				fclose(fp);
			}

			tempMaterial.TriangleVertexIDTbl.reserve(PREPARE_ELEMENTS);
			tempMaterial.TriangleNormalIDTbl.reserve(PREPARE_ELEMENTS);
			tempMaterial.TriangleUVIDTbl.reserve(PREPARE_ELEMENTS);
			if (isMaterialIncluded)
			{
				MaterialTbl.emplace_back(tempMaterial);
			}
			else
			{
				tempMaterial.MaterialName = "dummyMTL";
				tempMaterial.Reflection4Color.diffuse = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
				tempMaterial.Reflection4Color.ambient = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
				tempMaterial.Reflection4Color.emission = DirectX::XMFLOAT4(0.0, 0.0, 0.0, 0.0);
				tempMaterial.Reflection4Color.specular = DirectX::XMFLOAT4(0.5, 0.5, 0.5, 0.5);
				tempMaterial.Shininess = 0;
				tempMaterial.Ni = 1.45;
				tempMaterial.opacity = 1.0f;
				tempMaterial.hasDiffuseTex = false;
				tempMaterial.hasAlphaMask = false;
				tempMaterial.setDummyDiffuseTexture(device);
				tempMaterial.setDummyAlphaMask(device);
				MaterialTbl.emplace_back(tempMaterial);
			}
		}

		int count = 0;
		for (auto & cpuMaterial : MaterialTbl)
		{
			if (isSameWord(cpuMaterial.DiffuseTextureName.c_str(), ""))
			{
				cpuMaterial.hasDiffuseTex = false;
				cpuMaterial.setDummyDiffuseTexture(device);
			}
			else
			{
				if (
					cpuMaterial.Reflection4Color.diffuse.x == 0 &&
					cpuMaterial.Reflection4Color.diffuse.y == 0 &&
					cpuMaterial.Reflection4Color.diffuse.z == 0 &&
					cpuMaterial.Reflection4Color.diffuse.w == 0
					)
				{
					cpuMaterial.Reflection4Color.diffuse = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
				}
				wchar_t nameTex[512];
				swprintf(nameTex, 512, L"%ls/%ls", StringToWString(folderPath).c_str(), StringToWString(cpuMaterial.DiffuseTextureName).c_str());

				cpuMaterial.hasDiffuseTex = true;
				cpuMaterial.DiffuseTexture = utility::LoadTextureFromFile(device, nameTex, true);

				if (cpuMaterial.DiffuseTexture.res == nullptr)
				{
					cpuMaterial.hasDiffuseTex = false;
					cpuMaterial.setDummyDiffuseTexture(device);
				}
			}

			if (isSameWord(cpuMaterial.AlphaMaskName.c_str(), ""))
			{
				cpuMaterial.hasAlphaMask = false;
				cpuMaterial.setDummyAlphaMask(device);
			}
			else
			{
				if (
					cpuMaterial.Reflection4Color.diffuse.x == 0 &&
					cpuMaterial.Reflection4Color.diffuse.y == 0 &&
					cpuMaterial.Reflection4Color.diffuse.z == 0 &&
					cpuMaterial.Reflection4Color.diffuse.w == 0
					)
				{
					cpuMaterial.Reflection4Color.diffuse = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
				}
				wchar_t nameTex[512];
				swprintf(nameTex, 512, L"%ls/%ls", StringToWString(folderPath).c_str(), StringToWString(cpuMaterial.AlphaMaskName).c_str());
				//cpuMaterial.DiffuseTexture = utility::LoadTextureFromFile(device, StringToWString(cpuMaterial.TextureName));
				cpuMaterial.hasAlphaMask = true;
				cpuMaterial.AlphaMask = utility::LoadTextureFromFile(device, nameTex, true);

				if (cpuMaterial.AlphaMask.res == nullptr)
				{
					cpuMaterial.hasAlphaMask = false;
					cpuMaterial.setDummyAlphaMask(device);
				}
			}

			count++;
		}

		return true;
	}

	bool OBJMaterialLinkedMesh::CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, OBJMaterial& mat, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName)
	{
		u32 ibStride = u32(sizeof(u32)), vbStride = u32(sizeof(utility::VertexPNT));
		const auto flags = D3D12_RESOURCE_FLAG_NONE;
		const auto heapType = D3D12_HEAP_TYPE_DEFAULT;

		//triangle only
		if (mat.TriangleVertexTbl.size() > 0 && mat.TriangleVertexIDTbl.size() > 0)
		{
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
			return true;
		}
		else
		{
			return false;
		}
	}

	void OBJMaterialLinkedMesh::CreateMeshBuffers(std::unique_ptr<dx12::RenderDeviceDX12>& device, const wchar_t* modelNamePtr)
	{
		u32 count = 0;

		vector<u32> validMaterialIDTbl;
		for (auto& cpuMaterial : MaterialTbl)
		{
			wchar_t nameVB[60];
			swprintf(nameVB, 60, L"VB : %ls %ls", modelNamePtr, StringToWString(cpuMaterial.MaterialName).c_str());

			wchar_t nameIB[60];
			swprintf(nameIB, 60, L"IB : %ls %ls", modelNamePtr, StringToWString(cpuMaterial.MaterialName).c_str());
			
			const bool isValidMat = CreateMeshBuffer(device, cpuMaterial, nameVB, nameIB, L"");

			if (isValidMat)
			{
				MaterialParam mparams;
				mparams.albedo = XMVectorSet(cpuMaterial.Reflection4Color.diffuse.x, cpuMaterial.Reflection4Color.diffuse.y, cpuMaterial.Reflection4Color.diffuse.z, cpuMaterial.Reflection4Color.diffuse.w);
				mparams.specular = (cpuMaterial.Reflection4Color.specular.x + cpuMaterial.Reflection4Color.specular.y + cpuMaterial.Reflection4Color.specular.z + cpuMaterial.Reflection4Color.specular.w) / 4.0f;
				mparams.metallic = mparams.specular;
				//https://blender.stackexchange.com/questions/149755/changing-the-specular-exponent-of-principled-bsdf-with-texture-when-exporting-to
				//based on Principled BSDF
				mparams.roughness = min(1, max(0, 1 - sqrt(cpuMaterial.Shininess) / 30));
				mparams.transRatio = ((cpuMaterial.Ni != 1.45) && (cpuMaterial.opacity != 1.0f)) ? 1.0f : 0.0f;
				mparams.emission = XMVectorSet(cpuMaterial.Reflection4Color.emission.x, cpuMaterial.Reflection4Color.emission.y, cpuMaterial.Reflection4Color.emission.z, cpuMaterial.Reflection4Color.emission.w);
				mparams.transColor = mparams.albedo;
				mparams.isSSSExecutable = 0;
				mparams.hasDiffuseTex = cpuMaterial.hasDiffuseTex ? 1 : 0;
				mparams.hasAlphaMask = cpuMaterial.hasAlphaMask ? 1 : 0;

				if (mparams.transRatio == 1)
				{
					mparams.roughness = 0;
				}

				if (std::strcmp(cpuMaterial.MaterialName.c_str(), "ReinterpretMirror") == 0)
				{
					mparams.metallic = 1;
					mparams.roughness = MIN_ROUGHNESS;
				}
				if (std::strcmp(cpuMaterial.MaterialName.c_str(), "ReinterpretGlass") == 0)
				{
					mparams.transRatio = 1;
					mparams.transColor = mparams.albedo;
					mparams.roughness = MIN_ROUGHNESS;
				}
				if (std::strcmp(cpuMaterial.MaterialName.c_str(), "ReinterpretRoughMirror") == 0)
				{
					mparams.metallic = 1;
				}
				if (std::strcmp(cpuMaterial.MaterialName.c_str(), "ReinterpretRoughGlass") == 0)
				{
					mparams.transRatio = 1;
					mparams.transColor = mparams.albedo;
				}

				mparams.roughness = max(mparams.roughness, MIN_ROUGHNESS);

				cpuMaterial.materialCB = device->CreateConstantBuffer(sizeof(MaterialParam));
				device->ImmediateBufferUpdateHostVisible(cpuMaterial.materialCB.Get(), &mparams, sizeof(MaterialParam));

				validMaterialIDTbl.push_back(count);
			}

			count++;
		}

		vector <OBJMaterial> validMaterialTbl(validMaterialIDTbl.size());

		u32 countValid = 0;
		for (auto& id : validMaterialIDTbl)
		{
			validMaterialTbl[countValid] = MaterialTbl[id];
			countValid++;
		}

		MaterialTbl = validMaterialTbl;
	}

	void OBJMaterialLinkedMesh::CreateMaterialwiseBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device, OBJMaterial& mat, const wchar_t* blaslNamePtr)
	{
		auto command = device->CreateCommandList();
		dx12::AccelerationStructureBuffers ASBuffer;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = asDesc.Inputs;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc{};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		//AnyHit Shaders are called on BVH which is not constructed with  the flag "D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE"
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
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

	void OBJMaterialLinkedMesh::CreateBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device)
	{
		for (auto& m : MaterialTbl)
		{
			wchar_t nameBLAS[60];
			swprintf(nameBLAS, 60, L"BLAS : %ls", StringToWString(m.MaterialName).c_str());

			CreateMaterialwiseBLAS(device, m, nameBLAS);
		}
	}


	u32 OBJMaterialLinkedMesh::getTriangleVertexTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleVertexTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getQuadrangleVertexTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleVertexTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getTriangleVertexIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleVertexIDTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getTriangleNormalIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleNormalIDTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getTriangleUVIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.TriangleUVIDTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getQuadrangleVertexIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleVertexIDTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getQuadrangleNormalIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleNormalIDTbl.size();
		}
		return count;
	}

	u32 OBJMaterialLinkedMesh::getQuadrangleUVIDTblCount()
	{
		u32 count = 0;
		for (auto& m : MaterialTbl)
		{
			count += m.QuadrangleUVIDTbl.size();
		}
		return count;
	}
}
