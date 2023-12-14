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

//This Program supports TRIANGULAR POLYGON only
//If u wanna see beautiful caustics, polygon normal must be smooth!!!
DxrPhotonMapper::DxrPhotonMapper(u32 width, u32 height) : AppBase(width, height, L"PhotonMapper"),
mMeshStage(), mMeshSphere(), mMeshBox(), mDispatchRayDesc(), mSceneParam(),
mNormalSphereMaterialTbl()
{
}

void DxrPhotonMapper::Setup()
{
    mIntenceBoost = 10000;
    mGatherRadius = 0.25f;
    mGatherBlockRange = 0;
    mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_MIDDLE);
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_LOW);
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_HIGH);
    mSceneParam.photonParams.w = 6;
    mLightPosX = 21.f; mLightPosY = 31; mLightPosZ = -5;
    mLightRange = 0.03f;
    mStandardPhotonNum = 1;// (2 * mPhotonMapSize1D / GRID_DIMENSION)* (2 * mPhotonMapSize1D / GRID_DIMENSION);// mPhotonMapSize1D * 0.1f;
    mPhi = 396; mTheta = 276;
    mPhiDirectional = 396; mThetaDirectional = 276;
    mTmpAccumuRatio = 0.03f;
    mSpectrumMode = Spectrum_D65;
    mLightLambdaNum = 12;
    mGlassRotateRange = 4;
    mCausticsBoost = 1;
    mIsMoveModel = false;
    mIsApplyCaustics = true;
    mIsUseDenoise = false;
    mIsDebug = false;
    mVisualizeLightRange = true;
    mInverseMove = false;
    mIsUseTexture = false;
    mIsTargetGlass = true;
    mStageTextureFileName = L"model/tileTex.png";
    mCubeMapTextureFileName = L"model/ParisEquirec.png";
    //mCubeMapTextureFileName = L"model/ForestEquirec.png";

    //mOBJFileName = "crytekSponza.obj";
    //mOBJFolderName = "model/crytekSponza";
    //mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(0, 0, 0));

    mOBJFileName = "skull.obj";
    mOBJFolderName = "model";
    mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, -15, 0));

    mStageType = StageType_Plane;

    mGlassModelType = ModelType::ModelType_Dragon;
    //mGlassModelType = ModelType::ModelType_Sponza;
    mMetalModelType = ModelType::ModelType_TwistCube;

    switch (mGlassModelType)
    {
    case  ModelType::ModelType_Crab:
    {
        mOBJ0FileName = L"model/crab.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case ModelType::ModelType_TwistCube:
    {
        mOBJ0FileName = L"model/twistCube.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(5, 5, 5);
    }
    break;
    case ModelType::ModelType_Teapot:
    {
        mOBJ0FileName = L"model/teapot.obj";
        mCausticsBoost *= 0.5;
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(8, 8, 8);
    }
    break;
    case  ModelType::ModelType_LikeWater:
    {
        mOBJ0FileName = L"model/likeWater.obj";
        mCausticsBoost *= 3;
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(4, 4, 4);
    }
    break;
    case  ModelType::ModelType_Ocean:
    {
        mStageType = StageType_Box;
        mGlassRotateRange *= 2;
        mOBJ0FileName = L"model/ocean.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case  ModelType::ModelType_Ocean2:
    {
        mStageType = StageType_Box;
        mGlassRotateRange *= 2;
        mOBJ0FileName = L"model/ocean2.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case ModelType::ModelType_Diamond:
    {
        mOBJ0FileName = L"model/diamond.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(20, 20, 20);
    }
    break;
    case ModelType::ModelType_Skull:
    {
        mOBJ0FileName = L"model/skull.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(30, 30, 30);
    }
    break;
    default:
    {
        mOBJ0FileName = L"model/crab.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case  ModelType::ModelType_HorseStatue:
    {
        mOBJ0FileName = L"model/horse_statue_Tri.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(350, 350, 350);
    }
    break;
    case  ModelType::ModelType_Dragon:
    {
        mOBJ0FileName = L"model/dragon.obj";
        //mOBJ0FileName = L"san-miguel-low-poly.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case  ModelType::ModelType_Sponza:
    {
        mOBJ0FileName = L"model/triangulateSponza.obj";
        mGlassObjYOfsset = 0;
        mGlassObjScale = XMFLOAT3(5, 5, 5);
    }
    break;
    }

    switch (mMetalModelType)
    {
    case  ModelType::ModelType_Crab:
    {
        mOBJ1FileName = L"model/crab.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case ModelType::ModelType_TwistCube:
    {
        mOBJ1FileName = L"model/twistCube.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(3, 3, 3);
    }
    break;
    case ModelType::ModelType_Teapot:
    {
        mOBJ1FileName = L"model/teapot.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(2, 2, 2);
    }
    break;
    case  ModelType::ModelType_LikeWater:
    {
        mOBJ1FileName = L"model/likeWater.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(2, 4, 4);
    }
    break;
    case  ModelType::ModelType_Ocean:
    {
        mStageType = StageType_Box;
        mOBJ1FileName = L"model/ocean.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case  ModelType::ModelType_Ocean2:
    {
        mStageType = StageType_Box;
        mOBJ1FileName = L"model/ocean2.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case ModelType::ModelType_Diamond:
    {
        mOBJ1FileName = L"model/diamond.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(20, 20, 20);
    }
    break;
    case ModelType::ModelType_Skull:
    {
        mOBJ1FileName = L"model/skull.obj";
        mMetalObjYOfsset = 15;
        mMetalObjScale = XMFLOAT3(30, 30, 30);
    }
    break;
    default:
    {
        mOBJ1FileName = L"model/crab.obj";
        mMetalObjYOfsset = 15;
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
        Setup();
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
    XMFLOAT3 eyePos(-65, 8, 0);
    XMFLOAT3 target(0.0f, 0.0f, 0.0f);
    mCamera.SetLookAt(eyePos, target);

    mSceneParam.cameraParams = XMVectorSet(0.1f, 100.f, MAX_RECURSION_DEPTH / 2, 0);
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
    windowText << L" <I> : Inverse - " << (mInverseMove ? L"ON" : L"OFF")
        << L"  <SPACE> : ChangeTargetModel <R> : Roughness <S> : TransRatio <M> : Metallic"
        << L"    Photon : " << mPhotonMapSize1D * mPhotonMapSize1D << L"    " << getFrameRate() << L"[ms]";

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
        for (auto& pos : mOBJ0sNormalTbl)
        {
            pos = XMMatrixTranslation(0, mGlassObjYOfsset + mGlassRotateRange * sin(0.4 * mMoveFrame * OneRadian), 0);
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
    mSceneParam.directionalLightDirection = XMVectorSet(sin(mThetaDirectional * OneRadian) * cos(mPhiDirectional * OneRadian), sin(mThetaDirectional * OneRadian) * sin(mPhiDirectional * OneRadian), cos(mThetaDirectional * OneRadian), 0.0f);
    mSceneParam.directionalLightColor = XMVectorSet(1.0f, 0.5f, 0.1f, 0.0f);

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
    const f32 clampRange = (mStageType == StageType_Plane) ? 1.5 * PLANE_SIZE : 0.9 * PLANE_SIZE;

    switch (wparam)
    {
    case 'I':
        mInverseMove = !mInverseMove;
        break;
    case 'J':
        mIsMoveModel = !mIsMoveModel;
        break;
    case 'G':
        mGatherRadius = Clamp(0.01f, 2, mGatherRadius + (mInverseMove ? -0.01f : 0.01f));
        break;
    case 'X':
        mLightPosX = Clamp(-clampRange, clampRange, mLightPosX + (mInverseMove ? -PLANE_SIZE * 0.01f : PLANE_SIZE * 0.01f));
        break;
    case 'Y':
        mLightPosY = Clamp(-clampRange, clampRange, mLightPosY + (mInverseMove ? -PLANE_SIZE * 0.01f : PLANE_SIZE * 0.01f));
        break;
    case 'Z':
        mLightPosZ = Clamp(-clampRange, clampRange, mLightPosZ + (mInverseMove ? -PLANE_SIZE * 0.01f : PLANE_SIZE * 0.01f));
        break;
    case 'L':
        mLightRange = Clamp(0.01f, 0.4f, mLightRange + (mInverseMove ? -0.002f : 0.002f));
        break;
    case 'T':
        mTheta += mInverseMove ? -1 : 1;
        break;
    case 'P':
        mPhi += mInverseMove ? -1 : 1;
        break;
    case 'A':
        mThetaDirectional += mInverseMove ? -1 : 1;
        break;
    case 'F':
        mPhiDirectional += mInverseMove ? -1 : 1;
        break;
    case 'K':
        mIntenceBoost = Clamp(100, 100000, mIntenceBoost + (mInverseMove ? -100 : 100));
        break;
    case 'B':
        mGatherBlockRange = (u32)Clamp(0, 3, (f32)mGatherBlockRange + (mInverseMove ? -1 : 1));
        break;
    case 'C':
        mTmpAccumuRatio = Clamp(0.01f, 1, mTmpAccumuRatio + (mInverseMove ? -0.01f : 0.01f));
        break;
    case 'V':
        mVisualizeLightRange = !mVisualizeLightRange;
        break;
    case 'W':
        mLightLambdaNum = (u32)Clamp(3, 12, (f32)mLightLambdaNum + (mInverseMove ? -1 : 1));
        break;
    case 'N':
        mIsApplyCaustics = !mIsApplyCaustics;
        break;
    case 'D':
        mIsUseDenoise = !mIsUseDenoise;
        break;
    case 'Q':
        mCausticsBoost = Clamp(1, 50, mCausticsBoost + (mInverseMove ? -0.5 : 0.5));
        break;
    case 'U':
        mIsUseTexture = !mIsUseTexture;
        break;
        //material start
    case 'R':
        if(mIsTargetGlass)
        mMaterialParam0.roughness = Clamp(0.1, 1, mMaterialParam0.roughness + (mInverseMove ? -0.1 : 0.1));
        else
            mMaterialParam1.roughness = Clamp(0.1, 1, mMaterialParam1.roughness + (mInverseMove ? -0.1 : 0.1));
        break;
    case 'S':
        if (mIsTargetGlass)
        mMaterialParam0.transRatio = Clamp(0, 1, mMaterialParam0.transRatio + (mInverseMove ? -0.1 : 0.1));
        else
            mMaterialParam1.transRatio = Clamp(0, 1, mMaterialParam1.transRatio + (mInverseMove ? -0.1 : 0.1));
        break;
    case 'M':
        if (mIsTargetGlass)
        mMaterialParam0.metallic = Clamp(0, 1, mMaterialParam0.metallic + (mInverseMove ? -0.1 : 0.1));
        else
            mMaterialParam1.metallic = Clamp(0, 1, mMaterialParam1.metallic + (mInverseMove ? -0.1 : 0.1));
        break;
    case VK_SPACE:
        mIsTargetGlass = !mIsTargetGlass;
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

void DxrPhotonMapper::UpdateMaterialParams()
{
    auto materialConstantBuffer0 = mOBJ0MaterialCB.Get();
    mDevice->ImmediateBufferUpdateHostVisible(materialConstantBuffer0, &mMaterialParam0, sizeof(mMaterialParam0));

    auto materialConstantBuffer1 = mOBJ1MaterialCB.Get();
    mDevice->ImmediateBufferUpdateHostVisible(materialConstantBuffer1, &mMaterialParam1, sizeof(mMaterialParam1));
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

    const u32 src = mRenderFrame % 2;
    const u32 dst = (mRenderFrame + 1) % 2;

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
            CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)
    };

    D3D12_RESOURCE_BARRIER meanBarriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceMomentBufferTbl[src].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceMomentBufferTbl[dst].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
    };
    // BackBuffer To RT
    auto barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Present
    auto barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    mCommandList->ResourceBarrier(_countof(barriersSRVToUAV), barriersSRVToUAV);
    mCommandList->ResourceBarrier(_countof(meanBarriers), meanBarriers);

    UpdateSceneTLAS();
    UpdateMaterialParams();

    auto gridCB = mGridSortCB.Get();
    auto sceneCB = mSceneCB.Get();

    if (mIsApplyCaustics)
    {
        //PhotonMapping
        mCommandList->SetComputeRootSignature(mGlobalRootSigPhoton.Get());
        mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(1, mTLASDescriptor.hGpu);
        mCommandList->SetComputeRootDescriptorTable(2, mCubeMapTex.srv.hGpu);
        mCommandList->SetComputeRootDescriptorTable(3, mLuminanceMomentBufferDescriptorSRVTbl[src].hGpu);
        mCommandList->SetComputeRootConstantBufferView(4, sceneCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(5, mPhotonMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(6, mDepthBufferDescriptorUAVTbl[src].hGpu);
        mCommandList->SetComputeRootDescriptorTable(7, mDepthBufferDescriptorUAVTbl[dst].hGpu);
        mCommandList->SetComputeRootDescriptorTable(8, mPhotonGridIdDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(9, mPositionBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(10, mNormalBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(11, mMainOutputDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(12, mOutputDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(13, mLuminanceMomentBufferDescriptorUAVTbl[dst].hGpu);
        mCommandList->SetPipelineState1(mRTPSOPhoton.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "PhotonMapping");
        mCommandList->DispatchRays(&mDispatchPhotonRayDesc);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(_countof(uavBarriers)), uavBarriers);
        Grid3DSort();
    }

    //RayTracing
    mCommandList->SetComputeRootSignature(mGlobalRootSig.Get());
    mCommandList->SetComputeRootConstantBufferView(0, gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(1, mTLASDescriptor.hGpu);
    mCommandList->SetComputeRootDescriptorTable(2, mCubeMapTex.srv.hGpu);
    mCommandList->SetComputeRootDescriptorTable(3, mLuminanceMomentBufferDescriptorSRVTbl[src].hGpu);
    mCommandList->SetComputeRootConstantBufferView(4, sceneCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(5, mPhotonMapSortedDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(6, mDepthBufferDescriptorUAVTbl[src].hGpu);
    mCommandList->SetComputeRootDescriptorTable(7, mDepthBufferDescriptorUAVTbl[dst].hGpu);
    mCommandList->SetComputeRootDescriptorTable(8, mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(9, mPositionBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(10, mNormalBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(11, mMainOutputDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(12, mOutputDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(13, mLuminanceMomentBufferDescriptorUAVTbl[dst].hGpu);
    mCommandList->SetPipelineState1(mRTPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "PathTracing");
    mCommandList->DispatchRays(&mDispatchRayDesc);
    PIXEndEvent(mCommandList.Get());

    if (mIsUseDenoise)
    {
        SpatiotemporalVarianceGuidedFiltering();
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