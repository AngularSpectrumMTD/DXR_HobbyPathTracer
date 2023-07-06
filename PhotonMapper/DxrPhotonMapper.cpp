#include "DxrPhotonMapper.h"

#include "AppInvoker.h"

#include <fstream>
#include <DirectXTex.h>

#include <wincodec.h>
#include "utility/Utility.h"

#include <iostream>
#include <string>
#include <sstream>

using namespace DirectX;

//This Program is only support TRIANGULAR POLYGON
//If u see beautiful caustics, polygon normal must be smooth!!!
DxrPhotonMapper::DxrPhotonMapper(u32 width, u32 height) : AppBase(width, height, L"PhotonMapper"),
mMeshStage(), mMeshSphere(), mMeshBox(), mDispatchRayDesc(), mSceneParam(),
mSpheresReflectTbl(), mSpheresRefractTbl(), mSpheresNormalTbl(),
mNormalSphereMaterialTbl()
{
    mIntenceBoost = 5000;
    mGatherRadius = 0.45f;
    mGatherBlockRange = 1;
    mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_MIDDLE);
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_HIGH);
    mSceneParam.photonParams.w = MAX_RECURSION_DEPTH;
    mLightPosX = 11.f;mLightPosY = 20;mLightPosZ = 1.2;
    mLightRange = 0.21f;
    mStandardPhotonNum = mPhotonMapSize1D * 0.1f;
    mPhi = 412; mTheta = 261;
    mTmpAccumuRatio = 0.05f;
    mSpectrumMode = Spectrum_D65;
    mLightLambdaNum = 12;
    mGlassRotateRange = 4;
    mCausticsBoost = 3;
    mIsMoveModel = true;
    mIsApplyCaustics = true;
    mIsUseDenoise = false;
    mIsDebug = false;
    mVisualizeLightRange = true;
    mReverseMove = false;
    mIsUseTexture = true;
    mStageTextureFileName = L"tileTex.png";
    mCubeMapTextureFileName = L"ParisEquirec.png";
    //mCubeMapTextureFileName = L"ForestEquirec.png";

   mGlassModelType = ModelType::ModelType_Skull;
   mMetalModelType = ModelType::ModelType_LikeWater;

    switch (mGlassModelType)
    {
        case  ModelType::ModelType_Crab:
        {
            mGlassFileName = L"crab.obj";
            mCausticsBoost *= 3;
            mGlassObjYOfsset = 5;
            mGlassObjScale = XMFLOAT3(12, 12, 12);
        }
        break;
        case ModelType::ModelType_TwistCube:
        {
            mGlassFileName = L"twistCube.obj";
            mGlassObjYOfsset = -5;
            mGlassObjScale = XMFLOAT3(15, 15, 15);
        }
        break;
        case ModelType::ModelType_Teapot:
        {
            mGlassFileName = L"teapot.obj";
            mCausticsBoost *= 0.5;
            mGlassObjYOfsset = -15;
            mGlassObjScale = XMFLOAT3(8, 8, 8);
        }
        break;
        case  ModelType::ModelType_LikeWater:
        {
            mGlassFileName = L"likeWater.obj";
            mCausticsBoost *= 3;
            mGlassObjYOfsset = 0;
            mGlassObjScale = XMFLOAT3(4, 4, 4);
        }
        break;
        case  ModelType::ModelType_Ocean:
        {
            mGlassRotateRange *= 2;
            mGlassFileName = L"ocean.obj";
            mGlassObjYOfsset = 0;
            mGlassObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
        }
        break;
        case  ModelType::ModelType_Ocean2:
        {
            mGlassRotateRange *= 2;
            mGlassFileName = L"ocean2.obj";
            mGlassObjYOfsset = 0;
            mGlassObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
        }
        break;
        case ModelType::ModelType_Diamond:
        {
            mGlassFileName = L"diamond.obj";
            mGlassObjYOfsset = 1;
            mGlassObjScale = XMFLOAT3(20, 20, 20);
        }
        break;
        case ModelType::ModelType_Skull:
        {
            mGlassFileName = L"skull.obj";
            mGlassObjYOfsset = 0;
            mGlassObjScale = XMFLOAT3(30, 30, 30);
        }
        break;
        default:
        {
            mGlassFileName = L"crab.obj";
            mGlassObjYOfsset = 0;
            mGlassObjScale = XMFLOAT3(12, 12, 12);
        }
        break;
    }

    switch (mMetalModelType)
    {
    case  ModelType::ModelType_Crab:
    {
        mMetalFileName = L"crab.obj";
        mMetalObjYOfsset = 20;
        mMetalObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case ModelType::ModelType_TwistCube:
    {
        mMetalFileName = L"twistCube.obj";
        mMetalObjYOfsset = 20;
        mMetalObjScale = XMFLOAT3(10, 10, 10);
    }
    break;
    case ModelType::ModelType_Teapot:
    {
        mMetalFileName = L"teapot.obj";
        mMetalObjYOfsset = 10;
        mMetalObjScale = XMFLOAT3(6, 6, 6);
    }
    break;
    case  ModelType::ModelType_LikeWater:
    {
        mMetalFileName = L"likeWater.obj";
        mMetalObjYOfsset = 2000;
        mMetalObjScale = XMFLOAT3(2, 4, 4);
    }
    break;
    case  ModelType::ModelType_Ocean:
    {
        mMetalFileName = L"ocean.obj";
        mMetalObjYOfsset = 20;
        mMetalObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case ModelType::ModelType_Diamond:
    {
        mMetalFileName = L"diamond.obj";
        mMetalObjYOfsset = 1;
        mMetalObjScale = XMFLOAT3(20, 20, 20);
    }
    break;
    case ModelType::ModelType_Skull:
    {
        mMetalFileName = L"skull.obj";
        mMetalObjYOfsset = 0;
        mMetalObjScale = XMFLOAT3(30, 30, 30);
    }
    break;
    default:
    {
        mMetalFileName = L"crab.obj";
        mMetalObjYOfsset = 20;
        mMetalObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    }

   WCHAR assetsPath[512];
   GetAssetsPath(assetsPath, _countof(assetsPath));
   mAssetPath = assetsPath;
}

void DxrPhotonMapper::Initialize()
{
    if (!InitializeRenderDevice(AppInvoker::GetHWND()))
    {
        throw std::runtime_error("Failed Initialize RenderDevice.");
    }
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if ((hr == S_OK) || (hr == S_FALSE))
    {
        CreateRegularBuffer();
        CreateConstantBuffer();
        CreateAccelerationStructure();
        CreateComputeRootSignatureAndPSO();
        CreateRaytracingRootSignatureAndPSO();
        CreateShaderTable();

        mCommandList = mDevice->CreateCommandList();
        mCommandList->Close();

        InitializeCamera();
    }
    else
    {
        throw std::runtime_error("Failed CoInitializeEx.");
    }
}

void DxrPhotonMapper::InitializeCamera()
{
    XMFLOAT3 eyePos(0, 0, 150.0f);
    XMFLOAT3 target(0.0f, 0.0f, 0.0f);
    mCamera.SetLookAt(eyePos, target);

    mSceneParam.cameraParams = XMVectorSet(0.1f, 100.f, 0, 0);
    mCamera.SetPerspective(
        XM_PIDIV4, GetAspect(), 0.1f, 100.0f
    );
}

void DxrPhotonMapper::Terminate()
{
    TerminateRenderDevice();
}

void DxrPhotonMapper::UpdateWindowText()
{
    std::wstringstream windowText;
    windowText.str(L"");
    windowText << L" R : Reverse" << (mReverseMove ? L"[ON]" : L"[OFF]")
        << L"  G: Gather XYZ: Pos L: Range TP: LAngle I: Intensity B: Block C: Accumurate V: Visualize W: Lambda K: Caustics D: Denoise Q: Boost U: Texture"
        << L"    <PhotonNum> " << mPhotonMapSize1D * mPhotonMapSize1D << L"    Exe " << getFrameRate() << L"[ms]";

    std::wstring finalWindowText = std::wstring(GetTitle()) + windowText.str().c_str();
    SetWindowText(AppInvoker::GetHWND(), finalWindowText.c_str());
}

void DxrPhotonMapper::Update()
{
    const f32 OneRadian = XM_PI / 180.f;

    for (auto& pos : mLightTbl)
    {
        pos = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
    }

    if (mIsMoveModel)
    {
        mMoveFrame++;
        for (auto& pos : mGlasssNormalTbl)
        {
            pos = XMMatrixTranslation(0, mGlassObjYOfsset + mGlassRotateRange * sin(0.5 * mMoveFrame * OneRadian), 0);
        }
    }

    mSceneParam.mtxView = mCamera.GetViewMatrix();
    mSceneParam.mtxProj = mCamera.GetProjectionMatrix();
    mSceneParam.mtxViewInv = XMMatrixInverse(nullptr, mSceneParam.mtxView);
    mSceneParam.mtxProjInv = XMMatrixInverse(nullptr, mSceneParam.mtxProj);
    mSceneParam.lightColor = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    mSceneParam.backgroundColor = XMVectorSet(0.3f, 0.45f, 0.45f, 1.0f);
    mSceneParam.gatherParams = XMVectorSet(mGatherRadius, 2.f, mIntenceBoost, (f32)mGatherBlockRange);//radius sharp(if larger, photon visualize in small region) boost if radis large photon blured, w is blockRange
    mSceneParam.spotLightParams = XMVectorSet(mLightRange, (f32)mRenderFrame, (f32)mLightLambdaNum, mCausticsBoost);//light range,  seed, lambda num, CausticsBoost
    mSceneParam.gatherParams2 = XMVectorSet(mStandardPhotonNum, 0, 0, 0);
    mSceneParam.spotLightPosition = XMVectorSet(mLightPosX, mLightPosY, mLightPosZ, 0.0f);
    mSceneParam.spotLightDirection = XMVectorSet(sin(mTheta * OneRadian) * cos(mPhi * OneRadian), sin(mTheta * OneRadian) * sin(mPhi * OneRadian), cos(mTheta * OneRadian), 0.0f);
    mSceneParam.flags.x = 1;//0:DirectionalLight 1:SpotLight (Now Meaningless)
    mSceneParam.flags.y = mIsUseTexture;//Box Material 0: Texture 1:One Color
    mSceneParam.flags.z = mIsDebug ? 1 : 0;//1: Add HeatMap of Photon
    mSceneParam.flags.w = mVisualizeLightRange ? 1 : 0;//1: Visualize Light Range By Photon Intensity
    mSceneParam.photonParams.y = mTmpAccumuRatio;
    mSceneParam.photonParams.x = mIsApplyCaustics ? 1.f : 0.f;
    mSceneParam.photonParams.z = (f32)mSpectrumMode;
    mSceneParam.viewVec = XMVector3Normalize(mCamera.GetTarget() - mCamera.GetPosition());

    mRenderFrame++;

    if (mRenderFrame >= (UINT_MAX >> 1))
    {
        mRenderFrame = 0;
    }
    if (mMoveFrame >= (UINT_MAX >> 1))
    {
        mMoveFrame = 0;
    }

    UpdateWindowText();
}

void DxrPhotonMapper::OnKeyDown(UINT8 wparam)
{
    switch (wparam)
    {
    case 'R':
        mReverseMove = !mReverseMove;
        break;
    case 'M':
        mIsMoveModel = !mIsMoveModel;
        break;
    case 'G':
        mGatherRadius = Clamp(0.01f, 2, mGatherRadius + (mReverseMove ? -0.01f : 0.01f));
        break;
    case 'X':
        mLightPosX = Clamp(-PLANE_SIZE * 0.9f, PLANE_SIZE * 0.9f, mLightPosX + (mReverseMove ? -PLANE_SIZE * 0.01f : PLANE_SIZE * 0.01f));
        break;
    case 'Y':
        mLightPosY = Clamp(-PLANE_SIZE * 0.9f, PLANE_SIZE * 0.9f, mLightPosY + (mReverseMove ? -PLANE_SIZE * 0.01f : PLANE_SIZE * 0.01f));
        break;
    case 'Z':
        mLightPosZ = Clamp(-PLANE_SIZE * 0.9f, PLANE_SIZE * 0.9f, mLightPosZ + (mReverseMove ? -PLANE_SIZE * 0.01f : PLANE_SIZE * 0.01f));
        break;
    case 'L':
        mLightRange = Clamp(0.01f, 0.4f, mLightRange + (mReverseMove ? -0.002f : 0.002f));
        break;
    case 'T':
        mTheta += mReverseMove ? -1 : 1;
        break;
    case 'P':
        mPhi += mReverseMove ? -1 : 1;
        break;
    case 'I':
        mIntenceBoost = Clamp(100, 10000, mIntenceBoost + (mReverseMove ? -100 : 100));
        break;
    case 'B':
        mGatherBlockRange = (u32)Clamp(0, 3, (f32)mGatherBlockRange + (mReverseMove ? -1 : 1));
        break;
    case 'C':
        mTmpAccumuRatio = Clamp(0.05f, 1, mTmpAccumuRatio + (mReverseMove ? -0.05f : 0.05f));
        break;
    //case 'S':
    //    mSpectrumMode = (s32)Clamp(0, Spectrum_Count - 1, (f32)mSpectrumMode + (mReverseMove ? -1 : 1));
    //    break;
    case 'V':
        mVisualizeLightRange = !mVisualizeLightRange;
        break;
    case 'W':
        mLightLambdaNum = (u32)Clamp(3, 12, (f32)mLightLambdaNum + (mReverseMove ? -1 : 1));
        break;
    case 'K':
        mIsApplyCaustics = !mIsApplyCaustics;
        break;
    case 'D':
        mIsUseDenoise = !mIsUseDenoise;
        break;
    case 'Q':
        mCausticsBoost = Clamp(1, 5, mCausticsBoost + (mReverseMove ? -0.5 : 0.5));
        break;
    case 'U':
        mIsUseTexture = !mIsUseTexture;
        break;
    }
}

void DxrPhotonMapper::OnMouseDown(MouseButton button, s32 x, s32 y)
{
    f32 fdx = f32(x) / GetWidth();
    f32 fdy = f32(y) / GetHeight();
    mCamera.OnMouseButtonDown(s32(button), fdx, fdy);
}

void DxrPhotonMapper::OnMouseUp(MouseButton button, s32 x, s32 y)
{
    mCamera.OnMouseButtonUp();
}

void DxrPhotonMapper::OnMouseMove(s32 dx, s32 dy)
{
    f32 fdx = f32(dx) / GetWidth();
    f32 fdy = f32(dy) / GetHeight();
    mCamera.OnMouseMove(-fdx, fdy);
}

void DxrPhotonMapper::OnMouseWheel(s32 rotate)
{
    //Note : 120 is wheel dir base
    if (rotate == 120)
    {
        rotate *= -1;
    }
    else
    {
        rotate = 120;
    }
    mCamera.OnMouseWheel(rotate / 8000.f);
}

f32 DxrPhotonMapper::Clamp(f32 min, f32 max, f32 src)
{
    return std::fmax(min, std::fmin(src, max));
}

void DxrPhotonMapper::UpdateSceneParams()
{
    auto sceneConstantBuffer = mSceneCB.Get();
    mDevice->ImmediateBufferUpdateHostVisible(sceneConstantBuffer, &mSceneParam, sizeof(mSceneParam));
}

void DxrPhotonMapper::Draw()
{
    QueryPerformanceFrequency(&mCpuFreq);

    auto renderTarget = mDevice->GetRenderTarget();
    auto allocator = mDevice->GetCurrentCommandAllocator();
    allocator->Reset();
    mCommandList->Reset(allocator.Get(), nullptr);
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    UpdateSceneParams();

    ID3D12DescriptorHeap* descriptorHeaps[] = {
        mDevice->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Get(),
    };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // To UAV
    D3D12_RESOURCE_BARRIER barriersSRVToUAV[] = { 
            CD3DX12_RESOURCE_BARRIER::Transition(mDXRMainOutput.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
    };
    CD3DX12_RESOURCE_BARRIER uavBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::UAV(mDXROutput.Get()),
    };
    // Copy To BackBuffer
    D3D12_RESOURCE_BARRIER barriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mDXRMainOutput.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST),
    };
    // BackBuffer To RT
    auto barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Present
    auto barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    mCommandList->ResourceBarrier(_countof(barriersSRVToUAV), barriersSRVToUAV);

    UpdateSceneTLAS();

    auto gridCB = mGridSortCB.Get();
    auto sceneCB = mSceneCB.Get();

    //PhotonMapping
    mCommandList->SetComputeRootSignature(mGrsPhoton.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mTLASDescriptor.hGpu);
    mCommandList->SetComputeRootDescriptorTable(2, mCubeMapTex.srv.hGpu);
    mCommandList->SetComputeRootConstantBufferView(3, sceneCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(4, mPhotonMapDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(5, mDepthBufferDescriptorUAVTbl[mRenderFrame % 2].hGpu);
    mCommandList->SetComputeRootDescriptorTable(6, mDepthBufferDescriptorUAVTbl[(mRenderFrame + 1) % 2].hGpu);
    mCommandList->SetComputeRootDescriptorTable(7, mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(8, mPositionBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(9, mNormalBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(10, mMainOutputDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(11, mOutputDescriptorUAV.hGpu);
    mCommandList->SetPipelineState1(mRTPSOPhoton.Get());
    mCommandList->DispatchRays(&mDispatchPhotonRayDesc);

    mCommandList->ResourceBarrier(u32(_countof(uavBarriers)), uavBarriers);
    Grid3DSort();

    //RayTracing
    mCommandList->SetComputeRootSignature(mGrs.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mTLASDescriptor.hGpu);
    mCommandList->SetComputeRootDescriptorTable(2, mCubeMapTex.srv.hGpu);
    mCommandList->SetComputeRootConstantBufferView(3, sceneCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(4, mPhotonMapDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(5, mDepthBufferDescriptorUAVTbl[mRenderFrame % 2].hGpu);
    mCommandList->SetComputeRootDescriptorTable(6, mDepthBufferDescriptorUAVTbl[(mRenderFrame + 1) % 2].hGpu);
    mCommandList->SetComputeRootDescriptorTable(7, mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(8, mPositionBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(9, mNormalBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(10, mMainOutputDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(11, mOutputDescriptorUAV.hGpu);
    mCommandList->SetPipelineState1(mRTPSO.Get());
    mCommandList->DispatchRays(&mDispatchRayDesc);

    if (mIsUseDenoise)
    {
        Denoise();
    }
    
    mCommandList->ResourceBarrier(_countof(barriers), barriers);
    mCommandList->CopyResource(renderTarget.Get(), mDXRMainOutput.Get());

    mCommandList->ResourceBarrier(1, &barrierToRT);

    const auto& rtv = mDevice->GetRenderTargetView();
    const auto& viewport = mDevice->GetDefaultViewport();
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    mCommandList->RSSetViewports(1, &viewport);

    //If you need override RT, dispatch Command Between Here
    //(Do Something)
    //To Here

    mCommandList->ResourceBarrier(1, &barrierToPresent);

    mCommandList->Close();

    QueryPerformanceCounter(&mStartTime);
    mDevice->ExecuteCommandList(mCommandList);
    mDevice->Present(1);
    QueryPerformanceCounter(&mEndTime);
}

f32 DxrPhotonMapper::getFrameRate()
{
    return 1000.0f * (mEndTime.QuadPart - mStartTime.QuadPart) / mCpuFreq.QuadPart;
}