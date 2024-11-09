#include "DxrPhotonMapper.h"

#include "AppInvoker.h"

#include <fstream>
#include <DirectXTex.h>

#include <wincodec.h>
#include "utility/Utility.h"

#include <iostream>
#include <string>
#include <sstream>

#include <random>

using namespace DirectX;
#define ONE_RADIAN XM_PI / 180.f
#define MAX_ACCUMULATION_RANGE 1000
#define DIRECTIONAL_LIGHT_POWER 10

#define NEE_AVAILABLE

#define USE_SPATIAL_RESERVOIR_FEEDBACK

//#define CUBE_TEST

//This Program supports TRIANGULAR POLYGON only
//If u wanna see beautiful caustics, polygon normal must be smooth!!!
DxrPhotonMapper::DxrPhotonMapper(u32 width, u32 height) : AppBase(width, height, L"HobbyTracer"),
mMeshStage(), mMeshSphere(), mMeshBox(), mDispatchRayDesc(), mSceneParam(),
mNormalSphereMaterialTbl()
{
}

void DxrPhotonMapper::UpdateWindowText()
{
    std::wstringstream windowText;
    windowText.str(L"");

    windowText 
        << L" <I> : Inv " << (mInverseMove ? L"ON" : L"OFF")
        << L" <A> : Acc " << (mIsUseAccumulation ? L"ON" : L"OFF")
        << L" <E> : NEE " << (mIsUseNEE ? L"ON" : L"OFF")
        << L" <CTRL> : RIS " << (mIsUseStreamingRIS ? L"ON" : L"OFF")
        << L" <F1> : Temporal " << (mIsUseReservoirTemporalReuse ? L"ON" : L"OFF")
        << L" <F3> : Spatial " << (mIsUseReservoirSpatialReuse ? L"ON" : L"OFF")
        << L" <SPACE> : Target <R> : Rough <S> : Trans <M> : Metal"
        << L" <D> : Bounce : " << mRecursionDepth
        //<< L" <F8> : SSS " << (mMaterialParam0.isSSSExecutable ? L"ON" : L"OFF")
        << L" Photon[K] : " << (mIsApplyCaustics ? mPhotonMapSize1D * mPhotonMapSize1D / 1024 : 0)
        //<< L"    " << getFrameRate() << L"[ms]"
        << L" Frame : " << min(MAX_ACCUMULATION_RANGE, mRenderFrame)
        ;

    std::wstring finalWindowText = std::wstring(GetTitle()) + windowText.str().c_str();
    SetWindowText(AppInvoker::GetHWND(), finalWindowText.c_str());
}

