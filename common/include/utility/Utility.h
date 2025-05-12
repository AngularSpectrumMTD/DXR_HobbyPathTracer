#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <memory>
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <DirectXMath.h>

#include <string>
#include <vector>
#include <filesystem>
#include <functional>

#include "RenderDeviceDX12.h"

#include <DirectXTex.h>

namespace dx12 {
    class RenderDeviceDX12;
}

namespace utility {
    using Microsoft::WRL::ComPtr;
    using namespace DirectX;

    inline u32 roundUpPow2(s32 n)
    {
        if (n <= 0) return 0;
        if ((n & (n - 1)) == 0) return (u32)n;
        u32 ret = 1;
        while (n > 0) { ret <<= 1; n >>= 1; }
        return ret;
    }

    inline std::wstring StringToWString(std::string str)
    {
        const int bufSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, (wchar_t*)NULL, 0);
        wchar_t* WCSPtr = new wchar_t[bufSize];

        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, WCSPtr, bufSize);

        std::wstring wstr(WCSPtr, WCSPtr + bufSize - 1);
        delete[] WCSPtr;
        return wstr;
    }

    inline u32 RoundUp(size_t size, u32 align) {
        return u32(size + align - 1) & ~(align-1);
    }

    struct TextureResource
    {
        ComPtr<ID3D12Resource> res;
        dx12::Descriptor srv;
    };

    class RootSignatureCreater {
    public:
        enum class RootType {
            CBV = D3D12_ROOT_PARAMETER_TYPE_CBV,
            SRV = D3D12_ROOT_PARAMETER_TYPE_SRV,
            UAV = D3D12_ROOT_PARAMETER_TYPE_UAV,
        };
        enum class RangeType {
            SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            Sampler = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
        };
        enum class AddressMode {
            Wrap = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            Mirror = D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
            Clamp = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            Border = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            MirrorOnce = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
        };

        void Push(
            const RootType type,
            const u32 shaderRegister,
            const u32 registerSpace = 0
        );

        void Push(
            const RangeType type,
            const u32 shaderRegister,
            const u32 registerSpace = 0,
            const u32 descriptorCount = 1
        );

        void PushStaticSampler(
            const u32 shaderRegister,
            const u32 registerSpace = 0,
            const D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            const AddressMode addressingModeU = AddressMode::Wrap,
            const AddressMode addressingModeV = AddressMode::Wrap,
            const AddressMode addressingModeW = AddressMode::Wrap
        );

        void Clear();

        ComPtr<ID3D12RootSignature> Create(
            std::unique_ptr<dx12::RenderDeviceDX12>& device,
            bool isLocalRoot, const wchar_t* name);
    private:
        std::vector<D3D12_ROOT_PARAMETER> mParams;
        std::vector<D3D12_DESCRIPTOR_RANGE> mRanges;
        std::vector<u32> mRangeLocation;
        std::vector<D3D12_STATIC_SAMPLER_DESC> mSamplers;
    };

    std::vector<char> CompileShaderAtRuntime(const std::filesystem::path& shaderFile);

    void getTexMetadata(DirectX::TexMetadata& metadata, const std::wstring& fileName, bool & isSuccess, bool isNoExeption = false);
    utility::TextureResource LoadTextureFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& device, const std::wstring& fileName, bool isNoExeption = false);
    HRESULT ReadDataFromFile(LPCWSTR filename, byte** dataPtr, u32* sizePtr);

    struct VertexPN {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
    };
    struct VertexPNC {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT4 Color;
    };
    struct VertexPNT {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 UV;
    };

    struct PolygonMesh {
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;

        dx12::Descriptor descriptorVB;
        dx12::Descriptor descriptorIB;

        u32 vertexCount = 0;
        u32 indexCount = 0;
        u32 vertexStride = 0;

        ComPtr<ID3D12Resource> blas;

        std::wstring shaderName;
        XMMATRIX mtxWorld = DirectX::XMMatrixIdentity();

        template <typename T>
        void CreateMeshBuffer(std::unique_ptr<dx12::RenderDeviceDX12>& device, std::vector<T> vb, std::vector<u32> ib, const wchar_t* vbName, const wchar_t* ibName, const wchar_t* shaderName)
        {
            u32 ibStride = u32(sizeof(u32)), vbStride = u32(sizeof(T));
            const auto flags = D3D12_RESOURCE_FLAG_NONE;
            const auto heapType = D3D12_HEAP_TYPE_DEFAULT;

            auto vbsize = vbStride * vb.size();
            auto ibsize = ibStride * ib.size();
            vertexBuffer = device->CreateBuffer(vbsize, flags, D3D12_RESOURCE_STATE_COPY_DEST, heapType, vb.data(), vbName);
            indexBuffer = device->CreateBuffer(ibsize, flags, D3D12_RESOURCE_STATE_COPY_DEST, heapType, ib.data(), ibName);
            vertexCount = u32(vb.size());
            indexCount = u32(ib.size());
            vertexStride = vbStride;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDescVB{};
            srvDescVB.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDescVB.Format = DXGI_FORMAT_UNKNOWN;
            srvDescVB.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDescVB.Buffer.NumElements = vertexCount;
            srvDescVB.Buffer.FirstElement = 0;
            srvDescVB.Buffer.StructureByteStride = vbStride;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDescIB{};
            srvDescIB.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDescIB.Format = DXGI_FORMAT_UNKNOWN;
            srvDescIB.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDescIB.Buffer.NumElements = indexCount;
            srvDescIB.Buffer.FirstElement = 0;
            srvDescIB.Buffer.StructureByteStride = ibStride;

            descriptorVB = device->CreateShaderResourceView(vertexBuffer.Get(), &srvDescVB);
            descriptorIB = device->CreateShaderResourceView(indexBuffer.Get(), &srvDescIB);

            shaderName = shaderName;
        }

        void CreateBLAS(std::unique_ptr<dx12::RenderDeviceDX12>& device, const wchar_t* namePtr)
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
                triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
                triangles.VertexBuffer.StrideInBytes = vertexStride;
                triangles.VertexCount = vertexCount;
                triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress();
                triangles.IndexCount = indexCount;
                triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
            }

            inputs.NumDescs = 1;
            inputs.pGeometryDescs = &geomDesc;
            ASBuffer = device->CreateAccelerationStructure(asDesc);
            ASBuffer.ASBuffer->SetName(namePtr);
            asDesc.ScratchAccelerationStructureData = ASBuffer.scratchBuffer->GetGPUVirtualAddress();
            asDesc.DestAccelerationStructureData = ASBuffer.ASBuffer->GetGPUVirtualAddress();
            command->BuildRaytracingAccelerationStructure(
                &asDesc, 0, nullptr);

            std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
            uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(ASBuffer.ASBuffer.Get()));
            command->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());
            command->Close();
            device->ExecuteCommandList(command);

            blas = ASBuffer.ASBuffer;
            device->WaitForCompletePipe();
        }
    };

    void ONB(const XMFLOAT3 normal, XMFLOAT3& tangent, XMFLOAT3& bitangent);

    void CreatePlane(std::vector<VertexPN>& vertices, std::vector<u32>& indices, f32 size = 10.f);
    void CreatePlane(std::vector<VertexPNC>& vertices, std::vector<u32>& indices, f32 size = 10.f);
    void CreatePlane(std::vector<VertexPNT>& vertices, std::vector<u32>& indices, f32 size = 10.f);

    void CreateCube(std::vector<VertexPN>& vertices, std::vector<u32>& indices, f32 sizex = 1.0f, f32 sizey = 1.0f, f32 sizez = 1.0f);
    void CreateOpenedCube(std::vector<VertexPNT>& vertices, std::vector<u32>& indices, f32 size = 1.0f);

    void CreateSphere(std::vector<VertexPN>& vertices, std::vector<u32>& indices, f32 radius = 1.0f, int slices = 16, int stacks = 24);
    void CreateSphere(std::vector<VertexPNT>& vertices, std::vector<u32>& indices, f32 radius = 1.0f, int slices = 16, int stacks = 24);

    void Split(char splitChar, char* bufferPtr, std::vector<std::string>& out);
    void VertexParser(std::vector<XMFLOAT3>& data, char* buffPtr);
    void FacetParser(std::vector<VertexPN>& outVertices, std::vector<u32>& indices, std::vector<XMFLOAT3>& vertices, std::vector<XMFLOAT3>& normals, char* bufferPtr);
    void SlashParser(int* listPtr, char* bufferPtr);
    void Replace(char searchChar, char replaceChar, char* bufferPtr);
    bool CreateMesh(const char* fileNamePtr, std::vector<VertexPN>& outVertices, std::vector<u32>& indices, XMFLOAT3 scale);
}

#endif