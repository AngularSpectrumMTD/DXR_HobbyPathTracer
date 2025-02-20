#include "DXRPathTracer.h"

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
#define MAX_ACCUMULATION_RANGE 10000
#define DIRECTIONAL_LIGHT_POWER 2

#define NEE_AVAILABLE

//#define USE_SPATIAL_RESERVOIR_FEEDBACK

//#define CUBE_TEST

//This Program supports TRIANGULAR POLYGON only
//If u wanna see beautiful caustics, polygon normal must be smooth!!!
DXRPathTracer::DXRPathTracer(u32 width, u32 height) : AppBase(width, height, L"HobbyTracer"),
mMeshStage(), mMeshSphere(), mMeshBox(), mDispatchRayDesc(), mSceneParam(),
mNormalSphereMaterialTbl()
{
}

void DXRPathTracer::UpdateWindowText()
{
    std::wstringstream windowText;
    windowText.str(L"");

    windowText 
        << L" <I> : Inv " << (mInverseMove ? L"ON" : L"OFF")
        << L" <A> : Acc " << (mIsUseTemporalAccumulation ? L"ON" : L"OFF")
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

void DXRPathTracer::Setup()
{
    mSceneType = SceneType_PTTestRoom;

    mIsUseIBL = true;
    mRecursionDepth = min(5, REAL_MAX_RECURSION_DEPTH);
    mIntensityBoost = 300;
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
    mModelMovingRange = 8;
    mCausticsBoost = 0.005f;
    mIsMoveModel = false;
    mIsApplyCaustics = false;
    mIsUseDenoise = false;
    mIsDebug = true;
    mVisualizeLightRange = false;
    mInverseMove = false;
    mIsUseTexture = true;
    mIsUseDebugView = false;
    mTargetModelIndex = 0;
    mIsUseTemporalAccumulation = false;
    mIsIndirectOnly = false;
    mIsUseNEE = true;
    mIsUseManySphereLightLighting = false;
    mStageTextureFileName = L"model/tileTex.png";
    //mCubeMapTextureFileName = L"model/ParisEquirec.png";
    mCubeMapTextureFileName = L"model/SkyEquirec.png";
    //mCubeMapTextureFileName = L"model/ForestEquirec.png";
    mIsUseStreamingRIS = true;
    mIsUseReservoirTemporalReuse = true;
    mIsUseReservoirSpatialReuse = false;
    mIsTemporalAccumulationForceDisable = true;
    mIsUseEmissivePolygon = true;

    mInitTargetPos = XMFLOAT3(0, 0, 0);

    mLightCount = LightCount_ALL - 1;

    mGroundTex = utility::LoadTextureFromFile(mDevice, mStageTextureFileName);
    mCubeMapTex = utility::LoadTextureFromFile(mDevice, mCubeMapTextureFileName);

    if (mGroundTex.res != nullptr)
    {
        mStageMaterial.hasDiffuseTex = 1;
    }

    mStageType = StageType_Plane;
    mModelTypeTbl[1] = ModelType::ModelType_SimpleCube;

    switch (mSceneType)
    {
        case SceneType_Simple :
        {
            mOBJFileName = "diamond.obj";
            mOBJFolderName = "model";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(30, 30, 30), XMMatrixTranslation(0, -55, 0));
            mStageOffsetX = 0;
            mStageOffsetY = -55.0f;
            mStageOffsetZ = 0;
            mLightPosX = -16.0f; mLightPosY = -172.0f; mLightPosZ = -4.2f;
            mPhi = 149.0f; mTheta = 257.0f;
            mPhiDirectional = 70.0f; mThetaDirectional = 220.0f;
            mInitEyePos = XMFLOAT3(188.0f, -163.0f, -101.0f);
            mInitTargetPos = XMFLOAT3(0.0f, -132.0f, 0.0f);
            mLightRange = 10.0f;
            mModelTypeTbl[0] = ModelType_Afrodyta;
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
                mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(-20, 0, 10));
                mStageOffsetX = -20.0f;
                mStageOffsetY = 0.0f;
                mStageOffsetZ = 10.0f;
            }
            else
            {
                mOBJFileName = "sponza.obj";
                mOBJFolderName = "model/sponza";
                mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(0, 0, 0));
                mStageOffsetX = 0.0f;
                mStageOffsetY = 0.0f;
                mStageOffsetZ = 0.0f;
            }
            
            if (isRoomTestDebug)
            {
                mLightPosX = 1.99f; mLightPosY = 2.8f; mLightPosZ = 4.9f;
                mPhi = 306.0f; mTheta = 187.0f;
                mLightRange = 3.18f;
                mModelTypeTbl[0] = ModelType_CurvedMesh;
                //mModelTypeTbl[0] = ModelType_DebugMesh;
            }
            else
            {
                mLightPosX = 1.59f; mLightPosY = 9.8f; mLightPosZ = 3.19f;
                mPhi = 413.0f; mTheta = 242.0f;
                mLightRange = 0.79f;
                if (isDebugMeshTest)
                {
                    mModelTypeTbl[0] = ModelType_DebugMesh;
                }
                else
                {
                    mLightPosX = 0.1f; mLightPosY = 6.59f; mLightPosZ = 3.4f;
                    mPhi = 447.0f; mTheta = 206.0f;
                    mModelTypeTbl[0] = ModelType_Diamond;
                    mInitEyePos = XMFLOAT3(-20.0f, 19.0f, 2.4f);
                    if (isAfrodytaTest)
                    {
                        mLightPosX = -3.1f; mLightPosY = 12.19f; mLightPosZ = 1.79f;
                        mPhi = 334.0f; mTheta = 111.0f;
                        //mModelTypeTbl[0] = ModelType_Afrodyta;
                        mModelTypeTbl[0] = ModelType_Dragon;
                        //mModelTypeTbl[0] = ModelType_SimpleCube;
                        mPhiDirectional = 100.0f; mThetaDirectional = 261.0f;
                        mInitEyePos = XMFLOAT3(38.6f, 14.23, -1.55f);
                        mInitTargetPos = XMFLOAT3(12.37f, 7.95, -7.3f);
                        mCausticsBoost = 0.014;
                        mLightRange = 0.29f;

                        mModelTypeTbl[0] = ModelType_Afrodyta;
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
            mPhiDirectional = 414; mThetaDirectional = 302;
            mOBJFileName = "exterior.obj";
            mOBJFolderName = "model/bistro/Exterior";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
            mStageOffsetX = 20;
            mStageOffsetY = 0;
            mStageOffsetZ = 0;
            mLightPosX = -3.18f; mLightPosY = 8.2f; mLightPosZ = -2.08f;
            mPhi = 299; mTheta = 395;
            mInitEyePos = XMFLOAT3(-20, 9.97, -5.73);
            mInitTargetPos = XMFLOAT3(14.4, 7.5, -4.0);
            const bool isDragonTest = true;
            if (isDragonTest)
            {
#ifdef CUBE_TEST
                mInitTargetPos = XMFLOAT3(3.75, -1.03, -7.19);
                mInitEyePos = XMFLOAT3(-17, 23, -28);
                mLightRange = 2.29f;
#else
                mInitEyePos = XMFLOAT3(-20, 9.97, -5.73);
                mInitTargetPos = XMFLOAT3(14.4, 7.5, -4.0);
#endif
                mModelTypeTbl[0] = ModelType_Dragon;
            }
            else
            {
                mPhi = 412; mTheta = 262;
                mLightPosX = 4.2f; mLightPosY = 8.8f; mLightPosZ = 0.2f;
                mLightRange = 0.8f;
                mModelTypeTbl[0] = ModelType_Afrodyta;
            }
            mIsSpotLightPhotonMapper = true;
        }
        break;
        case SceneType_BistroInterior:
        {
            mPhiDirectional = 150; mThetaDirectional = 225;
            mOBJFileName = "interior.obj";
            mOBJFolderName = "model/bistro/Interior";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
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
                mLightPosX = 41; mLightPosY = 11.3f; mLightPosZ = -24.1f;
                mPhi = 371; mTheta = -84;
                mInitEyePos = XMFLOAT3(28, 12, 0.2);
                mInitTargetPos = XMFLOAT3(69, 9.36, -5.2f);
                mLightRange = 1.2f;
            }
            
            mModelTypeTbl[0] = ModelType_Afrodyta;
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
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(1, 1, 1), XMMatrixTranslation(20, 0, 0));
            mStageOffsetX = 20;
            mStageOffsetY = 0;
            mStageOffsetZ = 0;
            mLightPosX = 53; mLightPosY = 11.3f; mLightPosZ = -5.1f;
            mPhi = 376; mTheta = 107;
            mInitEyePos = XMFLOAT3(30, 12, 9);
            mInitTargetPos = XMFLOAT3(66, 10, -11.41f);
            mLightRange = 3.68f;
            mModelTypeTbl[0] = ModelType_Afrodyta;
            mIsSpotLightPhotonMapper = false;
        }
        break;
        case SceneType_Room:
        {
            mPhiDirectional = 41.0f; mThetaDirectional = 245.0f;
            mInitEyePos = XMFLOAT3(-7.69f, 4.48, 18.3f);
            mInitTargetPos = XMFLOAT3(23.13, 7.58, -0.12f);

            mOBJFileName = "roomTestExp.obj";
            mOBJFolderName = "model/roomTestExp";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(3.5, 3.5, 3.5), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
        }
        break;
        case SceneType_GITest:
        {
            mPhiDirectional = 123.0f; mThetaDirectional = 326.0f;
            mInitEyePos = XMFLOAT3(-45.85f, 46.655, 123.07f);
            mInitTargetPos = XMFLOAT3(-25.78, 39.42, 94.028f);

            mOBJFileName = "GITest.obj";
            mOBJFolderName = "model/GITest";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
        }
        break; 
        case SceneType_Kitchen:
        {
            mPhiDirectional = 8.0f; mThetaDirectional = 215.0f;
            mInitEyePos = XMFLOAT3(347, 46.51, -161.34f);
            mInitTargetPos = XMFLOAT3(319.1, 46.58, -142.253f);

            mOBJFileName = "Kitchen.obj";
            mOBJFolderName = "model/Kitchen";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
            mIntensityBoost *= 3;
        }
        break;
        case SceneType_PTTest:
        {
            mPhiDirectional = 51.0f; mThetaDirectional = 293.0f;
            mInitEyePos = XMFLOAT3(334.00f, 81.915, 161.07f);
            mInitTargetPos = XMFLOAT3(300.78, 79.42, 147.028f);

            mOBJFileName = "PTTest.obj";
            mOBJFolderName = "model/PTTest";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 0.79f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
        }
        break;
        case SceneType_PTTestBrick:
        {
            mLightAreaScale = 6;
            mPhiDirectional = 51.0f; mThetaDirectional = 234;

            //near
            //mInitEyePos = XMFLOAT3(-85, 64, -18);
            //mInitTargetPos = XMFLOAT3(-73.4,68, -52);

            //far
            mInitEyePos = XMFLOAT3(166, 121, -375);
            mInitTargetPos = XMFLOAT3(132, 108,-328);

            mOBJFileName = "PTTestBrick.obj";
            mOBJFolderName = "model/PTTest";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(35, 35, 35), XMMatrixTranslation(-150, 0, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 4.0f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
            mCameraSpeed = 10.0f;
        }
        break;
        case SceneType_PTTestRobot:
        {
            mLightAreaScale = 6;
            mPhiDirectional = 110.0f; mThetaDirectional = 287;

            //near
            //mInitEyePos = XMFLOAT3(-85, 64, -18);
            //mInitTargetPos = XMFLOAT3(-73.4,68, -52);

            //far
            mInitEyePos = XMFLOAT3(371, 141, -129);
            mInitTargetPos = XMFLOAT3(314, 139, -114);

            mOBJFileName = "PTTestRobot.obj";
            mOBJFolderName = "model/PTTest";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(100, 100, 100), XMMatrixTranslation(-150, 65, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 4.0f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
            mCameraSpeed = 10.0f;
        }
        break;
        case SceneType_PTTestRoom:
        {
            mLightAreaScale = 6;
            mPhiDirectional = 294.0f; mThetaDirectional = 147;

            //near
            //mInitEyePos = XMFLOAT3(-85, 64, -18);
            //mInitTargetPos = XMFLOAT3(-73.4,68, -52);
                
            //far
            mInitEyePos = XMFLOAT3(27.0, 137, 64.5);
            mInitTargetPos = XMFLOAT3(-18, 135, 22.5);

            mOBJFileName = "PTTestRoom.obj";
            mOBJFolderName = "model/PTTest";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(10, 10, 10), XMMatrixTranslation(-150, 65, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = -1.21f; mLightPosY = 18.0f; mLightPosZ = 12.78f;
            mPhi = 46.0f; mTheta = 239.0f;

            mLightRange = 4.0f;

            mModelTypeTbl[0] = ModelType_Afrodyta;
            mCameraSpeed = 10.0f;
        }
        break;
        case SceneType_MaterialTest:
        {
            mLightAreaScale = 6;
            mPhiDirectional = 51.0f; mThetaDirectional = 234;

            //near
            //mInitEyePos = XMFLOAT3(-85, 64, -18);
            //mInitTargetPos = XMFLOAT3(-73.4,68, -52);

            //far
            mInitEyePos = XMFLOAT3(75.9, 142, -48);
            mInitTargetPos = XMFLOAT3(33.5, 115, -17.4);

            mOBJFileName = "MaterialTest.obj";
            mOBJFolderName = "model/MaterialTest";
            mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(15, 15, 15), XMMatrixTranslation(0, 70, 0));
            mStageOffsetX = 0.0f;
            mStageOffsetY = 0.0f;
            mStageOffsetZ = 0.0f;

            mLightPosX = 10.36f; mLightPosY = 118.0f; mLightPosZ = 11.49f;
            mPhi = -83.0f; mTheta = 121.0f;

            mLightRange = 1.2f;

            mCausticsBoost = 0.05;
            mGatherBlockRange = 2;

            mModelTypeTbl[0] = ModelType_Afrodyta;
            mCameraSpeed = 10.0f;

            mIsUseDirectionalLight = false;

            mGatherRadius = 0.5f;

            mIsApplyCaustics = true;
        }
        break;
    }

#ifdef GI_TEST
    mIsApplyCaustics = false;
    mIsIndirectOnly = true;
    mPhiDirectional = 111; mThetaDirectional = 250;
    mOBJFileName = "interior.obj";
    mOBJFolderName = "model/bistro/Interior";
    mOBJMaterialLinkedMeshTRS = XMMatrixMultiply(XMMatrixScaling(0.5, 0.5, 0.5), XMMatrixTranslation(20, 0, 0));
    mStageOffsetX = 20;
    mStageOffsetY = 0;
    mStageOffsetZ = 0;
    mLightPosX = 53; mLightPosY = 3.7f; mLightPosZ = -5.9f;
    mPhi = 283; mTheta = 107;
    mInitEyePos = XMFLOAT3(30, 12, 9);
    mInitTargetPos = XMFLOAT3(66, 10, -11.41f);
    mLightRange = 2.98f;

    mModelTypeTbl[0] = ModelType_Afrodyta;
    mIsSpotLightPhotonMapper = false;
    mCausticsBoost = 0.0002f;
    mGatherRadius = 0.011f;

    mIsTemporalAccumulationForceDisable = true;
#endif

    switch (mModelTypeTbl[0])
    {
    case  ModelType::ModelType_Crab:
    {
        mOBJFileNameTbl[0] = L"model/crab.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(12, 12, 12);
    }
    break;
    case ModelType::ModelType_TwistCube:
    {
        mOBJFileNameTbl[0] = L"model/twistCube.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(5, 5, 5);
    }
    break;
    case ModelType::ModelType_SimpleCube:
    {
        mOBJFileNameTbl[0] = L"model/simpleCube.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(5, 5, 5);
    }
    break;
    case ModelType::ModelType_Buddha:
    {
        mOBJFileNameTbl[0] = L"model/buddha/deciBuddha.obj";//test
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(15, 15, 15);//test
        //mObjScaleTbl[0] = XMFLOAT3(0.1, 0.1, 0.1);//test
    }
    break;
    case ModelType::ModelType_Teapot:
    {
        mOBJFileNameTbl[0] = L"model/teapot.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(2, 2, 2);
    }
    break;
    case  ModelType::ModelType_LikeWater:
    {
        mOBJFileNameTbl[0] = L"model/likeWater.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(4, 4, 4);
    }
    break;
    case  ModelType::ModelType_Ocean:
    {
        mStageType = StageType_Box;
        mModelMovingRange *= 2;
        mOBJFileNameTbl[0] = L"model/ocean.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case  ModelType::ModelType_Ocean2:
    {
        mStageType = StageType_Box;
        mModelMovingRange *= 2;
        mOBJFileNameTbl[0] = L"model/ocean2.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case  ModelType::ModelType_CurvedMesh:
    {
        mOBJFileNameTbl[0] = L"model/curvedMesh.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(4, 4, 4);
    }
    break;
    case ModelType::ModelType_Diamond:
    {
        mOBJFileNameTbl[0] = L"model/diamond.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(2, 2, 2);
    }
    break;
    case ModelType::ModelType_Skull:
    {
        mOBJFileNameTbl[0] = L"model/skull.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(30, 30, 30);
    }
    break;
    case  ModelType::ModelType_HorseStatue:
    {
        mOBJFileNameTbl[0] = L"model/horse_statue_Tri.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(350, 350, 350);
    }
    break;
    case  ModelType::ModelType_Dragon:
    {
        mOBJFileNameTbl[0] = L"model/dragon.obj";
        mObjYOffsetTbl[0] = 11;
        if (mSceneType == SceneType_BistroExterior)
        {
#ifndef CUBE_TEST
            mObjYOffsetTbl[0] = 4;
#endif
        }
        mObjScaleTbl[0] = XMFLOAT3(12, 12, 12);
    }
    break;
    case  ModelType::ModelType_Afrodyta:
    {
        mOBJFileNameTbl[0] = L"model/aphorodite/Tri_Deci_Rz_123_Afrodyta_z_Melos.obj";
        mObjYOffsetTbl[0] = 8;
        mObjScaleTbl[0] = XMFLOAT3(0.1f, 0.1f, 0.1f);
        if (mSceneType == SceneType_Room)
        {
            mObjYOffsetTbl[0] = 16;
            mObjScaleTbl[0] = XMFLOAT3(0.02f, 0.02f, 0.02f);
        }
        if (mSceneType == SceneType_PTTestBrick)
        {
            mObjYOffsetTbl[0] = 14;
        }
    }
    break;
    case  ModelType::ModelType_Rock:
    {
        mOBJFileNameTbl[0] = L"model/rock.obj";
        mObjYOffsetTbl[0] = 8;
        mObjScaleTbl[0] = XMFLOAT3(5, 5, 5);
    }
    break;
    case  ModelType::ModelType_DebugMesh:
    {
        mOBJFileNameTbl[0] = L"model/debugMesh.obj";
        mObjYOffsetTbl[0] = 8;
        mObjScaleTbl[0] = XMFLOAT3(5, 5, 5);
    }
    break;
    default:
    {
        mOBJFileNameTbl[0] = L"model/crab.obj";
        mObjYOffsetTbl[0] = 5;
        mObjScaleTbl[0] = XMFLOAT3(12, 12, 12);
    }
    break;
    }

    switch (mModelTypeTbl[1])
    {
    case  ModelType::ModelType_Crab:
    {
        mOBJFileNameTbl[1] = L"model/crab.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
    }
    break;
    case ModelType::ModelType_TwistCube:
    {
        mOBJFileNameTbl[1] = L"model/twistCube.obj";
        mObjYOffsetTbl[1] = (mSceneType == SceneType_BistroExterior) ? 15.0f :  50.0f;
        mObjScaleTbl[1] = XMFLOAT3(3, 3, 3);

        if (mSceneType == SceneType_Simple)
        {
            mObjYOffsetTbl[1] = 90;
            mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
        }
        else if (mSceneType == SceneType_Sponza)
        {
            mObjYOffsetTbl[1] = 90;
            mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
        }
#ifdef CUBE_TEST
        mObjYOffsetTbl[1] = 10;//test
        mObjScaleTbl[1] = XMFLOAT3(6, 6, 6);//test
#endif
    }
    break;
    case ModelType::ModelType_SimpleCube:
    {
        mOBJFileNameTbl[1] = L"model/simpleCube.obj";
        mObjYOffsetTbl[1] = (mSceneType == SceneType_BistroExterior) ? 15.0f : 50.0f;
        mObjScaleTbl[1] = XMFLOAT3(3, 3, 3);

        if (mSceneType == SceneType_Simple)
        {
            mObjYOffsetTbl[1] = 90;
            mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
        }
        else if (mSceneType == SceneType_Sponza)
        {
            mObjYOffsetTbl[1] = 90;
            mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
        }
        else if (mSceneType == SceneType_BistroInterior)
        {
            mObjYOffsetTbl[1] = 90;
            mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
        }
        else
        {
#ifdef CUBE_TEST
            mObjYOffsetTbl[1] = 10;//test
            mObjScaleTbl[1] = XMFLOAT3(6, 6, 6);//test
#endif
        }
    }
    break;
    case ModelType::ModelType_Teapot:
    {
        mOBJFileNameTbl[1] = L"model/teapot.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(2, 2, 2);
    }
    break;
    case  ModelType::ModelType_LikeWater:
    {
        mOBJFileNameTbl[1] = L"model/likeWater.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(2, 4, 4);
    }
    break;
    case  ModelType::ModelType_Ocean:
    {
        mStageType = StageType_Box;
        mOBJFileNameTbl[1] = L"model/ocean.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case  ModelType::ModelType_Ocean2:
    {
        mStageType = StageType_Box;
        mOBJFileNameTbl[1] = L"model/ocean2.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f, PLANE_SIZE * 0.99f);
    }
    break;
    case  ModelType::ModelType_CurvedMesh:
    {
        mOBJFileNameTbl[1] = L"model/curvedMesh.obj";
        mObjYOffsetTbl[1] = 5;
        mObjScaleTbl[1] = XMFLOAT3(4, 4, 4);
    }
    break;
    case ModelType::ModelType_Diamond:
    {
        mOBJFileNameTbl[1] = L"model/diamond.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(2, 2, 2);
    }
    break;
    case ModelType::ModelType_Skull:
    {
        mOBJFileNameTbl[1] = L"model/skull.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(30, 30, 30);
    }
    break;
    case  ModelType::ModelType_DebugMesh:
    {
        mOBJFileNameTbl[1] = L"model/debugMesh.obj";
        mObjYOffsetTbl[1] = 8;
        mObjScaleTbl[1] = XMFLOAT3(5, 5, 5);
    }
    break;
    default:
    {
        mOBJFileNameTbl[1] = L"model/crab.obj";
        mObjYOffsetTbl[1] = 15;
        mObjScaleTbl[1] = XMFLOAT3(12, 12, 12);
    }
    break;
    }

    if (mSceneType == SceneType_Simple)
    {
        mObjYOffsetTbl[0] -= 180;
        mObjScaleTbl[0].x *= 3;
        mObjScaleTbl[0].y *= 3;
        mObjScaleTbl[0].z *= 3;

        mObjYOffsetTbl[1] -= 150;
        mObjScaleTbl[1].x *= 3;
        mObjScaleTbl[1].y *= 3;
        mObjScaleTbl[1].z *= 3;
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

void DXRPathTracer::Initialize()
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
        CreateShaderTables();

        mCommandList = mDevice->CreateCommandList();
        mCommandList->Close();

        InitializeCamera();
    }
    else
    {
        throw std::runtime_error("Failed CoInitializeEx.");
    }
}

void DXRPathTracer::Draw()
{
    QueryPerformanceFrequency(&mCpuFreq);

    auto renderTarget = mDevice->GetRenderTarget();
    auto allocator = mDevice->GetCurrentCommandAllocator();
    allocator->Reset();
    mCommandList->Reset(allocator.Get(), nullptr);
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    const u32 prev = mSeedFrame % 2;
    const u32 curr = (mSeedFrame + 1) % 2;

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

    CD3DX12_RESOURCE_BARRIER uavBRandom[] = {
      CD3DX12_RESOURCE_BARRIER::UAV(mRandomNumberBuffer.Get()),
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

    mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

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
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigPhoton["gRandomNumber"], mRandomNumberBufferDescriptorUAV.hGpu);
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
    mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

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
    mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSig["gRandomNumber"], mRandomNumberBufferDescriptorUAV.hGpu);
    mCommandList->SetPipelineState1(mRTPSO.Get());
    PIXBeginEvent(mCommandList.Get(), 0, "PathTracing");
    mCommandList->DispatchRays(&mDispatchRayDesc);
    PIXEndEvent(mCommandList.Get());

    mCommandList->ResourceBarrier(u32(_countof(uavBarriers)), uavBarriers);
    mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

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
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirTemporalReuse["gRandomNumber"], mRandomNumberBufferDescriptorUAV.hGpu);
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

    mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

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

        mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

        u32 spatialIDPing = curr;
        u32 spatialIDPong = prev;

        for (int i = 0; i < mSpatialReuseTap; i++)
        {
            ReSTIRParam cb;
            XMUINT4 d;
            d.x = max(1, 1 + 2 *  i);//DIReservoirSpatialReuseNum
            d.y = max(1, 1 + 1 * i);//GIReservoirSpatialReuseNum
            d.z = max(1, 1 + 1 * i);//DIReservoirSpatialReuseBaseRadius
            d.w = max(1, 1 + 1 * i);//GIReservoirSpatialReuseBaseRadius
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
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapGlobalRootSigReservoirSpatialReuse["gRandomNumber"], mRandomNumberBufferDescriptorUAV.hGpu);
            mCommandList->SetComputeRootConstantBufferView(mRegisterMapGlobalRootSigReservoirSpatialReuse["gReSTIRParam"], mReSTIRParamCBTbl[i]->GetGPUVirtualAddress());
            mCommandList->SetPipelineState1(mRTPSOReservoirSpatialReuse.Get());
            PIXBeginEvent(mCommandList.Get(), 0, "ReservoirSpatialReuse");
            mCommandList->DispatchRays(&mDispatchReservoirSpatialReuseRayDesc);
            PIXEndEvent(mCommandList.Get());

            u32 tmp = spatialIDPing;
            spatialIDPing = spatialIDPong;
            spatialIDPong = tmp;

            mCommandList->ResourceBarrier(u32(_countof(uavB)), uavB);
            mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);
        }

        finalSpatialID = spatialIDPong;

        mCommandList->ResourceBarrier(u32(_countof(uavB)), uavB);
        mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

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

    mCommandList->ResourceBarrier(u32(_countof(uavBRandom)), uavBRandom);

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