void DxrPhotonMapper::Setup()
{
    mSceneType = SceneType_GITest;

    mIsUseIBL = true;
    mRecursionDepth = min(5, REAL_MAX_RECURSION_DEPTH);
    mIntenceBoost = 300;
    mGatherRadius = 0.061f;
    mGatherBlockRange = 1;
    mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_MIDDLE);
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_LOW);
    //mPhotonMapSize1D = utility::roundUpPow2(CausticsQuality_HIGH);
    mSceneParam.photonParams.w = PHOTON_RECURSION_DEPTH;
    mLightRange = 1.0f;
    mStandardPhotonNum = 1;// (2 * mPhotonMapSize1D / GRID_DIMENSION)* (2 * mPhotonMapSize1D / GRID_DIMENSION);// mPhotonMapSize1D * 0.1f;
    mPhiDirectional = 70; mThetaDirectional = 280;
    mSpectrumMode = Spectrum_D65;
    mLightLambdaNum = 12;
    mGlassRotateRange = 8;
    mCausticsBoost = 0.005f;
    mIsMoveModel = false;
    mIsApplyCaustics = true;
    mIsUseDenoise = false;
    mIsDebug = true;
    mVisualizeLightRange = false;
    mInverseMove = false;
    mIsUseTexture = true;
    mIsUseDebugView = false;
    mIsTargetGlass = true;
    mIsUseAccumulation = false;
    mIsIndirectOnly = false;
    mIsUseNEE = true;
    mIsUseManySphereLightLighting = false;
    mStageTextureFileName = L"model/tileTex.png";
    //mCubeMapTextureFileName = L"model/ParisEquirec.png";
    mCubeMapTextureFileName = L"model/SkyEquirec.png";
    //mCubeMapTextureFileName = L"model/ForestEquirec.png";
    mIsUseStreamingRIS = true;
    mIsUseReservoirTemporalReuse = true;
    mIsUseReservoirSpatialReuse = true;
    mIsTemporalAccumulationForceDisable = true;

    mInitTargetPos = XMFLOAT3(0, 0, 0);

    mLightCount = LightCount_ALL - 1;

    mGroundTex = utility::LoadTextureFromFile(mDevice, mStageTextureFileName);
    mCubeMapTex = utility::LoadTextureFromFile(mDevice, mCubeMapTextureFileName);

    mStageType = StageType_Plane;
    mMetalModelType = ModelType::ModelType_SimpleCube;

    switch (mSceneType)
    {
        case SceneType_Simple :
        {
            mOBJFileName = "diamond.obj";
            mOBJFolderName = "model";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(30, 30, 30), XMMatrixTranslation(0, -55, 0));
            mStageOffsetX = 0;
            mStageOffsetY = -55.0f;
            mStageOffsetZ = 0;
            mLightPosX = -16.0f; mLightPosY = -172.0f; mLightPosZ = -4.2f;
            mPhi = 149.0f; mTheta = 257.0f;
            mPhiDirectional = 70.0f; mThetaDirectional = 220.0f;
            mInitEyePos = XMFLOAT3(188.0f, -163.0f, -101.0f);
            mInitTargetPos = XMFLOAT3(0.0f, -132.0f, 0.0f);
            mLightRange = 10.0f;
            mGlassModelType = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = true;
        }
        break;
        case SceneType_Sponza:
        {
            const bool isDebugMeshTest = false;
            const bool isRoomTestDebug = false;
            const bool isAfrodytaTest = true;
            mPhiDirectional = 100.0f; mThetaDirectional = 280.0f;
            mInitEyePos = XMFLOAT3(-27.9f, 15.0f, 5.54f);

            if (isRoomTestDebug)
            {
                mOBJFileName = "fireplace_room.obj";
                mOBJFolderName = "model/fireplace";
                mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(-20, 0, 10));
                mStageOffsetX = -20.0f;
                mStageOffsetY = 0.0f;
                mStageOffsetZ = 10.0f;
            }
            else
            {
                mOBJFileName = "sponza.obj";
                mOBJFolderName = "model/sponza";
                mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(0, 0, 0));
                mStageOffsetX = 0.0f;
                mStageOffsetY = 0.0f;
                mStageOffsetZ = 0.0f;
            }
            
            if (isRoomTestDebug)
            {
                mLightPosX = 1.99f; mLightPosY = 2.8f; mLightPosZ = 4.9f;
                mPhi = 306.0f; mTheta = 187.0f;
                mLightRange = 3.18f;
                mGlassModelType = ModelType_CurvedMesh;
                //mGlassModelType = ModelType_DebugMesh;
            }
            else
            {
                mLightPosX = 1.59f; mLightPosY = 9.8f; mLightPosZ = 3.19f;
                mPhi = 413.0f; mTheta = 242.0f;
                mLightRange = 0.79f;
                if (isDebugMeshTest)
                {
                    mGlassModelType = ModelType_DebugMesh;
                }
                else
                {
                    mLightPosX = 0.1f; mLightPosY = 6.59f; mLightPosZ = 3.4f;
                    mPhi = 447.0f; mTheta = 206.0f;
                    mGlassModelType = ModelType_Diamond;
                    mInitEyePos = XMFLOAT3(-20.0f, 19.0f, 2.4f);
                    if (isAfrodytaTest)
                    {
                        mLightPosX = -3.1f; mLightPosY = 12.19f; mLightPosZ = 1.79f;
                        mPhi = 334.0f; mTheta = 111.0f;
                        //mGlassModelType = ModelType_Afrodyta;
                        mGlassModelType = ModelType_Dragon;
                        //mGlassModelType = ModelType_SimpleCube;
                        mPhiDirectional = 100.0f; mThetaDirectional = 291.0f;
                        mInitEyePos = XMFLOAT3(38.6f, 14.23, -1.55f);
                        mInitTargetPos = XMFLOAT3(12.37f, 7.95, -7.3f);
                        mCausticsBoost = 0.014;
                        mLightRange = 0.29f;

                        mGlassModelType = ModelType_Buddha;
                        //mLightRange = 2.00f;
                    }
                }
            }
            mIsSpotLightPhotonMapper = false;
        }
        break;
        case SceneType_BistroExterior:
        {
            mCausticsBoost = 0.004f;
            mPhiDirectional = 480; mThetaDirectional = 263;
            mOBJFileName = "exterior.obj";
            mOBJFolderName = "model/bistro/Exterior";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
            mStageOffsetX = 20;
            mStageOffsetY = 0;
            mStageOffsetZ = 0;
            mLightPosX = -3.18f; mLightPosY = 8.2f; mLightPosZ = -2.08f;
            mPhi = 299; mTheta = 395;
            mInitEyePos = XMFLOAT3(-32, 16, -29);
            mInitTargetPos = XMFLOAT3(0, 8, 0);
            const bool isDragonTest = true;
            if (isDragonTest)
            {
                mInitTargetPos = XMFLOAT3(3.75, -1.03, -7.19);
#ifdef CUBE_TEST
                mInitEyePos = XMFLOAT3(-17, 23, -28);
                mLightRange = 2.29f;
#else
                mInitEyePos = XMFLOAT3(15.2f,11.0f, 24.3f);
#endif
                mGlassModelType = ModelType_Dragon;
            }
            else
            {
                mPhi = 412; mTheta = 262;
                mLightPosX = 4.2f; mLightPosY = 8.8f; mLightPosZ = 0.2f;
                mLightRange = 0.8f;
                mGlassModelType = ModelType_Afrodyta;
            }
            mIsSpotLightPhotonMapper = true;
        }
        break;
        case SceneType_BistroInterior:
        {
            mPhiDirectional = 150; mThetaDirectional = 250;
            mOBJFileName = "interior.obj";
            mOBJFolderName = "model/bistro/Interior";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
            mStageOffsetX = 20;
            mStageOffsetY = 0;
            mStageOffsetZ = 0;
            if (mIsApplyCaustics)
            {
                mLightPosX = 52.1; mLightPosY = 7.7f; mLightPosZ = -1.09f;
                mPhi = 222; mTheta = 87;
                mInitEyePos = XMFLOAT3(43.6, 11, 0.34);
                mInitTargetPos = XMFLOAT3(81.6, -1.9, -10.0f);
                mLightRange = 0.58f;
            }
            else
            {
                mLightPosX = 53; mLightPosY = 11.3f; mLightPosZ = -5.1f;
                mPhi = 376; mTheta = 107;
                mInitEyePos = XMFLOAT3(30, 12, 9);
                mInitTargetPos = XMFLOAT3(66, 10, -11.41f);
                mLightRange = 3.68f;
            }
            
            mGlassModelType = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = false;
            mCausticsBoost = 0.0002f;
            mGatherRadius = 0.011f;
        }
        break;
        case SceneType_SanMiguel:
        {
            //mPhiDirectional = 70; mThetaDirectional = 220;
            //mPhiDirectional = 100; mThetaDirectional = 280;//1
            mPhiDirectional = 104; mThetaDirectional = 255;//1
            mOBJFileName = "san-miguel-low-poly.obj";
            mOBJFolderName = "model/San_Miguel";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(1, 1, 1), XMMatrixTranslation(20, 0, 0));
            mStageOffsetX = 20;
            mStageOffsetY = 0;
            mStageOffsetZ = 0;
            mLightPosX = 53; mLightPosY = 11.3f; mLightPosZ = -5.1f;
            mPhi = 376; mTheta = 107;
            mInitEyePos = XMFLOAT3(30, 12, 9);
            mInitTargetPos = XMFLOAT3(66, 10, -11.41f);
            mLightRange = 3.68f;
            mGlassModelType = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = false;
        }
        break;
        case SceneType_Room:
        {
            const bool isDebugMeshTest = false;
            const bool isRoomTestDebug = false;
            const bool isAfrodytaTest = true;
            mPhiDirectional = 41.0f; mThetaDirectional = 245.0f;
            mInitEyePos = XMFLOAT3(-7.69f, 4.48, 18.3f);
            mInitTargetPos = XMFLOAT3(23.13, 7.58, -0.12f);

            mOBJFileName = "roomTestExp.obj";
            mOBJFolderName = "model/roomTestExp";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(3.5, 3.5, 3.5), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mGlassModelType = ModelType_Afrodyta;
        }
        break;
        case SceneType_GITest:
        {
            const bool isDebugMeshTest = false;
            const bool isRoomTestDebug = false;
            const bool isAfrodytaTest = true;
            mPhiDirectional = 123.0f; mThetaDirectional = 326.0f;
            mInitEyePos = XMFLOAT3(-45.85f, 46.655, 123.07f);
            mInitTargetPos = XMFLOAT3(-25.78, 39.42, 94.028f);

            mOBJFileName = "GITest.obj";
            mOBJFolderName = "model/GITest";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mGlassModelType = ModelType_Afrodyta;
        }
        break; 
        case SceneType_Kitchen:
        {
            const bool isDebugMeshTest = false; 
            const bool isRoomTestDebug = false;
            const bool isAfrodytaTest = true;
            mPhiDirectional = 8.0f; mThetaDirectional = 215.0f;
            mInitEyePos = XMFLOAT3(347, 46.51, -161.34f);
            mInitTargetPos = XMFLOAT3(319.1, 46.58, -142.253f);

            mOBJFileName = "Kitchen.obj";
            mOBJFolderName = "model/Kitchen";
            mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mGlassModelType = ModelType_Afrodyta;
            mIntenceBoost *= 3;
        }
        break;
    }

#ifdef GI_TEST
    mIsApplyCaustics = false;
    mIsIndirectOnly = true;
    mPhiDirectional = 111; mThetaDirectional = 250;
    mOBJFileName = "interior.obj";
    mOBJFolderName = "model/bistro/Interior";
    mOBJModelTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
    mStageOffsetX = 20;
    mStageOffsetY = 0;
    mStageOffsetZ = 0;
    mLightPosX = 53; mLightPosY = 3.7f; mLightPosZ = -5.9f;
    mPhi = 283; mTheta = 107;
    mInitEyePos = XMFLOAT3(30, 12, 9);
    mInitTargetPos = XMFLOAT3(66, 10, -11.41f);
    mLightRange = 2.98f;

    mGlassModelType = ModelType_Afrodyta;
    mIsSpotLightPhotonMapper = false;
    mCausticsBoost = 0.0002f;
    mGatherRadius = 0.011f;

    mIsTemporalAccumulationForceDisable = true;
