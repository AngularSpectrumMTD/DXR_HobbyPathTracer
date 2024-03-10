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
#define ONE_RADIAN XM_PI / 180.f
#define MAX_ACCUMULATION_RANGE 1000

//This Program supports TRIANGULAR POLYGON only
//If u wanna see beautiful caustics, polygon normal must be smooth!!!
DxrPhotonMapper::DxrPhotonMapper(u32 width, u32 height) : AppBase(width, height, L"PhotonMapper"),
mMeshStage(), mMeshSphere(), mMeshBox(), mDispatchRayDesc(), mSceneParam(),
mNormalSphereMaterialTbl()
{
}

void DxrPhotonMapper::Setup()
{
    mSceneType = SceneType_Sponza;

    mIntenceBoost = 8;
    mGatherRadius = min(0.1f, (2.f * PLANE_SIZE) / GRID_DIMENSION);
    mGatherBlockRange = 1;
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_MIDDLE);
    mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_LOW);
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_HIGH);
    mSceneParam.photonParams.w = 6;
    mLightRange = 0.054f;
    mStandardPhotonNum = 1;// (2 * mPhotonMapSize1D / GRID_DIMENSION)* (2 * mPhotonMapSize1D / GRID_DIMENSION);// mPhotonMapSize1D * 0.1f;
    mPhiDirectional = 70; mThetaDirectional = 280;
    mSpectrumMode = Spectrum_D65;
    mLightLambdaNum = 12;
    mGlassRotateRange = 4;
    mCausticsBoost = 200;
    mIsMoveModel = false;
    mIsApplyCaustics = true;
    mIsUseDenoise = false;
    mIsDebug = false;
    mVisualizeLightRange = false;
    mInverseMove = false;
    mIsUseTexture = false;
    mIsTargetGlass = true;
    mIsUseAccumulation = false;
    mIsIndirectOnly = false;
    mStageTextureFileName = L"model/tileTex.png";
    //mCubeMapTextureFileName = L"model/ParisEquirec.png";
    mCubeMapTextureFileName = L"model/SkyEquirec.png";
    //mCubeMapTextureFileName = L"model/ForestEquirec.png";

    mInitTargetPos = XMFLOAT3(0, 0, 0);

    mLightCount = LightCount_ALL - 1;

    switch (mSceneType)
    {
        case SceneType_Simple :
        {
            mOBJFileName = "skull.obj";
            mOBJFolderName = "model";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(30, 30, 30), XMMatrixTranslation(0, -55, 0));
            mLightPosX = -11.f; mLightPosY = -91; mLightPosZ = -1;
            mPhi = 480; mTheta = 257;
            mInitEyePos = XMFLOAT3(228, 270, -260);
            mLightRange = 0.0005f;
            mGlassModelType = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = true;
            mCausticsBoost = 400;
        }
        break;
        case SceneType_Sponza:
        {
            const bool isDiamondTest = false;
            mPhiDirectional = 70; mThetaDirectional = 280;
            mOBJFileName = "sponza.obj";
            mOBJFolderName = "model/sponza";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(0, 0, 0));
            
            if (isDiamondTest)
            {
                mLightPosX = 7.f; mLightPosY = 15; mLightPosZ = 5;//for diamond
                mPhi = 417; mTheta = 249;//for diamond
                mLightRange = 0.00018f;//for diamond
                mGlassModelType = ModelType_Diamond;
            }
            else
            {
                mLightPosX = 2.1f; mLightPosY = 12; mLightPosZ = 3.3;
                mPhi = 402; mTheta = 232;
                mLightRange = 0.00054f;
                mGlassModelType = ModelType_Afrodyta;
            }
            
            mInitEyePos = XMFLOAT3(-45, 42, 5.3);
            mIsSpotLightPhotonMapper = false;
        }
        break;
        case SceneType_BistroExterior:
        {
            mPhiDirectional = 480; mThetaDirectional = 280;
            mOBJFileName = "exterior.obj";
            mOBJFolderName = "model/bistro/Exterior";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
            mLightPosX = 15; mLightPosY = 26; mLightPosZ = 7;
            mPhi = 412; mTheta = 262;
            mInitEyePos = XMFLOAT3(19, 44, 32);
            mLightRange = 0.0002f;
            mGlassModelType = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = true;
        }
        break;
        case SceneType_BistroInterior:
        {
            mPhiDirectional = 150; mThetaDirectional = 250;
            mOBJFileName = "interior.obj";
            mOBJFolderName = "model/bistro/Interior";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
            mLightPosX = 34.0f; mLightPosY = 8.6; mLightPosZ = 5.7;
            mPhi = 402; mTheta = 256;
            mInitEyePos = XMFLOAT3(24, 8.57, 5.76);
            mInitTargetPos = XMFLOAT3(40, 5, -0.14);
            mLightRange = 0.001f;
            mGlassModelType = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = false;
            mGatherRadius = 0.021f;
            mCausticsBoost = 500;
            mIntenceBoost = 1;
        }
        break;
    }

    mGroundTex = utility::LoadTextureFromFile(mDevice, mStageTextureFileName);
    mCubeMapTex = utility::LoadTextureFromFile(mDevice, mCubeMapTextureFileName);

    mStageType = StageType_Plane;
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
        mGlassObjScale = XMFLOAT3(2, 2, 2);
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
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case  ModelType::ModelType_Afrodyta:
    {
        mOBJ0FileName = L"model/aphorodite/Tri_Deci_Rz_123_Afrodyta_z_Melos.obj";
        mGlassObjYOfsset = 8;
        mGlassObjScale = XMFLOAT3(0.1, 0.1, 0.1);
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
        mMetalObjYOfsset = 40;
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
        mMetalObjScale = XMFLOAT3(2, 2, 2);
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

    if (mSceneType == SceneType_Simple)
    {
        mMetalObjYOfsset -= 150;
        mGlassObjYOfsset -= 180;
        mGlassObjScale.x *= 3;
        mGlassObjScale.y *= 3;
        mGlassObjScale.z *= 3;
        mMetalObjScale.x *= 3;
        mMetalObjScale.y *= 3;
        mMetalObjScale.z *= 3;
    }

    WCHAR assetsPath[512];
    GetAssetsPath(assetsPath, _countof(assetsPath));
    mAssetPath = assetsPath;

    mDummyAlphaMask.res = mDevice->CreateTexture2D(
        1, 1, DXGI_FORMAT_R16_FLOAT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_HEAP_TYPE_DEFAULT
    );
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mDummyAlphaMask.srv = mDevice->CreateShaderResourceView(mDummyAlphaMask.res.Get(), &srvDesc);

    InitializeLightGenerateParams();
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
    mCamera.SetLookAt(mInitEyePos, mInitTargetPos);

    mSceneParam.cameraParams = XMVectorSet(0.1f, 100.f, REAL_MAX_RECURSION_DEPTH, 0);
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
        << L" <A> : Accunmulate - " << (mIsUseAccumulation ? L"ON" : L"OFF")
        << L"  <SPACE> : ChangeTargetModel <R> : Roughness <S> : TransRatio <M> : Metallic"
        << L"    Photon : " << mPhotonMapSize1D * mPhotonMapSize1D << L"    " << getFrameRate() << L"[ms]"
        << L"    Accumulated : " << min(MAX_ACCUMULATION_RANGE, mRenderFrame);

    std::wstring finalWindowText = std::wstring(GetTitle()) + windowText.str().c_str();
    SetWindowText(AppInvoker::GetHWND(), finalWindowText.c_str());
}