void DXRPathTracer::InitializeCamera()
{
    mCamera.SetLookAt(mInitEyePos, mInitTargetPos);

    mSceneParam.cameraParams = XMVectorSet(0.1f, 100.f, REAL_MAX_RECURSION_DEPTH, 0);
    mCamera.SetPerspective(
        XM_PIDIV4, GetAspect(), 0.1f, 100.0f
    );
    mCamera.SetSpeed(mCameraSpeed);
}

void DXRPathTracer::Terminate()
{
    TerminateRenderDevice();
}

void DXRPathTracer::Update()
{
    if (mIsTemporalAccumulationForceDisable)
    {
        mIsUseTemporalAccumulation = false;
    }

    if (!mIsUseTemporalAccumulation)
    {
        mRenderFrame = 0;
    }

    for (auto& pos : mLightTbl)
    {
        pos = XMMatrixTranslation(mLightPosX, mLightPosY, mLightPosZ);
    }

    if (mIsMoveModel)
    {
        u32 count = 0;
        for (auto& pos : mOBJNormalTbl)
        {
            pos = XMMatrixTranslation(0, mObjYOffsetTbl[count] + mModelMovingRange * sin(0.4f * mMoveFrame * ONE_RADIAN), 0);
            count++;
        }

        mMoveFrame++;
        if (mMoveFrame >= (UINT_MAX >> 1))
        {
            mMoveFrame = 0;
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
    mSceneParam.gatherParams2 = XMVectorSet(mStandardPhotonNum, mIsUseTemporalAccumulation ? 1.0f : 0.0f, 0.0f, 0.0f);
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
    mSceneParam.additional2.y = mIsUseEmissivePolygon ? 1 : 0;

    if (!mIsTemporalAccumulationForceDisable)
    {
        mRenderFrame++;
    }
    
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
    mIsUseTemporalAccumulation = true;
    mIsHistoryResetRequested = false;
}

void DXRPathTracer::OnKeyDown(UINT8 wparam)
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
        mIsUseTemporalAccumulation = false;
        mIsHistoryResetRequested = true;
        break;
    case 'E':
#ifdef NEE_AVAILABLE
        mIsUseNEE = !mIsUseNEE;
#endif
        mIsUseTemporalAccumulation = false;
        break;
    case 'J':
        mIsMoveModel = !mIsMoveModel;
        mIsUseTemporalAccumulation = false;
        break;
    case 'G':
        mGatherRadius = Clamp(0.001f, max(0.05f, (2.f * PLANE_SIZE) / GRID_DIMENSION), mGatherRadius + (mInverseMove ? -0.01f : 0.01f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'X':
        mLightPosX = Clamp(-clampRange, clampRange, mLightPosX + (mInverseMove ? -PLANE_SIZE * 0.002f : PLANE_SIZE * 0.002f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'Y':
        mLightPosY = Clamp(-clampRange, clampRange, mLightPosY + (mInverseMove ? -PLANE_SIZE * 0.002f : PLANE_SIZE * 0.002f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'Z':
        mLightPosZ = Clamp(-clampRange, clampRange, mLightPosZ + (mInverseMove ? -PLANE_SIZE * 0.002f : PLANE_SIZE * 0.002f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'L':
        mLightRange = Clamp(0.1f, 10.0f, mLightRange + (mInverseMove ? -0.1f : 0.1f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'T':
        mTheta += mInverseMove ? -1 : 1;
        mIsUseTemporalAccumulation = false;
        break;
    case 'P':
        mPhi += mInverseMove ? -1 : 1;
        mIsUseTemporalAccumulation = false;
        break;
    case 'K':
        mIntensityBoost = Clamp(1, 10000, mIntensityBoost + (mInverseMove ? -10 : 10));
        mIsUseTemporalAccumulation = false;
        break;
    case 'B':
        mGatherBlockRange = (u32)Clamp(0, 3, (f32)mGatherBlockRange + (mInverseMove ? -1 : 1));
        mIsUseTemporalAccumulation = false;
        break;
    case 'C':
        mIsSpotLightPhotonMapper = !mIsSpotLightPhotonMapper;
        mIsUseTemporalAccumulation = false;
        break;
    case 'V':
        //mVisualizeLightRange = !mVisualizeLightRange;
        mIsUseDebugView = !mIsUseDebugView;
        //mIsUseTemporalAccumulation = false;
        break;
    case 'A':
        mIsIndirectOnly = !mIsIndirectOnly;
        mIsUseTemporalAccumulation = false;
        mIsHistoryResetRequested = true;
        break;
    case 'O':
        mThetaDirectional += mInverseMove ? -1 : 1;
        mIsUseTemporalAccumulation = false;
        break;
    case 'W':
        mPhiDirectional += mInverseMove ? -1 : 1;
        mIsUseTemporalAccumulation = false;
        break;
    case 'N':
        mIsApplyCaustics = !mIsApplyCaustics;
        mIsUseTemporalAccumulation = false;
        break;
    case 'D':
        //mIsUseDenoise = !mIsUseDenoise;
        mRecursionDepth = (u32)Clamp(1.f, REAL_MAX_RECURSION_DEPTH * 1.0f, (f32)(mRecursionDepth + (mInverseMove ? -1 : 1)));
        mIsUseTemporalAccumulation = false;
        break;
    case 'Q':
        mCausticsBoost = Clamp(0.0001f, 20.0f, mCausticsBoost + (mInverseMove ? -0.0001f : 0.0001f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'U':
        mIsUseTexture = !mIsUseTexture;
        mIsUseTemporalAccumulation = false;
        break;
        //material start
    case 'R':
        mMaterialParamTbl[mTargetModelIndex].roughness = Clamp(0.02f, 1, mMaterialParamTbl[0].roughness + (mInverseMove ? -0.01f : 0.01f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'S':
        mMaterialParamTbl[mTargetModelIndex].transRatio = Clamp(0, 1, mMaterialParamTbl[0].transRatio + (mInverseMove ? -0.1f : 0.1f));
        mIsUseTemporalAccumulation = false;
        break;
    case 'M':
        mMaterialParamTbl[mTargetModelIndex].metallic = Clamp(0, 1, mMaterialParamTbl[0].metallic + (mInverseMove ? -0.1f : 0.1f));
        mIsUseTemporalAccumulation = false;
        break;
    case VK_SPACE:
        mTargetModelIndex = (mTargetModelIndex == 0) ? 1 : 0;
        break;
    case VK_CONTROL:
        mIsUseStreamingRIS = !mIsUseStreamingRIS;
        mIsUseTemporalAccumulation = false;
        break;
    case VK_TAB:
        mIsTemporalAccumulationForceDisable = !mIsTemporalAccumulationForceDisable;
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F1:
        mIsUseReservoirTemporalReuse = !mIsUseReservoirTemporalReuse;
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F3:
        mIsUseReservoirSpatialReuse = !mIsUseReservoirSpatialReuse;
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F4:
        mSpatialReuseTap = Clamp(1, MAX_SPATIAL_REUSE_TAP, mSpatialReuseTap + (mInverseMove ? -1 : 1));
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F5:
        mIsUseMetallicTest = !mIsUseMetallicTest;
        mIsUseTemporalAccumulation = false;
        break;
    /*case VK_F6:
        mIsAlbedoOne = !mIsAlbedoOne;
        mIsUseTemporalAccumulation = false;
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
        mIsUseTemporalAccumulation = false;
        break;*/
    case VK_F6:
        if (mMaterialParamTbl[mTargetModelIndex].isSSSExecutable > 0)
        {
            mMaterialParamTbl[mTargetModelIndex].isSSSExecutable = 0;
        }
        else
        {
            mMaterialParamTbl[mTargetModelIndex].isSSSExecutable = 1;
        }
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F7:
        mIsUseIBL = !mIsUseIBL;
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F8:
        mIsUseDirectionalLight = !mIsUseDirectionalLight;
        mIsUseTemporalAccumulation = false;
        break;
    case VK_F9:
        mIsUseEmissivePolygon = !mIsUseEmissivePolygon;
        mIsUseTemporalAccumulation = false;
        break;
    }
}

void DXRPathTracer::OnMouseDown(MouseButton button, s32 x, s32 y)
{
    f32 fdx = f32(x) / GetWidth();
    f32 fdy = f32(y) / GetHeight();
    mCamera.OnMouseButtonDown(s32(button), fdx, fdy);
}

void DXRPathTracer::OnMouseUp(MouseButton button, s32 x, s32 y)
{
    mCamera.OnMouseButtonUp();
}

void DXRPathTracer::OnMouseMove(s32 dx, s32 dy)
{
    f32 fdx = f32(dx) / GetWidth();
    f32 fdy = f32(dy) / GetHeight();
    mCamera.OnMouseMove(-fdx, fdy);
}

void DXRPathTracer::OnMouseWheel(s32 rotate)
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

f32 DXRPathTracer::Clamp(f32 min, f32 max, f32 src)
{
    return std::fmax(min, std::fmin(src, max));
}

void DXRPathTracer::UpdateSceneParams()
{
    auto sceneConstantBuffer = mSceneCB.Get();
    mDevice->ImmediateBufferUpdateHostVisible(sceneConstantBuffer, &mSceneParam, sizeof(mSceneParam));
}

void DXRPathTracer::UpdateMaterialParams()
{
    auto materialConstantBuffer = mOBJMaterialCB.Get();
    mDevice->ImmediateBufferUpdateHostVisible(materialConstantBuffer, &mMaterialParamTbl, mMaterialParamTbl.size() * sizeof(utility::MaterialParam));
}

void DXRPathTracer::InitializeLightGenerateParams()
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

void DXRPathTracer::UpdateLightGenerateParams()
{
    XMFLOAT3 colorTbl[] = {
        XMFLOAT3(mIntensityBoost * 1.0f, mIntensityBoost * 0.0f, mIntensityBoost * 0.0f),
        XMFLOAT3(mIntensityBoost * 1.0f, mIntensityBoost * 1.0f, mIntensityBoost * 0.0f),
        XMFLOAT3(mIntensityBoost * 0.0f, mIntensityBoost * 1.0f, mIntensityBoost * 0.0f),
        XMFLOAT3(mIntensityBoost * 0.0f, mIntensityBoost * 0.0f, mIntensityBoost * 1.0f),
        XMFLOAT3(mIntensityBoost * 0.0f, mIntensityBoost * 1.0f, mIntensityBoost * 1.0f),
        XMFLOAT3(mIntensityBoost * 1.0f, mIntensityBoost * 0.8f, mIntensityBoost * 0.0f),
        XMFLOAT3(mIntensityBoost * 0.0f, mIntensityBoost * 1.0f, mIntensityBoost * 0.8f),
        XMFLOAT3(mIntensityBoost * 1.0f, mIntensityBoost * 0.0f, mIntensityBoost * 1.0f),
        XMFLOAT3(mIntensityBoost * 0.8f, mIntensityBoost * 0.0f, mIntensityBoost * 1.0f),
    };

    const f32 scale = mLightRange;
    const u32 prevSize = (u32)mLightGenerationParamTbl.size();
    mLightGenerationParamTbl.resize(0);
    u32 count = 0;
    const f32 cellSize = 2 * 0.9 * PLANE_SIZE * mLightAreaScale / STAGE_DIVISION_FOR_LIGHT_POSITION;
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
            param.setParamAsRectLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost), tangent, bitangent);
            mLightGenerationParamTbl.push_back(param);
        }
        else if (!mIsSpotLightPhotonMapper)
        {
            f32 y = mLightPosY;
            f32 x = mStageOffsetX + cellSize * 0.5f + cellSize * (count / STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE * mLightAreaScale;
            f32 z = mStageOffsetZ + cellSize * 0.5f + cellSize * (count % STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE * mLightAreaScale;
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
            param.setParamAsRectLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost), tangent, bitangent);
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
            param.setParamAsSpotLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost), tangent, bitangent);
            mLightGenerationParamTbl.push_back(param);
        }
        else if (mIsSpotLightPhotonMapper)
        {
            f32 y = mLightPosY;
            f32 x = mStageOffsetX + cellSize * 0.5f + cellSize * (count / STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE * mLightAreaScale;
            f32 z = mStageOffsetZ + cellSize * 0.5f + cellSize * (count % STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE * mLightAreaScale;
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
            param.setParamAsSpotLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost), tangent, bitangent);
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
           
            f32 x = mStageOffsetX + cellSize * 0.5f + cellSize * (count / STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE * mLightAreaScale;
            f32 z = mStageOffsetZ + cellSize * 0.5f + cellSize * (count % STAGE_DIVISION_FOR_LIGHT_POSITION) - PLANE_SIZE * mLightAreaScale;
            LightGenerateParam param;
            //param.setParamAsSphereLight(XMFLOAT3(x, y, z), colorTbl[colorIndex % _countof(colorTbl)], mLightRange * SPHERE_LIGHTS_SIZE_RATIO);
            param.setParamAsSphereLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost), mLightRange* SPHERE_LIGHTS_SIZE_RATIO);
            //param.setParamAsSphereLight(XMFLOAT3(x, y, z), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost * 0.4), mLightRange * SPHERE_LIGHTS_SIZE_RATIO);
            //param.setParamAsSphereLight(XMFLOAT3(mLightPosX, mLightPosY, mLightPosZ), XMFLOAT3(mIntensityBoost, mIntensityBoost, mIntensityBoost), 10, 150);
            mLightGenerationParamTbl.push_back(param);
        }
        count++;
    }
    count = 0;
    if (mIsUseDirectionalLight)
    {
        for (u32 i = 0; i < LightCount_Directional; i++)
        {
            LightGenerateParam param;
            XMFLOAT3 direction;
            XMStoreFloat3(&direction, XMVectorSet(sin(mThetaDirectional * ONE_RADIAN) * cos(mPhiDirectional * ONE_RADIAN), sin(mThetaDirectional * ONE_RADIAN) * sin(mPhiDirectional * ONE_RADIAN), cos(mThetaDirectional * ONE_RADIAN), 0.0f));
            param.setParamAsDirectionalLight(direction, XMFLOAT3(DIRECTIONAL_LIGHT_POWER * mIntensityBoost, DIRECTIONAL_LIGHT_POWER * mIntensityBoost, DIRECTIONAL_LIGHT_POWER * mIntensityBoost));
            mLightGenerationParamTbl.push_back(param);
        }
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

f32 DXRPathTracer::getFrameRate()
{
    return 1000.0f * (mEndTime.QuadPart - mStartTime.QuadPart) / mCpuFreq.QuadPart;
}