#endif

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
    case ModelType::ModelType_SimpleCube:
    {
        mOBJ0FileName = L"model/simpleCube.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(5, 5, 5);
    }
    break;
    case ModelType::ModelType_Buddha:
    {
        mOBJ0FileName = L"model/buddha/deciBuddha.obj";//test
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(15, 15, 15);//test
        //mGlassObjScale = XMFLOAT3(0.1, 0.1, 0.1);//test
    }
    break;
    case ModelType::ModelType_Teapot:
    {
        mOBJ0FileName = L"model/teapot.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(2, 2, 2);
    }
    break;
    case  ModelType::ModelType_LikeWater:
    {
        mOBJ0FileName = L"model/likeWater.obj";
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
    case  ModelType::ModelType_CurvedMesh:
    {
        mOBJ0FileName = L"model/curvedMesh.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(4, 4, 4);
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
        mGlassObjYOfsset = 11;
        if (mSceneType == SceneType_BistroExterior)
        {
#ifndef CUBE_TEST
            mGlassObjYOfsset = 4;
#endif
        }
        mGlassObjScale = XMFLOAT3(12, 12, 12);
    }
    break;
    case  ModelType::ModelType_Afrodyta:
    {
        mOBJ0FileName = L"model/aphorodite/Tri_Deci_Rz_123_Afrodyta_z_Melos.obj";
        mGlassObjYOfsset = 8;
        mGlassObjScale = XMFLOAT3(0.1f, 0.1f, 0.1f);
        if (mSceneType == SceneType_Room)
        {
            mGlassObjYOfsset = 16;
            mGlassObjScale = XMFLOAT3(0.02f, 0.02f, 0.02f);
        }
    }
    break;
    case  ModelType::ModelType_Rock:
    {
        mOBJ0FileName = L"model/rock.obj";
        mGlassObjYOfsset = 8;
        mGlassObjScale = XMFLOAT3(5, 5, 5);
    }
    break;
    case  ModelType::ModelType_DebugMesh:
    {
        mOBJ0FileName = L"model/debugMesh.obj";
        mGlassObjYOfsset = 8;
        mGlassObjScale = XMFLOAT3(5, 5, 5);
    }
    break;
    default:
    {
        mOBJ0FileName = L"model/crab.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(12, 12, 12);
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
        mMetalObjYOfsset = (mSceneType == SceneType_BistroExterior) ? 15.0f :  50.0f;
        mMetalObjScale = XMFLOAT3(3, 3, 3);

        if (mSceneType == SceneType_Simple)
        {
            mMetalObjYOfsset = 90;
            mMetalObjScale = XMFLOAT3(12, 12, 12);
        }
        else if (mSceneType == SceneType_Sponza)
        {
            mMetalObjYOfsset = 90;
            mMetalObjScale = XMFLOAT3(12, 12, 12);
        }
#ifdef CUBE_TEST
        mMetalObjYOfsset = 10;//test
        mMetalObjScale = XMFLOAT3(6, 6, 6);//test
#endif
    }
    break;
    case ModelType::ModelType_SimpleCube:
    {
        mOBJ1FileName = L"model/simpleCube.obj";
        mMetalObjYOfsset = (mSceneType == SceneType_BistroExterior) ? 15.0f : 50.0f;
        mMetalObjScale = XMFLOAT3(3, 3, 3);

        if (mSceneType == SceneType_Simple)
        {
            mMetalObjYOfsset = 90;
            mMetalObjScale = XMFLOAT3(12, 12, 12);
        }
        else if (mSceneType == SceneType_Sponza)
        {
            mMetalObjYOfsset = 90;
            mMetalObjScale = XMFLOAT3(12, 12, 12);
        }
        else if (mSceneType == SceneType_BistroInterior)
        {
            mMetalObjYOfsset = 90;
            mMetalObjScale = XMFLOAT3(12, 12, 12);
        }
        else
        {
#ifdef CUBE_TEST
            mMetalObjYOfsset = 10;//test
            mMetalObjScale = XMFLOAT3(6, 6, 6);//test
#endif
        }
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
    case  ModelType::ModelType_CurvedMesh:
    {
        mOBJ0FileName = L"model/curvedMesh.obj";
        mGlassObjYOfsset = 5;
        mGlassObjScale = XMFLOAT3(4, 4, 4);
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
    case  ModelType::ModelType_DebugMesh:
    {
        mOBJ0FileName = L"model/debugMesh.obj";
        mGlassObjYOfsset = 8;
        mGlassObjScale = XMFLOAT3(5, 5, 5);
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

void DxrPhotonMapper::Draw()
{
    QueryPerformanceFrequency(&mCpuFreq);

    auto renderTarget = mDevice->GetRenderTarget();
    auto allocator = mDevice->GetCurrentCommandAllocator();
    allocator->Reset();
    mCommandList->Reset(allocator.Get(), nullptr);
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    const u32 prev = mRenderFrame % 2;
    const u32 curr = (mRenderFrame + 1) % 2;

    ID3D12DescriptorHeap* descriptorHeaps[] = {
        mDevice->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Get(),
    };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // To UAV
    D3D12_RESOURCE_BARRIER barriersSRVToUAV[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mFinalRenderResult.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
    };
    CD3DX12_RESOURCE_BARRIER uavBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::UAV(mAccumulationBuffer.Get())
    };
    // Copy To BackBuffer
    D3D12_RESOURCE_BARRIER barriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mFinalRenderResult.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)
    };

    D3D12_RESOURCE_BARRIER swapBarriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceMomentBufferTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceMomentBufferTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(mGIReservoirPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mGIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(mDIBufferPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mDIBufferPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(mGIBufferPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mGIBufferPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(mCausticsBufferPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mCausticsBufferPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
    };
    // BackBuffer To RT
    auto barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Present
    auto barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    mCommandList->ResourceBarrier(_countof(barriersSRVToUAV), barriersSRVToUAV);
    mCommandList->ResourceBarrier(_countof(swapBarriers), swapBarriers);

    mDevice->WaitForCompletePipe();
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
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigPhoton["gSceneParam"], sceneCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gBVH"], mTLASDescriptor.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gEquiRecEnvMap"], mCubeMapTex.srv.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gLightGenerateParams"], mLightGenerationParamSRV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonMap"], mPhotonMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonGridIdBuffer"], mPhotonGridIdDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPositionBuffer"], mPositionBufferDescriptorUAVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPrevIDBuffer"], mPrevIDBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gDIBuffer"], mDIBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gGIBuffer"], mGIBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gCausticsBuffer"], mCausticsBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gDIReservoirBuffer"], mDIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gDIReservoirBufferSrc"], mDISpatialReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gGIReservoirBuffer"], mGIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gGIReservoirBufferSrc"], mGISpatialReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonRandomCounterMap"], mPhotonRandomCounterMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[0].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[1].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[2].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap3"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[3].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap4"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[4].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap5"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[5].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPhotonEmissionGuideMap6"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[6].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gScreenSpaceMaterial"], mScreenSpaceMaterialBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gDebugTexture"], mDebugTexture0DescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPrevNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gPrevPositionBuffer"], mPositionBufferDescriptorUAVTbl[prev].hGpu);
        mCommandList->SetPipelineState1(mRTPSOPhoton.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "PhotonMapping");
        mCommandList->DispatchRays(&mDispatchPhotonRayDesc);
        PIXEndEvent(mCommandList.Get());

        CD3DX12_RESOURCE_BARRIER uavBarrierRandomCounter[] = {
             CD3DX12_RESOURCE_BARRIER::UAV(mPhotonRandomCounterMap.Get())
        };

        mCommandList->ResourceBarrier(u32(_countof(uavBarrierRandomCounter)), uavBarrierRandomCounter);

        mCommandList->SetComputeRootSignature(mRsClearPhotonEmissionGuideMap.Get());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[0].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[1].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[2].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap3"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[3].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap4"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[4].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap5"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[5].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapClearPhotonEmissionGuideMap["gPhotonEmissionGuideMap6"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[6].hGpu);
        mCommandList->SetPipelineState(mClearPhotonEmissionGuideMapPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "ClearPhotonEmissionGuideMap");
        mCommandList->Dispatch(PHOTON_EMISSION_GUIDE_MAP_SIZE_1D / 16, PHOTON_EMISSION_GUIDE_MAP_SIZE_1D / 16, 1);
        PIXEndEvent(mCommandList.Get());

        mCommandList->SetComputeRootSignature(mRsGeneratePhotonEmissionGuideMap.Get());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMap["gPhotonRandomCounterMap"], mPhotonRandomCounterMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMap["gPhotonEmissionGuideMap"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[0].hGpu);
        mCommandList->SetPipelineState(mGeneratePhotonEmissionGuideMapPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "GeneratePhotonEmissionGuideMap");
        mCommandList->Dispatch(PHOTON_EMISSION_GUIDE_MAP_SIZE_1D / 16, PHOTON_EMISSION_GUIDE_MAP_SIZE_1D / 16, 1);
        PIXEndEvent(mCommandList.Get());

        vector<CD3DX12_RESOURCE_BARRIER> uavBarrierGuideMap;
        for (u32 i = 0; i < PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL; i++)
        {
            uavBarrierGuideMap.push_back(CD3DX12_RESOURCE_BARRIER::UAV(mPhotonEmissionGuideMipMapTbl[i].Get()));
        }

        for (u32 i = 0; i + 1 < PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL; i += 3)
        {
            mCommandList->ResourceBarrier(PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL, uavBarrierGuideMap.data());

            switch (PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL - i)
            {
                case 2 : 
                {
                    mCommandList->SetComputeRootSignature(mRsGeneratePhotonEmissionGuideMipMap2x2.Get());
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap2x2["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[i + 0].hGpu);
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap2x2["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[i + 1].hGpu);
                    mCommandList->SetPipelineState(mGeneratePhotonEmissionGuideMipMap2x2PSO.Get());
                    PIXBeginEvent(mCommandList.Get(), 0, "GeneratePhotonEmissionGuideMipMap2x2");
                    mCommandList->Dispatch(1, 1, 1);
                    PIXEndEvent(mCommandList.Get());
                }
                break;
                case 3:
                {
                    mCommandList->SetComputeRootSignature(mRsGeneratePhotonEmissionGuideMipMap4x4.Get());
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap4x4["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[i + 0].hGpu);
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap4x4["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[i + 1].hGpu);
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap4x4["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[min(i + 2, PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL - 1)].hGpu);
                    mCommandList->SetPipelineState(mGeneratePhotonEmissionGuideMipMap4x4PSO.Get());
                    PIXBeginEvent(mCommandList.Get(), 0, "GeneratePhotonEmissionGuideMipMap4x4");
                    mCommandList->Dispatch(1, 1, 1);
                    PIXEndEvent(mCommandList.Get());
                }
                break;
                default:
                {
                    mCommandList->SetComputeRootSignature(mRsGeneratePhotonEmissionGuideMipMap.Get());
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[i + 0].hGpu);
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[i + 1].hGpu);
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[min(i + 2, PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL - 1)].hGpu);
                    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGeneratePhotonEmissionGuideMipMap["gPhotonEmissionGuideMap3"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[min(i + 3, PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL - 1)].hGpu);
                    mCommandList->SetPipelineState(mGeneratePhotonEmissionGuideMipMapPSO.Get());
                    PIXBeginEvent(mCommandList.Get(), 0, "GeneratePhotonEmissionGuideMipMap");
                    mCommandList->Dispatch(PHOTON_EMISSION_GUIDE_MAP_SIZE_1D / 8, PHOTON_EMISSION_GUIDE_MAP_SIZE_1D / 8, 1);
                    PIXEndEvent(mCommandList.Get());
                }
                break;
            }
        }

        Grid3DSort();
    }

    CD3DX12_RESOURCE_BARRIER uavBarrierRandomCounter[] = {
       CD3DX12_RESOURCE_BARRIER::UAV(mPhotonRandomCounterMap.Get())
    };

    mCommandList->ResourceBarrier(u32(_countof(uavBarrierRandomCounter)), uavBarrierRandomCounter);

    //RayTracing
    mCommandList->SetComputeRootSignature(mGlobalRootSig.Get());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSig["gGridParam"], gridCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSig["gSceneParam"], sceneCB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gBVH"], mTLASDescriptor.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gEquiRecEnvMap"], mCubeMapTex.srv.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gLightGenerateParams"], mLightGenerationParamSRV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonMap"], mPhotonMapDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonGridIdBuffer"], mPhotonGridIdDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPositionBuffer"], mPositionBufferDescriptorUAVTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPrevIDBuffer"], mPrevIDBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gDIBuffer"], mDIBufferDescriptorUAVPingPongTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gGIBuffer"], mGIBufferDescriptorUAVPingPongTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gCausticsBuffer"], mCausticsBufferDescriptorUAVPingPongTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gDIReservoirBuffer"], mDIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gDIReservoirBufferSrc"], mDISpatialReservoirDescriptorUAVPingPongTbl[curr].hGpu);//clear
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gGIReservoirBuffer"], mGIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gGIReservoirBufferSrc"], mGISpatialReservoirDescriptorUAVPingPongTbl[curr].hGpu);//clear
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonRandomCounterMap"], mPhotonRandomCounterMapDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[0].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[1].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[2].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap3"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[3].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap4"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[4].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap5"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[5].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPhotonEmissionGuideMap6"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[6].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gScreenSpaceMaterial"], mScreenSpaceMaterialBufferDescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gDebugTexture"], mDebugTexture0DescriptorUAV.hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPrevNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[prev].hGpu);
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gPrevPositionBuffer"], mPositionBufferDescriptorUAVTbl[prev].hGpu);
    mCommandList->SetPipelineState1(mRTPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "PathTracing");
    mCommandList->DispatchRays(&mDispatchRayDesc);
    PIXEndEvent(mCommandList.Get());

    mCommandList->ResourceBarrier(u32(_countof(uavBarriers)), uavBarriers);

    //if (mIsUseDenoise)
    //{
    //    SpatiotemporalVarianceGuidedFiltering();
    //}

    {
        CD3DX12_RESOURCE_BARRIER uavB[] = {
            CD3DX12_RESOURCE_BARRIER::UAV(mDIBufferPingPongTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mGIBufferPingPongTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mCausticsBufferPingPongTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mDIReservoirPingPongTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mGIReservoirPingPongTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mNormalDepthBufferTbl[prev].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mNormalDepthBufferTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mScreenSpaceMaterialBuffer.Get())
        };

        mCommandList->ResourceBarrier(u32(_countof(uavB)), uavB);
    }

    //Reservoir Temporal Reuse
    {
        //mCommandList->SetComputeRootSignature(mRsTemporalReuse.Get());
        //mCommandList->SetComputeRootConstantBufferView(mRegisterMapTemporalReuse["gSceneParam"], sceneCB->GetGPUVirtualAddress());
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["PrevNormalDepthBuffer"], mNormalDepthBufferDescriptorSRVTbl[prev].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["NormalDepthBuffer"], mNormalDepthBufferDescriptorSRVTbl[curr].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["PrevIDBuffer"], mPrevIDBufferDescriptorSRV.hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["DIReservoirBufferSrc"], mDIReservoirDescriptorSRVPingPongTbl[prev].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["DIReservoirBufferDst"], mDIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["GIReservoirBufferSrc"], mGIReservoirDescriptorSRVPingPongTbl[prev].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["GIReservoirBufferDst"], mGIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["PrevPositionBuffer"], mPositionBufferDescriptorSRVTbl[prev].hGpu);
        //mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalReuse["PositionBuffer"], mPositionBufferDescriptorSRVTbl[curr].hGpu);
        //mCommandList->SetPipelineState(mTemporalReusePSO.Get());
        //PIXBeginEvent(mCommandList.Get(), 0, "TemporalReuse");
        //mCommandList->Dispatch(GetWidth() / 16, GetHeight() / 16, 1);
        //PIXEndEvent(mCommandList.Get());

        mCommandList->SetComputeRootSignature(mGlobalRootSigReservoirTemporalReuse.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirTemporalReuse["gGridParam"], gridCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirTemporalReuse["gSceneParam"], sceneCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gBVH"], mTLASDescriptor.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gEquiRecEnvMap"], mCubeMapTex.srv.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gLightGenerateParams"], mLightGenerationParamSRV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonMap"], mPhotonMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonGridIdBuffer"], mPhotonGridIdDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPositionBuffer"], mPositionBufferDescriptorUAVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPrevIDBuffer"], mPrevIDBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gDIBuffer"], mDIBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gGIBuffer"], mGIBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gCausticsBuffer"], mCausticsBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gDIReservoirBuffer"], mDIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gDIReservoirBufferSrc"], mDIReservoirDescriptorUAVPingPongTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gGIReservoirBuffer"], mGIReservoirDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gGIReservoirBufferSrc"], mGIReservoirDescriptorUAVPingPongTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonRandomCounterMap"], mPhotonRandomCounterMapDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[0].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[1].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[2].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap3"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[3].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap4"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[4].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap5"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[5].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPhotonEmissionGuideMap6"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[6].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gScreenSpaceMaterial"], mScreenSpaceMaterialBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gDebugTexture"], mDebugTexture0DescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPrevNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gPrevPositionBuffer"], mPositionBufferDescriptorUAVTbl[prev].hGpu);
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirTemporalReuse["gReSTIRParam"], mReSTIRParamCBTbl[0]->GetGPUVirtualAddress());
        mCommandList->SetPipelineState1(mRTPSOReservoirTemporalReuse.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "ReservoirTemporalReuse");
        mCommandList->DispatchRays(&mDispatchReservoirTemporalReuseRayDesc);
        PIXEndEvent(mCommandList.Get());

        D3D12_RESOURCE_BARRIER tempBarrier[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[curr].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mGIReservoirPingPongTbl[curr].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        };
        mCommandList->ResourceBarrier(u32(_countof(tempBarrier)), tempBarrier);
    }

    u32 finalSpatialID = 0;
    //Reservoir Spatial Reuse
    {
        D3D12_RESOURCE_BARRIER copyReservoirBarrier[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mDISpatialReservoirPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST),
         CD3DX12_RESOURCE_BARRIER::Transition(mGIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mGISpatialReservoirPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        mCommandList->ResourceBarrier(u32(_countof(copyReservoirBarrier)), copyReservoirBarrier);
        mCommandList->CopyResource(mDISpatialReservoirPingPongTbl[prev].Get(), mDIReservoirPingPongTbl[curr].Get());
        mCommandList->CopyResource(mGISpatialReservoirPingPongTbl[prev].Get(), mGIReservoirPingPongTbl[curr].Get());

        D3D12_RESOURCE_BARRIER dstTouavReservoirBarrier[] = {
          CD3DX12_RESOURCE_BARRIER::Transition(mDISpatialReservoirPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
          CD3DX12_RESOURCE_BARRIER::Transition(mGISpatialReservoirPingPongTbl[prev].Get(),D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        mCommandList->ResourceBarrier(u32(_countof(dstTouavReservoirBarrier)), dstTouavReservoirBarrier);

        CD3DX12_RESOURCE_BARRIER uavB[] = {
            CD3DX12_RESOURCE_BARRIER::UAV(mDISpatialReservoirPingPongTbl[prev].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mDISpatialReservoirPingPongTbl[curr].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mGISpatialReservoirPingPongTbl[prev].Get()),
            CD3DX12_RESOURCE_BARRIER::UAV(mGISpatialReservoirPingPongTbl[curr].Get()),
        };

        mCommandList->ResourceBarrier(u32(_countof(uavB)), uavB);

        u32 spatialIDPing = curr;
        u32 spatialIDPong = prev;

        for (int i = 0; i < mSpatialReuseTap; i++)
        {
            ReSTIRParam cb;
            XMUINT4 d;
            d.x = max(1, 4 >> i);
            d.y = 0;
            d.z = 0;
            d.w = 0;
            cb.data = d;

            auto restirCB = mReSTIRParamCBTbl.at(i).Get();
            mDevice->ImmediateBufferUpdateHostVisible(restirCB, &cb, sizeof(cb));

            mCommandList->SetComputeRootSignature(mGlobalRootSigReservoirSpatialReuse.Get());
            mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirSpatialReuse["gGridParam"], gridCB->GetGPUVirtualAddress());
            mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirSpatialReuse["gSceneParam"], sceneCB->GetGPUVirtualAddress());
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gBVH"], mTLASDescriptor.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gEquiRecEnvMap"], mCubeMapTex.srv.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gLightGenerateParams"], mLightGenerationParamSRV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonMap"], mPhotonMapDescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[curr].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonGridIdBuffer"], mPhotonGridIdDescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPositionBuffer"], mPositionBufferDescriptorUAVTbl[curr].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPrevIDBuffer"], mPrevIDBufferDescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIBuffer"], mDIBufferDescriptorUAVPingPongTbl[curr].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gGIBuffer"], mGIBufferDescriptorUAVPingPongTbl[curr].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gCausticsBuffer"], mCausticsBufferDescriptorUAVPingPongTbl[curr].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIReservoirBuffer"], mDISpatialReservoirDescriptorUAVPingPongTbl[spatialIDPing].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gDIReservoirBufferSrc"], mDISpatialReservoirDescriptorUAVPingPongTbl[spatialIDPong].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gGIReservoirBuffer"], mGISpatialReservoirDescriptorUAVPingPongTbl[spatialIDPing].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gGIReservoirBufferSrc"], mGISpatialReservoirDescriptorUAVPingPongTbl[spatialIDPong].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonRandomCounterMap"], mPhotonRandomCounterMapDescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap0"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[0].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap1"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[1].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap2"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[2].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap3"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[3].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap4"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[4].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap5"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[5].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPhotonEmissionGuideMap6"], mPhotonEmissionGuideMipMapDescriptorUAVTbl[6].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gScreenSpaceMaterial"], mScreenSpaceMaterialBufferDescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gDebugTexture"], mDebugTexture0DescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPrevNormalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[prev].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gPrevPositionBuffer"], mPositionBufferDescriptorUAVTbl[prev].hGpu);
            mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirSpatialReuse["gReSTIRParam"], mReSTIRParamCBTbl[i]->GetGPUVirtualAddress());
            mCommandList->SetPipelineState1(mRTPSOReservoirSpatialReuse.Get());
            PIXBeginEvent(mCommandList.Get(), 0, "ReservoirSpatialReuse");
            mCommandList->DispatchRays(&mDispatchReservoirSpatialReuseRayDesc);
            PIXEndEvent(mCommandList.Get());

            u32 tmp = spatialIDPing;
            spatialIDPing = spatialIDPong;
            spatialIDPong = tmp;

            mCommandList->ResourceBarrier(u32(_countof(uavB)), uavB);
        }

        finalSpatialID = spatialIDPong;

        mCommandList->ResourceBarrier(u32(_countof(uavB)), uavB);

        //feedback
 #ifdef USE_SPATIAL_RESERVOIR_FEEDBACK
        D3D12_RESOURCE_BARRIER copyReservoirBarrier2[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST),
            CD3DX12_RESOURCE_BARRIER::Transition(mDISpatialReservoirPingPongTbl[finalSpatialID].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mGIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST),
            CD3DX12_RESOURCE_BARRIER::Transition(mGISpatialReservoirPingPongTbl[finalSpatialID].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)
        };
        mCommandList->ResourceBarrier(u32(_countof(copyReservoirBarrier2)), copyReservoirBarrier2);
        mCommandList->CopyResource(mDIReservoirPingPongTbl[curr].Get(), mDISpatialReservoirPingPongTbl[finalSpatialID].Get());
        mCommandList->CopyResource(mGIReservoirPingPongTbl[curr].Get(), mGISpatialReservoirPingPongTbl[finalSpatialID].Get());
