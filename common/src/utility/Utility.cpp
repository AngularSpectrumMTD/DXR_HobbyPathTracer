#include "utility/Utility.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <dxcapi.h>
#include "d3dx12.h"
#include <fstream>
#include <wincodec.h>
#include <DirectXTex.h>

#pragma comment(lib, "dxcompiler.lib")

using namespace DirectX;

namespace utility {
    void RootSignatureCreater::Push(const RootType type, const u32 shaderRegister, const u32 registerSpace)
    {
        D3D12_ROOT_PARAMETER rootParam{};
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParam.ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(type);
        rootParam.Descriptor.ShaderRegister = shaderRegister;
        rootParam.Descriptor.RegisterSpace = registerSpace;
        mParams.push_back(rootParam);
    }

    void RootSignatureCreater::Push(const RangeType type, const u32 shaderRegister, const u32 registerSpace, const u32 descriptorCount)
    {
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(type);
        range.NumDescriptors = descriptorCount;
        range.BaseShaderRegister = shaderRegister;
        range.RegisterSpace = registerSpace;
        range.OffsetInDescriptorsFromTableStart = 0;
        mRanges.push_back(range);

        // Range is resolved on "Create()"
        u32 rangeIndex = u32(mParams.size());
        mRangeLocation.push_back(rangeIndex);

        D3D12_ROOT_PARAMETER rootParam{};
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParam.DescriptorTable.NumDescriptorRanges = 1;
        rootParam.DescriptorTable.pDescriptorRanges = nullptr;
        mParams.push_back(rootParam);
    }

    void RootSignatureCreater::PushStaticSampler(const u32 shaderRegister, const u32 registerSpace, const D3D12_FILTER filter, const AddressMode addressingModeU, const AddressMode addressingModeV, const AddressMode addressingModeW)
    {
        CD3DX12_STATIC_SAMPLER_DESC desc;
        desc.Init(shaderRegister,
            filter,
            static_cast<D3D12_TEXTURE_ADDRESS_MODE>(addressingModeU),
            static_cast<D3D12_TEXTURE_ADDRESS_MODE>(addressingModeV),
            static_cast<D3D12_TEXTURE_ADDRESS_MODE>(addressingModeW)
        );
        desc.RegisterSpace = registerSpace;
        mSamplers.push_back(desc);
    }

    void RootSignatureCreater::Clear() {
        mParams.clear();
        mRanges.clear();
        mRangeLocation.clear();
        mSamplers.clear();
    }