void DxrPhotonMapper::Update()
{
    for (auto& pos : mLightTbl)
    {
        pos = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
    }

    if (mIsMoveModel)
    {
        mIsUseAccumulation = false;
        mMoveFrame++;
        for (auto& pos : mOBJ0sNormalTbl)
        {
            pos = XMMatrixTranslation(0, mGlassObjYOfsset + mGlassRotateRange * sin(0.4 * mMoveFrame * ONE_RADIAN), 0);
        }
    }

    if (mIsUseAccumulation == false)
    {
        mRenderFrame = 0;
    }

    mSceneParam.mtxView = mCamera.GetViewMatrix();
    mSceneParam.mtxProj = mCamera.GetProjectionMatrix();
    mSceneParam.mtxViewInv = XMMatrixInverse(nullptr, mSceneParam.mtxView);
    mSceneParam.mtxProjInv = XMMatrixInverse(nullptr, mSceneParam.mtxProj);
    mSceneParam.lightColor = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    mSceneParam.backgroundColor = XMVectorSet(0.3f, 0.45f, 0.45f, 1.0f);
    mSceneParam.gatherParams = XMVectorSet(mGatherRadius, 2.f, mIntenceBoost, (f32)mGatherBlockRange);//radius sharp(if larger, photon visualize in small region) boost if radis large photon blured, w is blockRange
    mSceneParam.spotLightParams = XMVectorSet(mLightRange, (f32)mRenderFrame, (f32)mLightLambdaNum, mCausticsBoost);//light range,  seed, lambda num, CausticsBoost
    mSceneParam.gatherParams2 = XMVectorSet(mStandardPhotonNum, mIsUseAccumulation ? 1 : 0, 0, 0);
    mSceneParam.spotLightPosition = XMVectorSet(mLightPosX, mLightPosY, mLightPosZ, 0.0f);
    mSceneParam.spotLightDirection = XMVectorSet(sin(mTheta * ONE_RADIAN) * cos(mPhi * ONE_RADIAN), sin(mTheta * ONE_RADIAN) * sin(mPhi * ONE_RADIAN), cos(mTheta * ONE_RADIAN), 0.0f);
    mSceneParam.flags.x = 1;//0:DirectionalLight 1:SpotLight (Now Meaningless)
    mSceneParam.flags.y = mIsUseTexture;//Box Material 0: Texture 1:One Color
    mSceneParam.flags.z = mIsDebug ? 1 : 0;//1: Add HeatMap of Photon
    mSceneParam.flags.w = mVisualizeLightRange ? 1 : 0;//1: Visualize Light Range By Photon Intensity
    mSceneParam.photonParams.x = mIsApplyCaustics ? 1.f : 0.f;
    mSceneParam.photonParams.z = (f32)mSpectrumMode;
    mSceneParam.viewVec = XMVector3Normalize(mCamera.GetTarget() - mCamera.GetPosition());
    mSceneParam.additional.x = mLightCount;
    mSceneParam.additional.y = mIsIndirectOnly;
    mSceneParam.additional.z = 0;
    mSceneParam.additional.w = 0;

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
    mIsUseAccumulation = true;
}