#endif

        D3D12_RESOURCE_BARRIER UAVToSRVReservoirBarrier[] = {
 #ifdef USE_SPATIAL_RESERVOIR_FEEDBACK
           CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS), //feedback
           CD3DX12_RESOURCE_BARRIER::Transition(mGISpatialReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS), //feedback
 #endif
            CD3DX12_RESOURCE_BARRIER::Transition(mDISpatialReservoirPingPongTbl[finalSpatialID].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mGISpatialReservoirPingPongTbl[finalSpatialID].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        };

        mCommandList->ResourceBarrier(u32(_countof(UAVToSRVReservoirBarrier)), UAVToSRVReservoirBarrier);
    }

    //temporal accumulation
    {
        mCommandList->SetComputeRootSignature(mRsTemporalAccumulation.Get());
        mCommandList->SetComputeRootConstantBufferView(mRegisterMapTemporalAccumulation["gSceneParam"], sceneCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["HistoryDIBuffer"], mDIBufferDescriptorSRVPingPongTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["HistoryGIBuffer"], mGIBufferDescriptorSRVPingPongTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["HistoryCausticsBuffer"], mCausticsBufferDescriptorSRVPingPongTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["PrevNormalDepthBuffer"], mNormalDepthBufferDescriptorSRVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["NormalDepthBuffer"], mNormalDepthBufferDescriptorSRVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["PrevPositionBuffer"], mPositionBufferDescriptorSRVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["PositionBuffer"], mPositionBufferDescriptorSRVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["PrevIDBuffer"], mPrevIDBufferDescriptorSRV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["CurrentDIBuffer"], mDIBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["CurrentGIBuffer"], mGIBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["CurrentCausticsBuffer"], mCausticsBufferDescriptorUAVPingPongTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["DIGIBuffer"], mFinalRenderResultDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["AccumulationCountBuffer"], mAccumulationCountBufferDescriptorUAVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["PrevAccumulationCountBuffer"], mAccumulationCountBufferDescriptorUAVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["LuminanceMomentBufferSrc"], mLuminanceMomentBufferDescriptorSRVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["LuminanceMomentBufferDst"], mLuminanceMomentBufferDescriptorUAVTbl[curr].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["DIReservoirBufferSrc"], mDISpatialReservoirDescriptorSRVPingPongTbl[finalSpatialID].hGpu);//"dst"
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapTemporalAccumulation["GIReservoirBufferSrc"], mGISpatialReservoirDescriptorSRVPingPongTbl[finalSpatialID].hGpu);//"dst"
        mCommandList->SetPipelineState(mTemporalAccumulationPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "TemporalAccumulation");
        mCommandList->Dispatch(GetWidth() / 16, GetHeight() / 16, 1);
        PIXEndEvent(mCommandList.Get());

        {
            D3D12_RESOURCE_BARRIER SRVToUAVReservoirBarrier[] = {
                 CD3DX12_RESOURCE_BARRIER::Transition(mDISpatialReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                 CD3DX12_RESOURCE_BARRIER::Transition(mGISpatialReservoirPingPongTbl[curr].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            };
            mCommandList->ResourceBarrier(u32(_countof(SRVToUAVReservoirBarrier)), SRVToUAVReservoirBarrier);
        }

        D3D12_RESOURCE_BARRIER tempBarrier[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mDIReservoirPingPongTbl[curr].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        mCommandList->ResourceBarrier(u32(_countof(tempBarrier)), tempBarrier);
    }

    if (mIsUseDebugView)
    {
        std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
        uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mFinalRenderResult.Get()));
        uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mNormalDepthBufferTbl[prev].Get()));
        uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPrevIDBuffer.Get()));
        mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());

        mCommandList->SetComputeRootSignature(mRsDebugView.Get());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapDebugView["screenSpaceMaterial"], mScreenSpaceMaterialBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapDebugView["normalDepthBuffer"], mNormalDepthBufferDescriptorUAVTbl[prev].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapDebugView["finalColor"], mFinalRenderResultDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mDebugViewPSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "DebugView");
        mCommandList->Dispatch(GetWidth() / 16, GetHeight() / 16, 1);
        PIXEndEvent(mCommandList.Get());
    }

    mCommandList->ResourceBarrier(_countof(barriers), barriers);
    mCommandList->CopyResource(renderTarget.Get(), mFinalRenderResult.Get());

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