    ComPtr<ID3D12RootSignature> RootSignatureCreater::Create(std::unique_ptr<dx12::RenderDeviceDX12>& device, bool isLocalRoot, const wchar_t* name)
    {
        for (u32 i = 0; i < u32(mRanges.size()); ++i) {
            auto index = mRangeLocation.at(i);
            mParams.at(index).DescriptorTable.pDescriptorRanges = &mRanges.at(i);
        }
        D3D12_ROOT_SIGNATURE_DESC desc{};
        if (!mParams.empty()) {
            desc.pParameters = mParams.data();
            desc.NumParameters = u32(mParams.size());
        }
        if (!mSamplers.empty()) {
            desc.pStaticSamplers = mSamplers.data();
            desc.NumStaticSamplers = u32(mSamplers.size());
        }
        if (isLocalRoot) {
            desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        }

        ComPtr<ID3DBlob> blob, errBlob;
        HRESULT hr = D3D12SerializeRootSignature(
            &desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errBlob);
        if (FAILED(hr)) {
            return nullptr;
        }

        ComPtr<ID3D12RootSignature> rootSignature;
        hr = device->GetDevice()->CreateRootSignature(
            0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        if (FAILED(hr)) {
            return nullptr;
        }

        return rootSignature;
    }
}


namespace utility {
    TextureResource LoadTextureFromFile(std::unique_ptr<dx12::RenderDeviceDX12>& rdevice, const std::wstring& fileName)
    {
        DirectX::TexMetadata metadata;
        DirectX::ScratchImage image;

        HRESULT hr = E_FAIL;
        const std::wstring extDDS(L"dds");
        const std::wstring extPNG(L"png");
        const std::wstring extJPG(L"jpg");
        const std::wstring extLJPG(L"JPG");
        if (fileName.length() < 3) {
            OutputDebugString(L"Texture Filename is Invalid.\n");
            throw std::runtime_error("Texture Filename is Invalid.");
        }

        if (std::equal(std::rbegin(extDDS), std::rend(extDDS), std::rbegin(fileName))) {
            hr = LoadFromDDSFile(fileName.c_str(), DDS_FLAGS_NONE, &metadata, image);
        }
        if (std::equal(std::rbegin(extPNG), std::rend(extPNG), std::rbegin(fileName)) ||
            std::equal(std::rbegin(extJPG), std::rend(extJPG), std::rbegin(fileName)) ||
            std::equal(std::rbegin(extLJPG), std::rend(extLJPG), std::rbegin(fileName))
            ) {
            hr = LoadFromWICFile(fileName.c_str(), WIC_FLAGS_NONE, &metadata, image);
        }

        if (hr == E_FAIL)
        {
            OutputDebugString(L"Texture Load Missed.\n");
            throw std::runtime_error("Texture Load Missed.");
        }

        ComPtr<ID3D12Resource> texRes;
        ComPtr<ID3D12Device> device;
        rdevice->GetDevice().As(&device);
        CreateTexture(device.Get(), metadata, &texRes);
        texRes->SetName(fileName.c_str());

        ComPtr<ID3D12Resource> srcBuffer;
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        PrepareUpload(device.Get(), image.GetImages(), image.GetImageCount(), metadata, subresources);
        const auto totalBytes = GetRequiredIntermediateSize(texRes.Get(), 0, u32(subresources.size()));

        auto staging = rdevice->CreateBuffer(
            totalBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
        staging->SetName(L"Tex-Staging");

        auto command = rdevice->CreateCommandList();
        UpdateSubresources(
            command.Get(), texRes.Get(), staging.Get(), 0, 0, u32(subresources.size()), subresources.data());
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texRes.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        command->ResourceBarrier(1, &barrier);
        command->Close();

        rdevice->ExecuteCommandList(command);
        rdevice->WaitForCompletePipe();

        utility::TextureResource ret;
        ret.res = texRes;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = metadata.format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (metadata.IsCubemap()) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = u32(metadata.mipLevels);
            srvDesc.TextureCube.MostDetailedMip = 0;
            srvDesc.TextureCube.ResourceMinLODClamp = 0;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = u32(metadata.mipLevels);
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0;
        }
        ret.srv = rdevice->CreateShaderResourceView(texRes.Get(), &srvDesc);

        return ret;
    }

    HRESULT ReadDataFromFile(LPCWSTR filename, byte** dataPtr, u32* sizePtr)
    {
        using namespace Microsoft::WRL;

        CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
        extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
        extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
        extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
        extendedParams.lpSecurityAttributes = nullptr;
        extendedParams.hTemplateFile = nullptr;

        Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
        if (file.Get() == INVALID_HANDLE_VALUE)
        {
            OutputDebugString(L"file not found\n");
            throw std::runtime_error("file not found");
        }

        FILE_STANDARD_INFO fileInfo = {};
        if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            OutputDebugString(L"file not found\n");
            throw std::runtime_error("file not found");
        }

        if (fileInfo.EndOfFile.HighPart != 0)
        {
            OutputDebugString(L"file not found\n");
            throw std::runtime_error("file not found");
        }

        *dataPtr = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
        *sizePtr = fileInfo.EndOfFile.LowPart;

        if (!ReadFile(file.Get(), *dataPtr, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
        {
            throw std::exception();
        }

        return S_OK;
    }

    std::vector<char> CompileShaderAtRuntime(const std::filesystem::path& shaderFile)
    {
        HRESULT hr;
        std::ifstream infile(shaderFile, std::ifstream::binary);
        if (!infile) {
            OutputDebugString(L"Runtime Shader Compile Failed.\n");
            throw std::runtime_error("Runtime Shader Compile Failed.");
        }

        std::wstring fileName = shaderFile.filename().wstring();
        std::stringstream strstream;
        strstream << infile.rdbuf();
        std::string shaderCode = strstream.str();

        ComPtr<IDxcLibrary> DXCLibrary;
        DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&DXCLibrary));
        ComPtr<IDxcIncludeHandler> DXCIncludeHandler;
        DXCLibrary->CreateIncludeHandler(&DXCIncludeHandler);

