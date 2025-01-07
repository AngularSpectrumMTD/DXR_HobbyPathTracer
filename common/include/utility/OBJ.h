#ifndef __OBJ_H__
#define __OBJ_H__

#include <vector>
#include <string>
#include "Utility.h"

using namespace std;

namespace utility {
	struct Reflection4 {
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 emission;
		DirectX::XMFLOAT4 specular;
	};

	struct MaterialParam
	{
		DirectX::XMVECTOR albedo;
		f32 metallic;
		f32 roughness;
		f32 specular;
		f32 transRatio;
		DirectX::XMVECTOR transColor;
		DirectX::XMVECTOR emission;
		u32 isSSSExecutable = 0;
		u32 hasDiffuseTex : 1;
		u32 hasAlphaMask : 1;
		u32 materialBitsReserved : 30;

		//ctor
		MaterialParam()
		{
			hasDiffuseTex = 0;
			hasAlphaMask = 0;
		}

		void asDefault()
		{
			albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
			metallic = 0;//blend diffuse specular at specTrans == 0
			roughness = 0;
			specular = 0;//spec power
			transRatio = 1;//0:diffuse  1:trans
			transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
			emission = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			isSSSExecutable = 0u;
		}

		void asDefaultSSS()
		{
			albedo = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
			metallic = 0.0;//blend diffuse specular at specTrans == 0
			roughness = 1.0f;
			specular = 0.0;//spec power
			transRatio = 0;//0:diffuse  1:trans
			transColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
			emission = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			isSSSExecutable = 1u;
		}
	};

	struct UV {
		f32 u;
		f32 v;
	};

	struct vec4i {
		s32 x;
		s32 y;
		s32 z;
		s32 w;
	};

	struct OBJMaterial {
		//cpu
		string MaterialName;
		Reflection4 Reflection4Color;
		f32 Shininess;
		f32 Ni;
		f32 opacity;
		string DiffuseTextureName;
		string AlphaMaskName;
		utility::TextureResource DiffuseTexture;
		utility::TextureResource AlphaMask;
		s32 TexID;
		vector <utility::VertexPNT> TriangleVertexTbl;
		vector <utility::VertexPNT> QuadrangleVertexTbl;
		vector <u32> TriangleVertexIDTbl;
		vector <u32> TriangleNormalIDTbl;
		vector <u32> TriangleUVIDTbl;
		vector <u32> QuadrangleVertexIDTbl;
		vector <u32> QuadrangleNormalIDTbl;
		vector <u32> QuadrangleUVIDTbl;

		//gpu
		u32 TriangleVertexCount = 0;
		u32 TriangleIndexCount = 0;
		u32 TriangleVertexStride = 0;
		ComPtr<ID3D12Resource> TriangleVertexBuffer;
		ComPtr<ID3D12Resource> TriangleIndexBuffer;
		dx12::Descriptor descriptorTriangleVB;
		dx12::Descriptor descriptorTriangleIB;
		std::wstring shaderName;
		ComPtr<ID3D12Resource> blas;
		ComPtr<ID3D12Resource> materialCB;

		bool hasDiffuseTex = false;
		bool hasAlphaMask = false;

		void setDummyDiffuseTexture(std::unique_ptr<dx12::RenderDeviceDX12>& device);
		void setDummyAlphaMask(std::unique_ptr<dx12::RenderDeviceDX12>& device);
	};

	class OBJMaterialLinkedMesh {
	protected:
		bool LoadMaterialFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName);
		vector <OBJMaterial> MaterialTbl;
		bool CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, OBJMaterial& mat, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName);

		void CreateMaterialwiseBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device, OBJMaterial& mat, const wchar_t* blaslNamePtr);
	public:
		vector<OBJMaterial> getMaterialList() const;
		OBJMaterialLinkedMesh();
		OBJMaterialLinkedMesh(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr);
		bool loadObjFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr);
		u32 getTriangleVertexTblCount();
		u32 getQuadrangleVertexTblCount();
		u32 getTriangleVertexIDTblCount();
		u32 getTriangleNormalIDTblCount();
		u32 getTriangleUVIDTblCount();
		u32 getQuadrangleVertexIDTblCount();
		u32 getQuadrangleNormalIDTblCount();
		u32 getQuadrangleUVIDTblCount();

		void CreateMeshBuffers(std::unique_ptr<dx12::RenderDeviceDX12>& device, const wchar_t* modelNamePtr);
		void CreateBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device);
	};
}

#endif