void DxrPhotonMapper::Update()
{
    if (mIsTemporalAccumulationForceDisable)
    {
        mIsUseAccumulation = false;
    }

    for (auto& pos : mLightTbl)
    {
        pos = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
    }

    if (mIsMoveModel)
    {
        mIsUseAccumulation = false;
        for (auto& pos : mOBJ0sNormalTbl)
        {
            pos = XMMatrixTranslation(0, mGlassObjYOfsset + mGlassRotateRange * sin(0.4f * mSeedFrame * ONE_RADIAN), 0);
        }
    }

    mSceneParam.mtxViewPrev = mSceneParam.mtxView;
    mSceneParam.mtxProjPrev = mSceneParam.mtxProj;
    mSceneParam.mtxViewInvPrev = mSceneParam.mtxViewInv;
    mSceneParam.mtxProjInvPrev = mSceneParam.mtxProjInv;
    mSceneParam.mtxView = mCamera.GetViewMatrix();
    mSceneParam.mtxProj = mCamera.GetProjectionMatrix();
    mSceneParam.mtxViewInv = XMMatrixInverse(nullptr, mSceneParam.mtxView);
    mSceneParam.mtxProjInv = XMMatrixInverse(nullptr, mSceneParam.mtxProj);
    mSceneParam.gatherParams = XMVectorSet(mGatherRadius, 2.f, 0, (f32)mGatherBlockRange);
    mSceneParam.spotLightParams = XMVectorSet(mLightRange, (f32)mSeedFrame, (f32)mLightLambdaNum, mCausticsBoost);//light range,  seed, lambda num, CausticsBoost
    mSceneParam.gatherParams2 = XMVectorSet(mStandardPhotonNum, mIsUseAccumulation ? 1.0f : 0.0f, 0.0f, 0.0f);
    mSceneParam.flags.x = 1;//0:DirectionalLight 1:SpotLight (Now Meaningless)
    mSceneParam.flags.y = mIsUseTexture;
    mSceneParam.flags.z = mIsDebug ? 1 : 0;//1: Add HeatMap of Photon
    mSceneParam.flags.w = mVisualizeLightRange ? 1 : 0;//1: Visualize Light Range By Photon Intensity
    mSceneParam.photonParams.x = mIsApplyCaustics ? 1.f : 0.f;
    mSceneParam.photonParams.z = (f32)mSpectrumMode;
    mSceneParam.viewVec = XMVector3Normalize(mCamera.GetTarget() - mCamera.GetPosition());
    mSceneParam.additional.x = (u32)mLightGenerationParamTbl.size();
    mSceneParam.additional.y = mIsIndirectOnly ? 1 : 0;
    mSceneParam.additional.z = mIsUseNEE ? 1 : 0;
    mSceneParam.additional.w = mIsUseStreamingRIS ? 1 : 0;
    mSceneParam.cameraParams = XMVectorSet(0.1f, 100.f, (f32)min(mRecursionDepth, REAL_MAX_RECURSION_DEPTH), 0);
    mSceneParam.additional1.x = mIsUseReservoirTemporalReuse ? 1 : 0;
    mSceneParam.additional1.y = mIsUseReservoirSpatialReuse ? 1 : 0;
    mSceneParam.additional1.z = mIsUseMetallicTest ? 1 : 0;
    mSceneParam.additional1.w = mIsHistoryResetRequested ? 1 : 0;
    mSceneParam.sssParam = XMVectorSet(mMeanFreePathRatio * mMeanFreePath, 0, 0, 0);
    mSceneParam.additional2.x = mIsUseIBL ? 1 : 0;

    mRenderFrame++;
    mSeedFrame++;

    if (mRenderFrame >= (UINT_MAX >> 1))
    {
        mRenderFrame = 0;
    }
    if (mSeedFrame >= (UINT_MAX >> 1))
    {
        mSeedFrame = 0;
    }

    UpdateWindowText();
    mIsUseAccumulation = true;
    mIsHistoryResetRequested = false;
}