void DxrPhotonMapper::OnKeyDown(UINT8 wparam)
{
    const f32 clampRange = (mStageType == StageType_Plane) ? 1.5 * PLANE_SIZE : 0.9 * PLANE_SIZE;

    if (mCamera.OnKeyDown(wparam))
    {
        mIsUseAccumulation = false;
    }

    switch (wparam)
    {
    case 'I':
        mInverseMove = !mInverseMove;
        break;
    case 'J':
        mIsMoveModel = !mIsMoveModel;
        mIsUseAccumulation = false;
        break;
    case 'G':
        mGatherRadius = Clamp(0.001f, max(0.05f, (2.f * PLANE_SIZE) / GRID_DIMENSION), mGatherRadius + (mInverseMove ? -0.01f : 0.01f));
        mIsUseAccumulation = false;
        break;
    case 'X':
        mLightPosX = Clamp(-clampRange, clampRange, mLightPosX + (mInverseMove ? -PLANE_SIZE * 0.002f : PLANE_SIZE * 0.002f));
        mIsUseAccumulation = false;
        break;
    case 'Y':
        mLightPosY = Clamp(-clampRange, clampRange, mLightPosY + (mInverseMove ? -PLANE_SIZE * 0.002f : PLANE_SIZE * 0.002f));
        mIsUseAccumulation = false;
        break;
    case 'Z':
        mLightPosZ = Clamp(-clampRange, clampRange, mLightPosZ + (mInverseMove ? -PLANE_SIZE * 0.002f : PLANE_SIZE * 0.002f));
        mIsUseAccumulation = false;
        break;
    case 'L':
        mLightRange = Clamp(0.0001f, 0.4f, mLightRange + (mInverseMove ? -0.00001f : 0.00001f));
        mIsUseAccumulation = false;
        break;
    case 'T':
        mTheta += mInverseMove ? -1 : 1;
        mIsUseAccumulation = false;
        break;
    case 'P':
        mPhi += mInverseMove ? -1 : 1;
        mIsUseAccumulation = false;
        break;
    case 'K':
        mIntenceBoost = Clamp(1, 100, mIntenceBoost + (mInverseMove ? -1 : 1));
        mIsUseAccumulation = false;
        break;
    case 'B':
        mGatherBlockRange = (u32)Clamp(0, 3, (f32)mGatherBlockRange + (mInverseMove ? -1 : 1));
        mIsUseAccumulation = false;
        break;
    case 'C':
        mIsSpotLightPhotonMapper = !mIsSpotLightPhotonMapper;
        mIsUseAccumulation = false;
        break;
    //case 'V':
    //    mVisualizeLightRange = !mVisualizeLightRange;
    //    mIsUseAccumulation = false;
    //    break;
    case 'A':
        mIsIndirectOnly = !mIsIndirectOnly;
        mIsUseAccumulation = false;
        break;
    case 'W':
        mLightLambdaNum = (u32)Clamp(3, 12, (f32)mLightLambdaNum + (mInverseMove ? -1 : 1));
        mIsUseAccumulation = false;
        break;
    case 'N':
        mIsApplyCaustics = !mIsApplyCaustics;
        mIsUseAccumulation = false;
        break;
    case 'D':
        mIsUseDenoise = !mIsUseDenoise;
        break;
    case 'Q':
        mCausticsBoost = Clamp(1, 500, mCausticsBoost + (mInverseMove ? -0.5 : 0.5));
        mIsUseAccumulation = false;
        break;
    case 'U':
        mIsUseTexture = !mIsUseTexture;
        mIsUseAccumulation = false;
        break;
        //material start
    case 'R':
        if (mIsTargetGlass)
        {
            mMaterialParam0.roughness = Clamp(0.1, 1, mMaterialParam0.roughness + (mInverseMove ? -0.1 : 0.1));
            mIsUseAccumulation = false;
        }
        else
        {
            mMaterialParam1.roughness = Clamp(0.1, 1, mMaterialParam1.roughness + (mInverseMove ? -0.1 : 0.1));
            mIsUseAccumulation = false;
        }
        break;
    case 'S':
        if (mIsTargetGlass)
        {
            mMaterialParam0.transRatio = Clamp(0, 1, mMaterialParam0.transRatio + (mInverseMove ? -0.1 : 0.1));
            mIsUseAccumulation = false;
        }
        else
        {
            mMaterialParam1.transRatio = Clamp(0, 1, mMaterialParam1.transRatio + (mInverseMove ? -0.1 : 0.1));
            mIsUseAccumulation = false;
        }
        break;
    case 'M':
        if (mIsTargetGlass)
        {
            mMaterialParam0.metallic = Clamp(0, 1, mMaterialParam0.metallic + (mInverseMove ? -0.1 : 0.1));
            mIsUseAccumulation = false;
        }
        else
        {
            mMaterialParam1.metallic = Clamp(0, 1, mMaterialParam1.metallic + (mInverseMove ? -0.1 : 0.1));
            mIsUseAccumulation = false;
        }
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
    mIsUseAccumulation = false;
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
    mIsUseAccumulation = false;
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
    //mIsUseAccumulation = false;
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

void DxrPhotonMapper::InitializeLightGenerateParams()
{
    u32 count = 0;
    for (u32 i = 0; i < LightCount_Sphere; i++)
    {
        LightGenerateParam param;
        param.setParamAsSphereLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), 1, 10);
        mLightGenerationParamTbl[count] = param;
        count++;
    }
    for (u32 i = 0; i < LightCount_Rect; i++)
    {
        if (i == 0 && !mIsSpotLightPhotonMapper)
        {
            LightGenerateParam param;
            param.setParamAsRectLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(1, 0, 0), XMFLOAT3(0, 0, 1), 10);
            mLightGenerationParamTbl[count] = param;
            count++;
        }
    }
    for (u32 i = 0; i < LightCount_Spot; i++)
    {
        if (i == 0 && mIsSpotLightPhotonMapper)
        {
            LightGenerateParam param;
            param.setParamAsSpotLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(1, 0, 0), XMFLOAT3(0, 0, 1), 10);
            mLightGenerationParamTbl[count] = param;
            count++;
        }
    }
    for (u32 i = 0; i < LightCount_Directional; i++)
    {
        LightGenerateParam param;
        param.setParamAsDirectionalLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
        mLightGenerationParamTbl[count] = param;
        count++;
    }
}