        ComPtr<IDxcCompiler> DXCCompiler;
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXCCompiler));

        ComPtr<IDxcBlobEncoding> blobEnc;
        DXCLibrary->CreateBlobWithEncodingFromPinned(
            (LPBYTE)shaderCode.c_str(), (u32)shaderCode.size(), CP_UTF8, &blobEnc);

        std::vector<LPCWSTR> compileArgsTbl;
#if _DEBUG
        compileArgsTbl.emplace_back(L"/Od");
#endif

        const auto compileTarget = L"lib_6_4";
        ComPtr<IDxcOperationResult> DXCResult;
        hr = DXCCompiler->Compile(blobEnc.Get(), fileName.c_str(), L"", compileTarget,
            compileArgsTbl.data(), u32(compileArgsTbl.size()),
            nullptr, 0,
            DXCIncludeHandler.Get(), &DXCResult);

        if (FAILED(hr)) {
            OutputDebugString(L"Runtime Shader Compile Failed.\n");
            throw std::runtime_error("Runtime Shader Compile Failed.");
        }

        DXCResult->GetStatus(&hr);

        if (FAILED(hr)) {
            ComPtr<IDxcBlobEncoding> errBlob;
            hr = DXCResult->GetErrorBuffer(&errBlob);
            auto size = errBlob->GetBufferSize();
            std::vector<char> errLog(size + 1);
            memcpy(errLog.data(), errBlob->GetBufferPointer(), size);
            errLog.back() = 0;
            OutputDebugStringA(errLog.data());
            OutputDebugString(L"Runtime Shader Compile Failed.\n");
            throw std::runtime_error("Runtime Shader Compile Failed.");
        }

        ComPtr<IDxcBlob> blob;
        hr = DXCResult->GetResult(&blob);

        if (SUCCEEDED(hr)) {
            std::vector<char> result;
            auto size = blob->GetBufferSize();
            result.resize(size);
            memcpy(result.data(), blob->GetBufferPointer(), size);
            return result;
        }
        OutputDebugString(L"Runtime Shader Compile Failed.\n");
        throw std::runtime_error("Runtime Shader Compile Failed.");
    }
}