void DxrPhotonMapper::OnKeyDown(UINT8 wparam)
{
    const f32 clampRange = (mStageType == StageType_Plane) ? 1.5f * PLANE_SIZE : 0.9f * PLANE_SIZE;

    mCamera.OnKeyDown(wparam);

    switch (wparam)
    {
    case 'I':
        mInverseMove = !mInverseMove;
        break;
    case 'F':
        mIsUseManySphereLightLighting = !mIsUseManySphereLightLighting;
        mIsUseAccumulation = false;
        mIsHistoryResetRequested = true;
        break;
    case 'E':
#ifdef NEE_AVAILABLE
        mIsUseNEE = !mIsUseNEE;
#endif
        mIsUseAccumulation = false;
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
        mLightRange = Clamp(0.1f, 10.0f, mLightRange + (mInverseMove ? -0.1f : 0.1f));
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
        mIntenceBoost = Clamp(1, 10000, mIntenceBoost + (mInverseMove ? -10 : 10));
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
    case 'V':
        //mVisualizeLightRange = !mVisualizeLightRange;
        mIsUseDebugView = !mIsUseDebugView;
        //mIsUseAccumulation = false;
        break;
    case 'A':
        mIsIndirectOnly = !mIsIndirectOnly;
        mIsUseAccumulation = false;
        break;
    case 'O':
        mThetaDirectional += mInverseMove ? -1 : 1;
        mIsUseAccumulation = false;
        break;
    case 'W':
        mPhiDirectional += mInverseMove ? -1 : 1;
        mIsUseAccumulation = false;
        break;
    case 'N':
        mIsApplyCaustics = !mIsApplyCaustics;
        mIsUseAccumulation = false;
        break;
    case 'D':
        //mIsUseDenoise = !mIsUseDenoise;
        mRecursionDepth = (u32)Clamp(1.f, REAL_MAX_RECURSION_DEPTH * 1.0f, (f32)(mRecursionDepth + (mInverseMove ? -1 : 1)));
        mIsUseAccumulation = false;
        break;
    case 'Q':
        mCausticsBoost = Clamp(0.0001f, 20.0f, mCausticsBoost + (mInverseMove ? -0.0001f : 0.0001f));
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
            mMaterialParam0.roughness = Clamp(0.02f, 1, mMaterialParam0.roughness + (mInverseMove ? -0.01f : 0.01f));
            mIsUseAccumulation = false;
        }
        else
        {
            mMaterialParam1.roughness = Clamp(0.02f, 1, mMaterialParam1.roughness + (mInverseMove ? -0.01f : 0.01f));
            mIsUseAccumulation = false;
        }
        break;
    case 'S':
        if (mIsTargetGlass)
        {
            mMaterialParam0.transRatio = Clamp(0, 1, mMaterialParam0.transRatio + (mInverseMove ? -0.1f : 0.1f));
            mIsUseAccumulation = false;
        }
        else
        {
            mMaterialParam1.transRatio = Clamp(0, 1, mMaterialParam1.transRatio + (mInverseMove ? -0.1f : 0.1f));
            mIsUseAccumulation = false;
        }
        break;
    case 'M':
        if (mIsTargetGlass)
        {
            mMaterialParam0.metallic = Clamp(0, 1, mMaterialParam0.metallic + (mInverseMove ? -0.1f : 0.1f));
            mIsUseAccumulation = false;
        }
        else
        {
            mMaterialParam1.metallic = Clamp(0, 1, mMaterialParam1.metallic + (mInverseMove ? -0.1f : 0.1f));
            mIsUseAccumulation = false;
        }
        break;
    case VK_SPACE:
        mIsTargetGlass = !mIsTargetGlass;
        break;
    case VK_CONTROL:
        mIsUseStreamingRIS = !mIsUseStreamingRIS;
        mIsUseAccumulation = false;
        break;
    case VK_TAB:
        mIsTemporalAccumulationForceDisable = !mIsTemporalAccumulationForceDisable;
        mIsUseAccumulation = false;
        break;
    case VK_F1:
        mIsUseReservoirTemporalReuse = !mIsUseReservoirTemporalReuse;
        mIsUseAccumulation = false;
        break;
    case VK_F3:
        mIsUseReservoirSpatialReuse = !mIsUseReservoirSpatialReuse;
        mIsUseAccumulation = false;
        break;
    case VK_F4:
        mSpatialReuseTap = Clamp(1, MAX_SPATIAL_REUSE_TAP, mSpatialReuseTap + (mInverseMove ? -1 : 1));
        mIsUseAccumulation = false;
        break;
    case VK_F5:
        mIsUseMetallicTest = !mIsUseMetallicTest;
        mIsUseAccumulation = false;
        break;
    /*case VK_F6:
        mIsAlbedoOne = !mIsAlbedoOne;
        mIsUseAccumulation = false;
        break;
    case VK_F7:
        if (mInverseMove)
        {
            mMeanFreePathRatio -= 1;
        }
        else
        {
            mMeanFreePathRatio += 1;
        }
        mMeanFreePathRatio = Clamp(1, 100, mMeanFreePathRatio);
        mIsUseAccumulation = false;
        break;*/
    case VK_F6:
        if (mIsTargetGlass)
        {
            if (mMaterialParam0.isSSSExecutable > 0)
            {
                mMaterialParam0.isSSSExecutable = 0;
            }
            else
            {
                mMaterialParam0.isSSSExecutable = 1;
            }
            mIsUseAccumulation = false;
        }
        else
        {
            if (mMaterialParam1.isSSSExecutable > 0)
            {
                mMaterialParam1.isSSSExecutable = 0;
            }
            else
            {
                mMaterialParam1.isSSSExecutable = 1;
            }
            mIsUseAccumulation = false;
        }
        break;
    case VK_F7:
        mIsUseIBL = !mIsUseIBL;
        mIsUseAccumulation = false;
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

void DxrPhotonMapper::InitializeLightGenerateParams()
{
    mLightGenerationParamTbl.resize(0);
    for (u32 i = 0; i < LightCount_Rect; i++)
    {
        LightGenerateParam param;
        param.setParamAsRectLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(1, 0, 0), XMFLOAT3(0, 0, 1));
        mLightGenerationParamTbl.push_back(param);
    }
    for (u32 i = 0; i < LightCount_Spot; i++)
    {
        LightGenerateParam param;
        param.setParamAsSpotLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(1, 0, 0), XMFLOAT3(0, 0, 1));
        mLightGenerationParamTbl.push_back(param);
    }
    for (u32 i = 0; i < LightCount_Sphere; i++)
    {
        if (mIsUseManySphereLightLighting)
        {
            LightGenerateParam param;
            param.setParamAsSphereLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), 1);
            mLightGenerationParamTbl.push_back(param);
        }
    }
    for (u32 i = 0; i < LightCount_Directional; i++)
    {
        LightGenerateParam param;
        param.setParamAsDirectionalLight(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
        mLightGenerationParamTbl.push_back(param);
    }
}

