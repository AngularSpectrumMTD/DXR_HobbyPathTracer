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
	};

	class OBJ_MODEL {
	protected:
		bool LoadMaterialFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* FileName);
		vector <MATERIAL> Material;
		void CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, MATERIAL& mat, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName)
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

		void CreateBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device, MATERIAL& mat, const wchar_t* blaslNamePtr);
	public:
		OBJ_MODEL();
		OBJ_MODEL(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* FileName, const wchar_t* modelNamePtr);
		bool OBJ_Load(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* FileName, const wchar_t* modelNamePtr);
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
