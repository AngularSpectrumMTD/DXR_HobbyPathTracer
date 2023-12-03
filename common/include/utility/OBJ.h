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

	//共通のマテリアルを持つ頂点
	struct MATERIAL {
		//cpu
		string MaterialName;
		Reflection4 Reflection4Color;
		f32 Shininess;
		string TextureName;
		utility::TextureResource DiffuseTexture;
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
	};

	class OBJ_MODEL {
	protected:
		bool LoadMaterialFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName);
		vector <MATERIAL> MaterialTbl;
		void CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, MATERIAL& mat, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName);

		void CreateBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device, MATERIAL& mat, const wchar_t* blaslNamePtr);
	public:
		vector<MATERIAL> getMaterialList() const;
		OBJ_MODEL();
		OBJ_MODEL(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr);
		bool OBJ_Load(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* folderPath, const char* FileName, const wchar_t* modelNamePtr);
		u32 getTriangleVertexTblCount();
		u32 getQuadrangleVertexTblCount();
		u32 getTriangleVertexIDTblCount();
		u32 getTriangleNormalIDTblCount();
		u32 getTriangleUVIDTblCount();
		u32 getQuadrangleVertexIDTblCount();
		u32 getQuadrangleNormalIDTblCount();
		u32 getQuadrangleUVIDTblCount();

		void CreateMeshBuffers(std::unique_ptr<dx12::RenderDeviceDX12>& device, const wchar_t* modelNamePtr);
		void CreateBLASs(std::unique_ptr<dx12::RenderDeviceDX12>& device);
	};
}

#endif