void DxrPhotonMapper::UpdateLightGenerateParams()
{
    XMFLOAT3 colorTbl[] = {
        XMFLOAT3(mIntenceBoost * 1.0f, mIntenceBoost * 0.0f, mIntenceBoost * 0.0f),
        XMFLOAT3(mIntenceBoost * 1.0f, mIntenceBoost * 1.0f, mIntenceBoost * 0.0f),
        XMFLOAT3(mIntenceBoost * 0.0f, mIntenceBoost * 1.0f, mIntenceBoost * 0.0f),
        XMFLOAT3(mIntenceBoost * 0.0f, mIntenceBoost * 0.0f, mIntenceBoost * 1.0f),
        XMFLOAT3(mIntenceBoost * 0.0f, mIntenceBoost * 1.0f, mIntenceBoost * 1.0f),
        XMFLOAT3(mIntenceBoost * 1.0f, mIntenceBoost * 0.8f, mIntenceBoost * 0.0f),
        XMFLOAT3(mIntenceBoost * 0.0f, mIntenceBoost * 1.0f, mIntenceBoost * 0.8f),
        XMFLOAT3(mIntenceBoost * 1.0f, mIntenceBoost * 0.0f, mIntenceBoost * 1.0f),
        XMFLOAT3(mIntenceBoost * 0.8f, mIntenceBoost * 0.0f, mIntenceBoost * 1.0f),
    };

    const f32 scale = mLightRange;
    const u32 prevSize = (u32)mLightGenerationParamTbl.size();
    mLightGenerationParamTbl.resize(0);
    u32 count = 0;
    const f32 cellSize = 2 * 0.9 * PLANE_SIZE / STAGE_DIVISION_FOR_LIGHT_POSITION;
    for (u32 i = 0; i < LightCount_Rect; i++)
    {
        if (i == 0 && !mIsSpotLightPhotonMapper)
        {
            LightGenerateParam param;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, XMVectorSet(sin(mTheta * ONE_RADIAN) * cos(mPhi * ONE_RADIAN), sin(mTheta * ONE_RADIAN) * sin(mPhi * ONE_RADIAN), cos(mTheta * ONE_RADIAN), 0.0f));
            utility::ONB(normal, tangent, bitangent);
            tangent.x *= scale;
            tangent.y *= scale;
            tangent.z *= scale;
            bitangent.x *= scale;
            bitangent.y *= scale;
            bitangent.z *= scale;
            param.setParamAsRectLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), tangent, bitangent);
            mLightGenerationParamTbl.push_back(param);
        }
        else if (!mIsSpotLightPhotonMapper)
        {
            f32 y = mLightPosY;
            f32 x = mStageOffsetX + cellSize * 0.5f + cellSize * (count / STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE;
            f32 z = mStageOffsetZ + cellSize * 0.5f + cellSize * (count % STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE;
            LightGenerateParam param;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, XMVectorSet(sin(mTheta * ONE_RADIAN) * cos(mPhi * ONE_RADIAN), sin(mTheta * ONE_RADIAN) * sin(mPhi * ONE_RADIAN), cos(mTheta * ONE_RADIAN), 0.0f));
            utility::ONB(normal, tangent, bitangent);
            tangent.x *= scale;
            tangent.y *= scale;
            tangent.z *= scale;
            bitangent.x *= scale;
            bitangent.y *= scale;
            bitangent.z *= scale;
            param.setParamAsRectLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), tangent, bitangent);
            mLightGenerationParamTbl.push_back(param);
        }
        count++;
    }
    count = 0;
    for (u32 i = 0; i < LightCount_Spot; i++)
    {
        if (i == 0 && mIsSpotLightPhotonMapper)
        {
            LightGenerateParam param;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, XMVectorSet(sin(mTheta * ONE_RADIAN) * cos(mPhi * ONE_RADIAN), sin(mTheta * ONE_RADIAN) * sin(mPhi * ONE_RADIAN), cos(mTheta * ONE_RADIAN), 0.0f));
            utility::ONB(normal, tangent, bitangent);
            tangent.x *= scale;
            tangent.y *= scale;
            tangent.z *= scale;
            bitangent.x *= scale;
            bitangent.y *= scale;
            bitangent.z *= scale;
            param.setParamAsSpotLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), tangent, bitangent);
            mLightGenerationParamTbl.push_back(param);
        }
        else if (mIsSpotLightPhotonMapper)
        {
            f32 y = mLightPosY;
            f32 x = mStageOffsetX + cellSize * 0.5f + cellSize * (count / STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE;
            f32 z = mStageOffsetZ + cellSize * 0.5f + cellSize * (count % STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE;
            LightGenerateParam param;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, XMVectorSet(sin(mTheta * ONE_RADIAN) * cos(mPhi * ONE_RADIAN), sin(mTheta * ONE_RADIAN) * sin(mPhi * ONE_RADIAN), cos(mTheta * ONE_RADIAN), 0.0f));
            utility::ONB(normal, tangent, bitangent);
            tangent.x *= scale;
            tangent.y *= scale;
            tangent.z *= scale;
            bitangent.x *= scale;
            bitangent.y *= scale;
            bitangent.z *= scale;
            param.setParamAsSpotLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), tangent, bitangent);
            mLightGenerationParamTbl.push_back(param);
        }
        count++;
    }
    count = 0;
    u32 colorOffset = 0;
    std::mt19937 mt;
    std::uniform_int_distribution rnd(0, 15);
    for (u32 i = 0; i < LightCount_Sphere; i++)
    {
        u32 colorIndex = rnd(mt);
        if (mIsUseManySphereLightLighting)
        {
            f32 y = mLightPosY;
            if (i == LightCount_Sphere / 2)
            {
                count = 0;
                colorOffset++;
            }

            if (i > LightCount_Sphere / 2)
            {
                y = mLightPosY + 15;
            }
           
            f32 x = mStageOffsetX + cellSize * 0.5f + cellSize * (count / STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE;
            f32 z = mStageOffsetZ + cellSize * 0.5f + cellSize * (count % STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE;
            LightGenerateParam param;
            //param.setParamAsSphereLight(XMFLOAT3(x, y, z), colorTbl[colorIndex % _countof(colorTbl)], mLightRange * SPHERE_LIGHTS_SIZE_RATIO);
            param.setParamAsSphereLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), mLightRange* SPHERE_LIGHTS_SIZE_RATIO);
            //param.setParamAsSphereLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost * 0.4), mLightRange * SPHERE_LIGHTS_SIZE_RATIO);
            //param.setParamAsSphereLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntenceBoost, mIntenceBoost, mIntenceBoost), 10, 150);
            mLightGenerationParamTbl.push_back(param);
        }
        count++;
    }
    count = 0;
    for (u32 i = 0; i < LightCount_Directional; i++)
    {
        LightGenerateParam param;
        XMFLOAT3 direction;
        XMStoreFloat3(&direction, XMVectorSet(sin(mThetaDirectional * ONE_RADIAN) * cos(mPhiDirectional * ONE_RADIAN), sin(mThetaDirectional * ONE_RADIAN) * sin(mPhiDirectional * ONE_RADIAN), cos(mThetaDirectional * ONE_RADIAN), 0.0f));
        param.setParamAsDirectionalLight(direction, XMFLOAT3(DIRECTIONAL_LIGHT_POWER * mIntenceBoost, DIRECTIONAL_LIGHT_POWER * mIntenceBoost, DIRECTIONAL_LIGHT_POWER * mIntenceBoost));
        mLightGenerationParamTbl.push_back(param);
    }

    if (prevSize != mLightGenerationParamTbl.size())
    {
        CreateLightGenerationBuffer();
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

f32 DxrPhotonMapper::getFrameRate()
{
    return 1000.0f * (mEndTime.QuadPart - mStartTime.QuadPart) / mCpuFreq.QuadPart;
}