#ifndef __OBJ_H__
#define __OBJ_H__

#include <vector>
#include <string>
#include "Utility.h"

using namespace std;

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
	string MaterialName;
	Reflection4 Reflection4Color;
	f32 Shininess;
	string TextureName;
	utility::TextureResource DiffuseTexture;
	s32 TexID;
	vector <utility::VertexPNT> TrigonTbl;
	vector <utility::VertexPNT> TetragonTbl;
	vector <u32> TriangleVertexIDTbl;
	vector <u32> TriangleNormalIDTbl;
	vector <u32> TriangleUVIDTbl;
	vector <u32> QuadrangleVertexIDTbl;
	vector <u32> QuadrangleNormalIDTbl;
	vector <u32> QuadrangleUVIDTbl;
};

class OBJ_MODEL {
protected:
	bool LoadMaterialFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* FileName);
	vector <MATERIAL> Material;
public:
	OBJ_MODEL();
	OBJ_MODEL(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* FileName);
	bool OBJ_Load(std::unique_ptr<dx12::RenderDeviceDX12>& device, const char* FileName);
	u32 getTrigonTblCount();
	u32 getTetragonTblCount();
	u32 getTriangleVertexIDTblCount();
	u32 getTriangleNormalIDTblCount();
	u32 getTriangleUVIDTblCount();	
	u32 getQuadrangleVertexIDTblCount();
	u32 getQuadrangleNormalIDTblCount();
	u32 getQuadrangleUVIDTblCount();

	//template <typename T>
	//void CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, std::vector<T> vb, std::vector<u32> ib, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName)
	//{
	//	u32 ibStride = u32(sizeof(u32)), vbStride = u32(sizeof(T));
	//	const auto flags = D3D12_RESOURCE_FLAG_NONE;
	//	const auto heapType = D3D12_HEAP_TYPE_DEFAULT;

	//	auto vbsize = vbStride * vb.size();
	//	auto ibsize = ibStride * ib.size();
	//	vertexBuffer = device->CreateBuffer(vbsize, flags, D3D12_RESOURCE_STATE_COPY_DEST, heapType, vb.data(), vbName);
	//	indexBuffer = device->CreateBuffer(ibsize, flags, D3D12_RESOURCE_STATE_COPY_DEST, heapType, ib.data(), ibName);
	//	vertexCount = u32(vb.size());
	//	indexCount = u32(ib.size());
	//	vertexStride = vbStride;

	//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescVB{};
	//	srvDescVB.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	//	srvDescVB.Format = DXGI_FORMAT_UNKNOWN;
	//	srvDescVB.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	srvDescVB.Buffer.NumElements = vertexCount;
	//	srvDescVB.Buffer.FirstElement = 0;
	//	srvDescVB.Buffer.StructureByteStride = vbStride;

	//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescIB{};
	//	srvDescIB.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	//	srvDescIB.Format = DXGI_FORMAT_UNKNOWN;
	//	srvDescIB.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	srvDescIB.Buffer.NumElements = indexCount;
	//	srvDescIB.Buffer.FirstElement = 0;
	//	srvDescIB.Buffer.StructureByteStride = ibStride;

	//	descriptorVB = device->CreateShaderResourceView(vertexBuffer.Get(), &srvDescVB);
	//	descriptorIB = device->CreateShaderResourceView(indexBuffer.Get(), &srvDescIB);

	//	shaderName = shaderName;
	//}
};

#endif