void DxrPhotonMapper::UpdateLightGenerateParams()
{
    u32 count = 0;

    const f32 scale = mLightRange;
    for (u32 i = 0; i < LightCount_Sphere; i++)
    {
        LightGenerateParam param;
        param.setParamAsSphereLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(5, 5, 5), scale, 150);
        mLightGenerationParamTbl[count] = param;
        count++;
    }
    for (u32 i = 0; i < LightCount_Rect; i++)
    {
        if (i == 0 && !mIsSpotLightPhotonMapper)
        {
            LightGenerateParam param;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, mSceneParam.spotLightDirection);
            utility::ONB(normal, tangent, bitangent);
            tangent.x *= scale;
            tangent.y *= scale;
            tangent.z *= scale;
            bitangent.x *= scale;
            bitangent.y *= scale;
            bitangent.z *= scale;
            param.setParamAsRectLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), tangent, bitangent, 150);
            mLightGenerationParamTbl[count] = param;
            count++;
        }
    }
    for (u32 i = 0; i < LightCount_Spot; i++)
    {
        if (i == 0 && mIsSpotLightPhotonMapper)
        {
            LightGenerateParam param;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, mSceneParam.spotLightDirection);
            utility::ONB(normal, tangent, bitangent);
            tangent.x *= scale;
            tangent.y *= scale;
            tangent.z *= scale;
            bitangent.x *= scale;
            bitangent.y *= scale;
            bitangent.z *= scale;
            param.setParamAsSpotLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), tangent, bitangent, 150);
            mLightGenerationParamTbl[count] = param;
            count++;
        }
    }
    for (u32 i = 0; i < LightCount_Directional; i++)
    {
        LightGenerateParam param;
        XMFLOAT3 direction;
        XMStoreFloat3(&direction, XMVectorSet(sin(mThetaDirectional * ONE_RADIAN) * cos(mPhiDirectional * ONE_RADIAN), sin(mThetaDirectional * ONE_RADIAN) * sin(mPhiDirectional * ONE_RADIAN), cos(mThetaDirectional * ONE_RADIAN), 0.0f));
        param.setParamAsDirectionalLight(direction, XMFLOAT3(1, 1, 1));
        mLightGenerationParamTbl[count] = param;
        count++;
    }

    auto buf = mLightGenerationParamBuffer.Get();
    
    D3D12_RESOURCE_BARRIER srvTodst[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(buf,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
    };

    D3D12_RESOURCE_BARRIER dstTosrv[] = {
    CD3DX12_RESOURCE_BARRIER::Transition(buf,D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    };

    //mCommandList->ResourceBarrier(_countof(srvTodst), srvTodst);
    mDevice->ImmediateBufferUpdateHostVisible(buf, mLightGenerationParamTbl.data(), sizeof(LightGenerateParam) * mLightGenerationParamTbl.size());
    //mDevice->ImmediateBufferUpdateStagingCopy(buf, mLightGenerationParamTbl.data(), sizeof(LightGenerateParam) * mLightGenerationParamTbl.size());
    //mCommandList->ResourceBarrier(_countof(dstTosrv), dstTosrv);
}

void DxrPhotonMapper::Draw()
{
    QueryPerformanceFrequency(&mCpuFreq);

    auto renderTarget = mDevice->GetRenderTarget();
    auto allocator = mDevice->GetCurrentCommandAllocator();
    allocator->Reset();
    mCommandList->Reset(allocator.Get(), nullptr);
    auto frameIndex = mDevice->GetCurrentFrameIndex();

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
        CD3DX12_RESOURCE_BARRIER::UAV(mAccumulationBuffer.Get()),
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

    UpdateSceneParams();
    UpdateSceneTLAS();
    UpdateMaterialParams();
    UpdateLightGenerateParams();

    auto gridCB = mGridSortCB.Get();
    auto sceneCB = mSceneCB.Get();

    if (mIsApplyCaustics)
    {
        //PhotonMapping
        mCommandList->SetComputeRootSignature(mGlobalRootSigPhoton.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigPhoton["gGridParam"], gridCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gRtScene"], mTLASDescriptor.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gEquiRecEnvMap"], mCubeMapTex.srv.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gLuminanceMomentBufferSrc"], mLuminanceMomentBufferDescriptorSRVTbl[src].hGpu);
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigPhoton["gSceneParam"], sceneCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonMap"], mPhotonMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gDepthBuffer"], mDepthBufferDescriptorUAVTbl[src].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPrevDepthBuffer"], mDepthBufferDescriptorUAVTbl[dst].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonGridIdBuffer"], mPhotonGridIdDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPositionBuffer"], mPositionBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gNormalBuffer"], mNormalBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gOutput"], mMainOutputDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gAccumulationBuffer"], mAccumulationBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gLuminanceMomentBufferDst"], mLuminanceMomentBufferDescriptorUAVTbl[dst].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gAccumulationCountBuffer"], mAccumulationCountBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gLightGenerateParams"], mLightGenerationParamSRV.hGpu);
        mCommandList->SetPipelineState1(mRTPSOPhoton.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "PhotonMapping");
        mCommandList->DispatchRays(&mDispatchPhotonRayDesc);
        PIXEndEvent(mCommandList.Get());

        mCommandList->ResourceBarrier(u32(_countof(uavBarriers)), uavBarriers);
        Grid3DSort();
    }

    //RayTracing
    mCommandList->SetComputeRootSignature(mGlobalRootSig.Get());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSig["gGridParam"], gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gRtScene"], mTLASDescriptor.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gEquiRecEnvMap"], mCubeMapTex.srv.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gLuminanceMomentBufferSrc"], mLuminanceMomentBufferDescriptorSRVTbl[src].hGpu);
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSig["gSceneParam"], sceneCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonMap"], mPhotonMapSortedDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gDepthBuffer"], mDepthBufferDescriptorUAVTbl[src].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPrevDepthBuffer"], mDepthBufferDescriptorUAVTbl[dst].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonGridIdBuffer"], mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPositionBuffer"], mPositionBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gNormalBuffer"], mNormalBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gOutput"], mMainOutputDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gAccumulationBuffer"], mAccumulationBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gLuminanceMomentBufferDst"], mLuminanceMomentBufferDescriptorUAVTbl[dst].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gAccumulationCountBuffer"], mAccumulationCountBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gLightGenerateParams"], mLightGenerationParamSRV.hGpu);
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