namespace utility {
    void CreatePlane(std::vector<VertexPN>& vertices, std::vector<u32>& indices, f32 size)
    {
        vertices = {
            VertexPN{ {-1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f } },
            VertexPN{ {-1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
            VertexPN{ { 1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f } },
            VertexPN{ { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
        };

        for (auto& v : vertices)
        {
            v.Position.x *= size;
            v.Position.y *= size;
            v.Position.z *= size;
        }

        indices = { 0, 1, 2, 2, 1, 3 };
    }

    void CreatePlane(std::vector<VertexPNC>& vertices, std::vector<u32>& indices, f32 size)
    {
        const auto white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertices = {
            VertexPNC{ {-1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, white },
            VertexPNC{ {-1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, white },
            VertexPNC{ { 1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, white },
            VertexPNC{ { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, white },
        };

        for (auto& v : vertices)
        {
            v.Position.x *= size;
            v.Position.y *= size;
            v.Position.z *= size;
        }

        indices = { 0, 1, 2, 2, 1, 3 };
    }

    void CreatePlane(std::vector<VertexPNT>& vertices, std::vector<u32>& indices, f32 size)
    {
        vertices = {
            VertexPNT{ {-1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f} },
            VertexPNT{ {-1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f} },
            VertexPNT{ { 1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f} },
            VertexPNT{ { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f} },
        };

        for (auto& v : vertices)
        {
            v.Position.x *= size;
            v.Position.y *= size;
            v.Position.z *= size;
        }

        indices = { 0, 1, 2, 2, 1, 3 };
    }

    void CreateCube(std::vector<VertexPN>& vertices, std::vector<u32>& indices, f32 sizex, f32 sizey, f32 sizez)
    {
        vertices.clear();
        indices.clear();

        vertices = {
            // back
            { {-1.0f,-1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }},
            { {-1.0f, 1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f } },
            { { 1.0f, 1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f } },
            { { 1.0f,-1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f } },
            // right
            { { 1.0f,-1.0f,-1.0f}, { 1.0f, 0.0f, 0.0f } },
            { { 1.0f, 1.0f,-1.0f}, { 1.0f, 0.0f, 0.0f }},
            { { 1.0f, 1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f }},
            { { 1.0f,-1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f }},
            // left
            { {-1.0f,-1.0f, 1.0f}, { -1.0f, 0.0f, 0.0f }},
            { {-1.0f, 1.0f, 1.0f}, { -1.0f, 0.0f, 0.0f }},
            { {-1.0f, 1.0f,-1.0f}, { -1.0f, 0.0f, 0.0f }},
            { {-1.0f,-1.0f,-1.0f}, { -1.0f, 0.0f, 0.0f }},
            // front
            { { 1.0f,-1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}},
            { { 1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}},
            { {-1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}},
            { {-1.0f,-1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}},
            // top
            { {-1.0f, 1.0f,-1.0f}, { 0.0f, 1.0f, 0.0f}},
            { {-1.0f, 1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f} },
            { { 1.0f, 1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f} },
            { { 1.0f, 1.0f,-1.0f}, { 0.0f, 1.0f, 0.0f}},
            // bottom
            { {-1.0f,-1.0f, 1.0f}, { 0.0f, -1.0f, 0.0f}},
            { {-1.0f,-1.0f,-1.0f}, { 0.0f, -1.0f, 0.0f}},
            { { 1.0f,-1.0f,-1.0f}, { 0.0f, -1.0f, 0.0f}},
            { { 1.0f,-1.0f, 1.0f}, { 0.0f, -1.0f, 0.0f}},
        };
        indices = {
            0, 1, 2, 2, 3,0,
            4, 5, 6, 6, 7,4,
            8, 9, 10, 10, 11, 8,
            12,13,14, 14,15,12,
            16,17,18, 18,19,16,
            20,21,22, 22,23,20,
        };

        for (auto& v : vertices)
        {
            v.Position.x *= sizex;
            v.Position.y *= sizey;
            v.Position.z *= sizez;
        }
    }

    void CreateOpenedCube(std::vector<VertexPNT>& vertices, std::vector<u32>& indices, f32 size)
    {
        vertices.clear();
        indices.clear();

        vertices = {
            // back
            { {-1.0f,-1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f} },
            { {-1.0f, 1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f} },
            { { 1.0f, 1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f} },
            { { 1.0f,-1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f} },
            // right
            { { 1.0f,-1.0f,-1.0f}, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f} },
            { { 1.0f, 1.0f,-1.0f}, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f}},
            { { 1.0f, 1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f}},
            { { 1.0f,-1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f}},
            // left
            { {-1.0f,-1.0f, 1.0f}, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f}},
            { {-1.0f, 1.0f, 1.0f}, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f}},
            { {-1.0f, 1.0f,-1.0f}, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f}},
            { {-1.0f,-1.0f,-1.0f}, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f}},
            // top
            { {-1.0f, 1.0f,-1.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 0.0f}},
            { {-1.0f, 1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f} },
            { { 1.0f, 1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 1.0f} },
            { { 1.0f, 1.0f,-1.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f}},
            // bottom
            { {-1.0f,-1.0f, 1.0f}, { 0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f}},
            { {-1.0f,-1.0f,-1.0f}, { 0.0f, -1.0f, 0.0f}, { 0.0f, 1.0f}},
            { { 1.0f,-1.0f,-1.0f}, { 0.0f, -1.0f, 0.0f}, { 1.0f, 1.0f}},
            { { 1.0f,-1.0f, 1.0f}, { 0.0f, -1.0f, 0.0f}, { 1.0f, 0.0f}},
        };
        indices = {
            0, 1, 2, 2, 3,0,
            4, 5, 6, 6, 7,4,
            8, 9, 10, 10, 11, 8,
            12,13,14, 14,15,12,
            16,17,18, 18,19,16
        };

        for (auto& v : vertices)
        {
            v.Position.x *= size;
            v.Position.y *= size;
            v.Position.z *= size;
        }
    }

    static void SetVertex(
        VertexPN& vertex,
        const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& normal, const DirectX::XMFLOAT2& uv) {
        XMStoreFloat3(&vertex.Position, position);
        XMStoreFloat3(&vertex.Normal, normal);
    }

    static void SetVertex(
        VertexPNT& vertex,
        const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& normal, const DirectX::XMFLOAT2& uv) {
        XMStoreFloat3(&vertex.Position, position);
        XMStoreFloat3(&vertex.Normal, normal);
        vertex.UV = uv;
    }

    template<class T>
    static void CreateSphereVertices(std::vector<T>& vertices, f32 radius, s32 slices, s32 stacks) {
        using namespace DirectX;

        vertices.clear();

        vertices.resize((stacks + 1) * (slices + 1));

        u32 count = 0;

        for (s32 stack = 0; stack <= stacks; ++stack) {
            for (s32 slice = 0; slice <= slices; ++slice) {
                XMFLOAT3 pos;
                pos.y = 2.0f * stack / (f32)slices - 1.0f;
                f32 currentR = sqrtf(1 - pos.y * pos.y);
                f32 theta = 2.0f * XM_PI * slice / (f32)slices;
                pos.x = currentR * sinf(theta);
                pos.z = currentR * cosf(theta);

                XMVECTOR vertex = XMLoadFloat3(&pos) * radius;
                XMVECTOR normal = XMVector3Normalize(vertex);
                XMFLOAT2 uv = {
                    f32(slice) / (f32)slices,
                    1.0f - f32(stack) / (f32)stacks,
                };

                T v{};
                SetVertex(v, vertex, normal, uv);
                vertices.at(count) = v;

                count++;
            }
        }
    }

    static void CreateSphereIndices(std::vector<u32>& indices, s32 slices, s32 stacks) {
        indices.reserve((u32)(stacks * slices));

        for (s32 stack = 0; stack < stacks; ++stack) {
            for (s32 slice = 0; slice < slices; ++slice) {
                s32 idx = stack * (slices + 1);
                s32 i0 = idx + (slice + 0) % (slices + 1);
                s32 i1 = idx + (slice + 1) % (slices + 1);
                s32 i2 = i0 + (slices + 1);
                s32 i3 = i1 + (slices + 1);

                //triangle
                indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
                //triangle
                indices.push_back(i2); indices.push_back(i1); indices.push_back(i3);
            }
        }
    }

    void CreateSphere(std::vector<VertexPN>& vertices, std::vector<u32>& indices, f32 radius, s32 slices, s32 stacks)
    {
        vertices.clear();
        indices.clear();
        CreateSphereVertices(vertices, radius, slices, stacks);
        CreateSphereIndices(indices, slices, stacks);
    }

    void CreateSphere(std::vector<VertexPNT>& vertices, std::vector<u32>& indices, f32 radius, s32 slices, s32 stacks)
    {
        vertices.clear();
        indices.clear();
        CreateSphereVertices(vertices, radius, slices, stacks);
        CreateSphereIndices(indices, slices, stacks);
    }

    void Split(char splitChar, char* bufferPtr, std::vector<std::string>& out)
    {
        s32 count = 0;
        if (bufferPtr == nullptr)
        {
            return;
        }

        s32 startPoint = 0;

        while (bufferPtr[count] != '\0')
        {
            if (bufferPtr[count] == splitChar)
            {
                if (startPoint != count)
                {
                    char splitStr[256] = { 0 };
                    strncpy_s(splitStr, 256, &bufferPtr[startPoint], count - startPoint);
                    out.emplace_back(splitStr);
                }
                else
                {
                    out.emplace_back("");
                }
                startPoint = count + 1;
            }
            count++;
        }

        if (startPoint != count)
        {
            char splitStr[256] = { 0 };
            strncpy_s(splitStr, 256, &bufferPtr[startPoint], count - startPoint);
            out.emplace_back(splitStr);
        }
    }

    void VertexParser(std::vector<XMFLOAT3>& data, char* bufferPtr)
    {
        std::vector<std::string> splitStrings;
        Split(' ', bufferPtr, splitStrings);

        s32 count = 0;
        f32 values[3] = { 0.0f };

        for (std::string str : splitStrings)
        {
            values[count] = (f32)atof(str.c_str());
            count++;
        }

        data.push_back(XMFLOAT3(values[0], values[1], values[2]));
    }

    void FacetParser(std::vector<VertexPN>& outVertices, std::vector<u32>& indices, std::vector<XMFLOAT3>& vertices, std::vector<XMFLOAT3>& normals, char* bufferPtr)
    {
        s32 count = 0;
        s32 vertexInfo[3] =
        {
            -1, -1, -1,
        };
        std::vector<std::string> spaceSplit;

        Split(' ', bufferPtr, spaceSplit);

        for (s32 i = 0; i < spaceSplit.size(); i++)
        {
            VertexPN vertex;
            SlashParser(vertexInfo, (char*)spaceSplit.at(i).c_str());

            for (s32 i = 0; i < 3; i++)
            {
                if (vertexInfo[i] == -1)
                {
                    continue;
                }

                s32 id = vertexInfo[i];

                switch (i)
                {
                case 0:
                    vertex.Position = vertices[id];
                    break;
                case 2:
                    vertex.Normal = normals[id];
                    break;
                }
            }

            outVertices.push_back(vertex);
            indices.push_back((u32)outVertices.size() - 1);
        }
    }

    void SlashParser(s32* listPtr, char* bufferPtr)
    {
        s32 counter = 0;
        std::vector<std::string> slashSplit;
        Split('/', bufferPtr, slashSplit);

        for (std::string str : slashSplit)
        {
            if (str.size() > 0)
            {
                listPtr[counter] = atoi(str.c_str()) - 1;
            }
            counter++;
        }
    }

    void Replace(char searchChar, char replaceChar, char* bufferPtr)
    {
        u32 len = (u32)strlen(bufferPtr);

        for (u32 i = 0; i < len; i++)
        {
            if (bufferPtr[i] == searchChar)
            {
                bufferPtr[i] = replaceChar;
            }
        }
    }

    bool CreateMesh(const char* fileNamePtr, std::vector<VertexPN>& outVertices, std::vector<u32>& indices, XMFLOAT3 scale)
    {
        outVertices.clear();
        indices.clear();

        FILE* fp = nullptr;
        fopen_s(&fp, fileNamePtr, "r");

        if (fp == nullptr)
        {
            return false;
        }

        std::vector<XMFLOAT3> vertices;
        std::vector<XMFLOAT3> normals;

        const s32 LineBufferLength = 1024;
        char buffer[LineBufferLength];

        while (fgets(buffer, LineBufferLength, fp) != nullptr)
        {
            //Comment
            if (buffer[0] == '#')
            {
                continue;
            }

            char* parsePoint = strchr(buffer, ' ');
            if (parsePoint == nullptr)
            {
                continue;
            }

            Replace('\n', '\0', buffer);

            //Vertex / Normal
            if (buffer[0] == 'v')
            {
                if (buffer[1] == ' ')
                {
                    VertexParser(vertices, &parsePoint[1]);
                }
                else if (buffer[1] == 'n')
                {
                    VertexParser(normals, &parsePoint[1]);
                }
            }
            //Facet
            else if (buffer[0] == 'f')
            {
                FacetParser(outVertices, indices, vertices, normals, &parsePoint[1]);
            }
        }

        for (auto& v : outVertices)
        {
            v.Position.x *= scale.x;
            v.Position.y *= scale.y;
            v.Position.z *= scale.z;
        }

        fclose(fp);

        return true;
    }
}
