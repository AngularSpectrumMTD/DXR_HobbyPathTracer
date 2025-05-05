#ifndef __DXRPHOTONMAPPER_H__
#define __DXRPHOTONMAPPER_H__

#include "AppBase.h"
#include "utility/Utility.h"
#include "utility/OBJ.h"
#include <DirectXMath.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <d3d12.h>
#include <dxgi1_6.h>

#include "d3dx12.h"
#include <pix3.h>
#include "Camera.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

//#define GI_TEST

#define FORMAT_FINAL_RENDER_RESULT DXGI_FORMAT_R8G8B8A8_UNORM

#define STAGE_DIVISION 4
#define STAGE_DIVISION_FOR_LIGHT_POSITION 20

#define BITONIC_BLOCK_SIZE 1024
#define TRANSPOSE_BLOCK_SIZE 16

#define BITONIC2_THREAD_NUM 64

#define GRID_SORT_THREAD_NUM 1024
#define GRID_DIMENSION 800//if increase this param, grid lines are occured in caustics. (Cause : Photon Accumuration)
#define PLANE_SIZE 200

#define PHOTON_NUM_1D 1024
#define DENOISE_ITE 1
#define MAX_RECURSION_DEPTH 10//0---31
#define REAL_MAX_RECURSION_DEPTH 8//0---31
#define PHOTON_RECURSION_DEPTH 8//0---31

#define SPHERE_LIGHTS_SIZE_RATIO 0.7f

#define MAX_SPATIAL_REUSE_TAP 8

#define PHOTON_RANDOM_COUNTER_MAP_SIZE_1D 64
#define PHOTON_EMISSION_GUIDE_MAP_SIZE_1D 64
#define PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL 7

#define SPHERE_RADIUS 5
#define BOX_X_LENGTH 3
#define BOX_Y_LENGTH 3
#define BOX_Z_LENGTH 3

#define OBJ_COUNT 2

#define DI_RESERVOIR_MAX_SPATIAL_REUSE_RADIUS 2
#define GI_RESERVOIR_MAX_SPATIAL_REUSE_RADIUS 2

//#define USE_SSS

namespace HitGroups {
    static const wchar_t* ReflectReflactMaterialSphere = L"hgReflectReflactSpheres";
    static const wchar_t* ReflectReflactMaterialBox = L"hgReflectReflactBoxes";
    static const wchar_t* DefaultMaterialSphere = L"hgMaterialSpheres";
    static const wchar_t* DefaultMaterialBox = L"hgMaterlalBoxes";
    static const wchar_t* Floor = L"hgFloor";
    static const wchar_t* Obj[OBJ_COUNT] = { L"hgObj0", L"hgObj1" };
    static const wchar_t* Light = L"hgLight";
}

namespace RayTracingDxlibs {
    static const wchar_t* RayGen = L"raygen.dxlib";
    static const wchar_t* Miss = L"miss.dxlib";
    static const wchar_t* DefaultMaterialClosestHit = L"closestHitMaterial.dxlib";
    static const wchar_t* DefaultMaterialWithTexClosestHit = L"closestHitMaterialWithTex.dxlib";
    static const wchar_t* LightClosestHit = L"closestHitLight.dxlib";
}

namespace RayTracingEntryPoints {
    static const wchar_t* RayGen = L"rayGen";
    static const wchar_t* Miss = L"miss";
    static const wchar_t* AnyHit = L"anyHit";
    static const wchar_t* AnyHitWithTex = L"anyHitWithTex";
    static const wchar_t* ClosestHitMaterial = L"materialClosestHit";
    static const wchar_t* ClosestHitMaterialWithTex = L"materialWithTexClosestHit";
    static const wchar_t* ClosestHitLight = L"lightClosestHit";    
    static const wchar_t* RayGenPhoton = L"photonEmitting";
    static const wchar_t* MissPhoton = L"photonMiss";
    static const wchar_t* ClosestHitMaterialPhoton = L"materialStorePhotonClosestHit";
    static const wchar_t* ClosestHitMaterialWithTexPhoton = L"materialWithTexStorePhotonClosestHit";
    static const wchar_t* RayGenSpatialReuse = L"spatialReuse";
    static const wchar_t* RayGenTemporalReuse = L"temporalReuse";
}

namespace ComputeShaders {
    const LPCWSTR BitonicSort = L"BitonicSort_BitonicSort.cso";
    const LPCWSTR Transpose = L"BitonicSort_Transpose.cso";

    const LPCWSTR BitonicSort2 = L"BitonicSort.cso";

    const LPCWSTR BuildGrid = L"Grid3D_BuildGrid.cso";
    const LPCWSTR BuildGridIndices = L"Grid3D_BuildGridIndices.cso";
    const LPCWSTR ClearGridIndices = L"Grid3D_ClearGridIndices.cso";
    const LPCWSTR Copy = L"Grid3D_Copy.cso";
    const LPCWSTR RearrangePhoton = L"Grid3D_RearrangePhoton.cso";

    const LPCWSTR Denoise = L"Denoise.cso";

    const LPCWSTR ComputeVariance = L"computeVariance.cso";
    const LPCWSTR A_Trous = L"A-trous.cso";

    const LPCWSTR DebugView = L"debugView.cso";

    const LPCWSTR TemporalAccumulation = L"temporalAccumulation.cso";
    const LPCWSTR FinalizePathtracedResult = L"finalizePathtracedResult.cso";
    //const LPCWSTR TemporalReuse = L"temporalReuse.cso";

    const LPCWSTR ClearPhotonEmissionGuideMap = L"clearPhotonEmissionGuideMap.cso";
    const LPCWSTR GeneratePhotonEmissionGuideMap = L"generatePhotonEmissionGuideMap.cso";
    const LPCWSTR GeneratePhotonEmissionGuideMipMap = L"generatePhotonEmissionGuideMipMap.cso";
    const LPCWSTR GeneratePhotonEmissionGuideMipMap2x2 = L"generatePhotonEmissionGuideMipMap2x2.cso";
    const LPCWSTR GeneratePhotonEmissionGuideMipMap4x4 = L"generatePhotonEmissionGuideMipMap4x4.cso";
    const LPCWSTR AccumulatePhotonEmissionGuideMap = L"accumulatePhotonEmissionGuideMap.cso";
    const LPCWSTR CopyPhotonEmissionGuideMap = L"copyPhotonEmissionGuideMap.cso";
}

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class DXRPathTracer : public AppBase {
public:
    DXRPathTracer(u32 width, u32 height);

    void Initialize() override;
    void Terminate() override;

    void Update() override;
    void Draw() override;

    void OnMouseDown(MouseButton button, s32 x, s32 y) override;
    void OnMouseUp(MouseButton button, s32 x, s32 y) override;
    void OnMouseMove(s32 dx, s32 dy) override;
    void OnKeyDown(UINT8 wparam) override;
    void OnMouseWheel(s32 rotate) override;

private:

    struct float2
    {
        f32 x;
        f32 y;
    };

    struct float3
    {
        f32 x;
        f32 y;
        f32 z;
    };

    struct uint2
    {
        u32 x;
        u32 y;
    };

    struct CompressedMaterialParams
    {
        u32 albedoU32;
        f32 metallic;
        f32 roughness;
        f32 specular;
        f32 transRatio;
        u32 transColorU32;
        u32 emissionU32;
        u32 isSSSExecutable = 0;
    };

    struct Payload
    {
        u32 throughputU32;
        s32 recursive;
        u32 flags;
        f32 T;//for SSS
        u32 hittedCount;//for SSS
        float3 SSSnormal;//for SSS
        u32 primaryBSDFU32;//for ReSTIR GI
        f32 primaryPDF;//for ReSTIR GI
        u32 bsdfRandomSeed;//for ReSTIR GI
        u32 randomSeed;
    };

    struct PhotonPayload
    {
        u32 throughputU32;
        s32 recursive;
        f32 lambdaNM;
        float2 randomUV;
        u32 flags;
        u32 randomSeed;
    };

    struct TriangleIntersectionAttributes
    {
        float2 barys;
    };

    struct DIReservoir
    {
        u32 lightID; //light ID of most important light
        u32 randomSeed;//replay
        f32 targetPDF; //weight of light
        u32 targetPDF_3f_U32; //weight of light(float 3, compressed)
        f32 W_sum; //sum of all weight
        f32 M; //number of ligts processed for this reservoir
    };

    struct GISample
    {
        u32 Lo_2nd_U32;
        float3 pos_2nd;
        float3 nml_2nd;
    };

    struct GIReservoir
    {
        u32 randomSeed;//replay(must be setted before the invocation of sampleBSDF_PDF())
        f32 targetPDF; //weight of light
        u32 targetPDF_3f_U32; //weight of light(float 3)
        f32 W_sum; //sum of all weight
        f32 M; //number of ligts processed for this reservoir

        GISample giSample;
        CompressedMaterialParams compressedMaterial;
    };

    enum SphereTypeCount {
        NormalSpheres = 9
    };

    enum BoxTypeCount {
        NormalBoxes = 6
    };

    enum OBJTypeCount {
        NormalObjs = OBJ_COUNT
    };

    enum StageType {
        StageType_Plane,
        StageType_Box
    };

    enum LightTypeCount {
        NormalLight = 1
    };



    struct SceneParam
    {
        XMMATRIX mtxView;
        XMMATRIX mtxProj;
        XMMATRIX mtxViewInv;
        XMMATRIX mtxProjInv;
        XMMATRIX mtxViewPrev;
        XMMATRIX mtxProjPrev;
        XMMATRIX mtxViewInvPrev;
        XMMATRIX mtxProjInvPrev;
        XMUINT4 flags;
        XMFLOAT4 photonParams;//x unused
        XMVECTOR cameraParams;//near far reserved reserved
        XMVECTOR gatherParams;
        XMVECTOR gatherParams2;
        XMVECTOR spotLightParams;
        XMVECTOR viewVec;
        XMUINT4 additional;
        XMUINT4 additional1;
        XMUINT4 additional2;
        XMVECTOR sssParam;
        XMVECTOR toneMappingParam;
    };

    struct PhotonInfo
    {
        u32 throughput;
        XMFLOAT3 position;
        //XMFLOAT3 inDir;
    };

    struct BitonicSortCB
    {
        u32 mLevel;
        u32 mLevelMask;
        u32 mWidth;
        u32 mHeight;

        void set(u32 level, u32 levelMask, u32 width, u32 height)
        {
            mLevel = level;
            mLevelMask = levelMask;
            mWidth = width;
            mHeight = height;
        }
    };

    struct BitonicSortCB2
    {
        s32 inc;
        s32 dir;
    };

    struct GridCB
    {
        s32 numPhotons;
        XMFLOAT3 gridDimensions;
        f32 gridH;
        f32 photonExistRange;
    };

    struct DenoiseCB
    {
        u32 stepScale;
    };

    enum LightType
    {
        LightType_Sphere = 0,
        LightType_Rect = 1,
        LightType_Spot = 2,
        LightType_Directional = 3
    };

    struct ReSTIRParam
    {
        XMUINT4 data;
    };

    struct LightGenerateParam
    {
        XMFLOAT3 positionORDirection = XMFLOAT3(0, 0, 0);
        XMFLOAT3 emission = XMFLOAT3(0, 0, 0);
        XMFLOAT3 U = XMFLOAT3(0, 0, 0); //u vector for rectangle or spot light
        XMFLOAT3 V = XMFLOAT3(0, 0, 0); //v vector for rectangle or spot light
        f32 sphereRadius = 0; //radius for sphere light
        u32 type = 0; //Sphere Light 0 / Rect Light 1 / Spot Light 2 / Directional Light 3

        void setParamAsSphereLight(const XMFLOAT3 pos, const XMFLOAT3 emi, const f32 radius)
        {
            type = LightType_Sphere;
            positionORDirection = pos;
            emission = emi;
            sphereRadius = radius;
        }

        void setParamAsRectLight(const XMFLOAT3 pos, const XMFLOAT3 emi, const XMFLOAT3 u, const XMFLOAT3 v)
        {
            type = LightType_Rect;
            positionORDirection = pos;
            emission = emi;
            U = u;
            V = v;
        }

        void setParamAsSpotLight(const XMFLOAT3 pos, const XMFLOAT3 emi, const XMFLOAT3 u, const XMFLOAT3 v)
        {
            type = LightType_Spot;
            positionORDirection = pos;
            emission = emi;
            U = u;
            V = v;
        }

        void setParamAsDirectionalLight(const XMFLOAT3 dir, const XMFLOAT3 emi)
        {
            type = LightType_Directional;
            positionORDirection = dir;
            emission = emi;
        }
    };

    enum LightCount
    {
        LightCount_Sphere = 800,
        LightCount_Rect = 1,
        LightCount_Spot = 1,
        LightCount_Directional = 1,
        LightCount_ALL = LightCount_Sphere + LightCount_Rect + LightCount_Spot + LightCount_Directional
    };

    enum ModelType
    {
        ModelType_Crab,
        ModelType_TwistCube,
        ModelType_SimpleCube,
        ModelType_Teapot,
        ModelType_LikeWater,
        ModelType_Ocean,
        ModelType_Ocean2,
        ModelType_Diamond,
        ModelType_Skull,
        ModelType_HorseStatue,
        ModelType_Dragon,
        ModelType_Afrodyta,
        ModelType_Rock,
        ModelType_CurvedMesh,
        ModelType_DebugMesh,
        ModelType_Buddha
    };

    enum SceneType
    {
        SceneType_Simple,
        SceneType_Sponza,
        SceneType_BistroExterior,
        SceneType_BistroInterior,
        SceneType_SanMiguel,
        //SceneType_Room,
        //SceneType_GITest,
        //SceneType_Kitchen,
        SceneType_PTTest,
        SceneType_PTTestBrick,
        SceneType_PTTestRobot,
        SceneType_PTTestRoom,
        SceneType_PTTestRoom2,
        SceneType_MaterialTest,
        SceneType_Corridor,
        SceneType_CausticsTest,
    };

    enum Spectrum
    {
        Spectrum_D65,
        Spectrum_Stabdard,
        Spectrum_Fluorescent,
        Spectrum_IncandescentBulb,
        Spectrum_Count,
    };

    enum CausticsQuality
    {
        CausticsQuality_LOW = 256,
        CausticsQuality_MIDDLE = 512,
        CausticsQuality_HIGH = 1024
    };

    void InitializeCamera();

    void Setup();
    void CreateSceneInfo();
    void CreateSceneBLAS();
    void CreateSceneTLAS();
    void CreateStateObject(ComPtr<ID3D12StateObject>& stateObject, ComPtr<ID3D12RootSignature>& globalRootSignature, const u32 maxPayloadSize, const u32 maxAttributeSize, const u32 maxRecursionDepth, const wchar_t* rayGenLibraryName, const wchar_t* rayGenShaderName, const wchar_t* missLibraryName, const wchar_t* missShaderName, const wchar_t* chLibraryName, const wchar_t* chShaderName, const wchar_t* chLibraryNameWithTex, const wchar_t* chShaderNameWithTex, const wchar_t* ahShaderName = nullptr, const wchar_t* ahShaderNameWithTex = nullptr);
    void CreateStateObjects();
    void CreateLightGenerationBuffer();
    void CreateRootSignatureGlobal();
    void CreateRootSignatureLocal();
    void CreateShaderTable(ComPtr<ID3D12Resource>& shaderTable, ComPtr<ID3D12StateObject>& stateObject, D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc, const u32 maxRootSigSizeRayGen, const u32 maxRootSigSizeMiss, const u32 maxRootSigSizeHitGroup, const wchar_t* shaderTableName, const wchar_t* rayGenShaderName, const wchar_t* missShaderName);
    void CreateShaderTables();
    void CreateComputeRootSignatureAndPSO();
    void CreateComputeShaderStateObject(const LPCWSTR& compiledComputeShaderName, ComPtr<ID3D12PipelineState>& computePipelineState, ComPtr<ID3D12RootSignature> rootSig);
    //Å´ combine
    void CreateAccelerationStructure();
    void CreateRaytracingRootSignatureAndPSO();
    void CreateRegularBuffer();
    void CreateConstantBuffer();

    void UpdateSceneTLAS();
    void UpdateSceneParams();
    void UpdateMaterialParams();
    void UpdateLightGenerateParams();
    void InitializeLightGenerateParams();
    void UpdateWindowText();

    void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, u32 pathSize);
    std::wstring GetAssetFullPath(LPCWSTR assetName);
    f32 Clamp(f32 min, f32 max, f32 src);
    f32 getFrameRate();
    //utility::TextureResource LoadTextureFromFile(const std::wstring& fileName);
    
    void UpdateMeshInstances(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs);

    void Grid3DSort();
    void BitonicSortLDS();
    void BitonicSortSimple();

    void SpatiotemporalVarianceGuidedFiltering();

    ComPtr<ID3D12GraphicsCommandList4> mCommandList;
    static const u32 BackBufferCount = dx12::RenderDeviceDX12::BackBufferCount;
    
    //Meshes
    utility::PolygonMesh mMeshStage;
    utility::PolygonMesh mMeshSphere;
    utility::PolygonMesh mMeshBox;
    std::array < utility::PolygonMesh, NormalObjs> mMeshOBJTbl;
    //utility::PolygonMesh mMeshLightSphere;
    std::array <std::wstring, NormalObjs> mOBJFileNameTbl;
    std::wstring mStageTextureFileName;
    std::wstring mCubeMapTextureFileName;

    std::string mOBJFileName;
    std::string mOBJFolderName;

    utility::OBJMaterialLinkedMesh mOBJMaterialLinkedMesh;

    //ObjectAttributes of TLAS
    std::array<XMMATRIX, NormalSpheres> mSpheresNormalTbl;
    std::array<XMMATRIX, NormalBoxes> mBoxesNormalTbl;
    std::array<XMMATRIX, NormalObjs> mOBJNormalTbl;
    std::array<XMMATRIX, NormalLight> mLightTbl;

    XMMATRIX mOBJMaterialLinkedMeshTRS;

    //Materials
    std::array<utility::MaterialParam, NormalSpheres> mNormalSphereMaterialTbl;
    std::array<utility::MaterialParam, NormalBoxes> mNormalBoxMaterialTbl;
    std::array<utility::MaterialParam, NormalObjs> mOBJMaterialTbl;
    utility::MaterialParam mStageMaterial;
    ComPtr<ID3D12Resource> mNormalSphereMaterialCB;
    ComPtr<ID3D12Resource> mNormalBoxMaterialCB;
    ComPtr<ID3D12Resource> mReflectSphereMaterialCB;
    ComPtr<ID3D12Resource> mRefractSphereMaterialCB;
    ComPtr<ID3D12Resource> mReflectBoxMaterialCB;
    ComPtr<ID3D12Resource> mRefractBoxMaterialCB;
    ComPtr<ID3D12Resource> mOBJMaterialCB;
    ComPtr<ID3D12Resource> mStageMaterialCB;

    //Scale, Offset
    std::array<f32, NormalObjs> mObjYOffsetTbl;
    std::array<XMFLOAT3, NormalObjs> mObjScaleTbl;

    //Material
    std::array<utility::MaterialParam, NormalObjs> mMaterialParamTbl;

    //Lights
    std::vector<LightGenerateParam> mLightGenerationParamTbl;
    ComPtr <ID3D12Resource> mLightGenerationParamBuffer;
    dx12::Descriptor mLightGenerationParamSRV;

    SceneParam mSceneParam;
    utility::TextureResource mGroundTex;
    utility::TextureResource mCubeMapTex;
    utility::TextureResource mDummyAlphaMask;

    Camera mCamera;

    //Acceleration Structures
    ComPtr<ID3D12Resource> mTLAS;
    ComPtr<ID3D12Resource> mTLASUpdate;
    dx12::Descriptor mTLASDescriptor;
    ComPtr<ID3D12Resource> mInstanceDescsBuffer;

    //Buffers
    ComPtr <ID3D12Resource> mFinalRenderResult;
    dx12::Descriptor mFinalRenderResultDescriptorUAV;
    dx12::Descriptor mFinalRenderResultDescriptorSRV;
    ComPtr <ID3D12Resource> mPathtracedRenderResult;
    dx12::Descriptor mPathtracedRenderDescriptorUAV;
    dx12::Descriptor mPathtracedRenderDescriptorSRV;
    ComPtr<ID3D12Resource> mPhotonMap;
    dx12::Descriptor mPhotonMapDescriptorSRV;
    dx12::Descriptor mPhotonMapDescriptorUAV;
    ComPtr<ID3D12Resource> mPhotonMapSorted;
    dx12::Descriptor mPhotonMapSortedDescriptorSRV;
    dx12::Descriptor mPhotonMapSortedDescriptorUAV;
    ComPtr<ID3D12Resource> mPhotonGrid;
    dx12::Descriptor mPhotonGridDescriptorSRV;
    dx12::Descriptor mPhotonGridDescriptorUAV;
    ComPtr<ID3D12Resource> mPhotonGridTmp;
    dx12::Descriptor mPhotonGridTmpDescriptorSRV;
    dx12::Descriptor mPhotonGridTmpDescriptorUAV;
    ComPtr<ID3D12Resource> mPhotonGridId;
    dx12::Descriptor mPhotonGridIdDescriptorSRV;
    dx12::Descriptor mPhotonGridIdDescriptorUAV;
    ComPtr <ID3D12Resource> mRandomNumberBuffer;
    dx12::Descriptor mRandomNumberBufferDescriptorUAV;
    dx12::Descriptor mRandomNumberBufferDescriptorSRV;

    std::vector < ComPtr<ID3D12Resource>> mNormalDepthBufferTbl;
    std::vector < dx12::Descriptor> mNormalDepthBufferDescriptorSRVTbl;
    std::vector < dx12::Descriptor> mNormalDepthBufferDescriptorUAVTbl;
    std::vector < ComPtr<ID3D12Resource>> mPositionBufferTbl;
    std::vector < dx12::Descriptor> mPositionBufferDescriptorSRVTbl;
    std::vector < dx12::Descriptor> mPositionBufferDescriptorUAVTbl;
    std::vector < ComPtr<ID3D12Resource>> mAccumulationCountBufferTbl;
    std::vector < dx12::Descriptor> mAccumulationCountBufferDescriptorSRVTbl;
    std::vector < dx12::Descriptor> mAccumulationCountBufferDescriptorUAVTbl;
    ComPtr<ID3D12Resource> mDenoisedColorBuffer;
    dx12::Descriptor mDenoisedColorBufferDescriptorSRV;
    dx12::Descriptor mDenoisedColorBufferDescriptorUAV;

    ComPtr<ID3D12Resource> mPrevIDBuffer;
    dx12::Descriptor mPrevIDBufferDescriptorSRV;
    dx12::Descriptor mPrevIDBufferDescriptorUAV;

    std::vector < ComPtr<ID3D12Resource>> mDIBufferPingPongTbl;
    std::vector < dx12::Descriptor> mDIBufferDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mDIBufferDescriptorUAVPingPongTbl;

    std::vector < ComPtr<ID3D12Resource>> mGIBufferPingPongTbl;
    std::vector < dx12::Descriptor> mGIBufferDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mGIBufferDescriptorUAVPingPongTbl;

    std::vector < ComPtr<ID3D12Resource>> mCausticsBufferPingPongTbl;
    std::vector < dx12::Descriptor> mCausticsBufferDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mCausticsBufferDescriptorUAVPingPongTbl;

    std::vector < ComPtr<ID3D12Resource>> mLuminanceMomentBufferTbl;
    std::vector < dx12::Descriptor> mLuminanceMomentBufferDescriptorSRVTbl;
    std::vector < dx12::Descriptor> mLuminanceMomentBufferDescriptorUAVTbl;

    std::vector < ComPtr<ID3D12Resource>> mLuminanceVarianceBufferTbl;
    std::vector < dx12::Descriptor> mLuminanceVarianceBufferDescriptorSRVTbl;
    std::vector < dx12::Descriptor> mLuminanceVarianceBufferDescriptorUAVTbl;

    ComPtr<ID3D12Resource> mDebugTexture;
    dx12::Descriptor mDebugTextureDescriptorSRV;
    dx12::Descriptor mDebugTextureDescriptorUAV;

    ComPtr<ID3D12Resource> mDebugTexture0;
    dx12::Descriptor mDebugTexture0DescriptorSRV;
    dx12::Descriptor mDebugTexture0DescriptorUAV;

    ComPtr<ID3D12Resource> mDebugTexture1;
    dx12::Descriptor mDebugTexture1DescriptorSRV;
    dx12::Descriptor mDebugTexture1DescriptorUAV;

    std::vector < ComPtr<ID3D12Resource>> mDIReservoirPingPongTbl;
    std::vector < dx12::Descriptor> mDIReservoirDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mDIReservoirDescriptorUAVPingPongTbl;

    std::vector < ComPtr<ID3D12Resource>> mDISpatialReservoirPingPongTbl;
    std::vector < dx12::Descriptor> mDISpatialReservoirDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mDISpatialReservoirDescriptorUAVPingPongTbl;

    std::vector < ComPtr<ID3D12Resource>> mGIReservoirPingPongTbl;
    std::vector < dx12::Descriptor> mGIReservoirDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mGIReservoirDescriptorUAVPingPongTbl;

    std::vector < ComPtr<ID3D12Resource>> mGISpatialReservoirPingPongTbl;
    std::vector < dx12::Descriptor> mGISpatialReservoirDescriptorSRVPingPongTbl;
    std::vector < dx12::Descriptor> mGISpatialReservoirDescriptorUAVPingPongTbl;

    //photon guiding (PHOTON_RANDOM_COUNTER_MAP_SIZE_1D x PHOTON_RANDOM_COUNTER_MAP_SIZE_1D)
    ComPtr<ID3D12Resource> mPhotonRandomCounterMap;
    dx12::Descriptor mPhotonRandomCounterMapDescriptorSRV;
    dx12::Descriptor mPhotonRandomCounterMapDescriptorUAV;

    ComPtr<ID3D12Resource> mPhotonEmissionGuideMipMapTbl[PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL];
    dx12::Descriptor mPhotonEmissionGuideMipMapDescriptorSRVTbl[PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL];
    dx12::Descriptor mPhotonEmissionGuideMipMapDescriptorUAVTbl[PHOTON_EMISSION_GUIDE_MAP_MIP_LEVEL];

    ComPtr<ID3D12Resource> mPhotonEmissionGuideMipMap0Prev;
    dx12::Descriptor mPhotonEmissionGuideMipMap0PrevDescriptorSRV;
    dx12::Descriptor mPhotonEmissionGuideMipMap0PrevDescriptorUAV;

    //ConstantBuffers
    std::vector<ComPtr<ID3D12Resource>> mBitonicLDSCB0Tbl;
    std::vector<ComPtr<ID3D12Resource>> mBitonicLDSCB1Tbl;
    std::vector<ComPtr<ID3D12Resource>> mBitonicLDSCB2Tbl;
    std::vector<ComPtr<ID3D12Resource>> mBitonicSimpleCBTbl;
    ComPtr<ID3D12Resource> mGridSortCB;
    std::vector<ComPtr<ID3D12Resource>> mDenoiseCBTbl;
    ComPtr<ID3D12Resource> mSceneCB;
    std::vector<ComPtr<ID3D12Resource>> mReSTIRParamCBTbl;

    //Pipeline State
    //--Ordinal Raytrace
    ComPtr<ID3D12StateObject> mRTPSO;
    ComPtr<ID3D12Resource> mShaderTable;
    D3D12_DISPATCH_RAYS_DESC mDispatchRayDesc;
    ComPtr<ID3D12RootSignature> mGlobalRootSig;
    std::unordered_map < std::string, u32> mRegisterMapGlobalRootSig;

    //--PhotonMapping
    ComPtr<ID3D12StateObject> mRTPSOPhoton;
    ComPtr<ID3D12Resource> mShaderTablePhoton;
    D3D12_DISPATCH_RAYS_DESC mDispatchPhotonRayDesc;
    ComPtr<ID3D12RootSignature> mGlobalRootSigPhoton;
    std::unordered_map < std::string, u32> mRegisterMapGlobalRootSigPhoton;

    //--Reservoir Spatial Reuse
    ComPtr<ID3D12StateObject> mRTPSOReservoirSpatialReuse;
    ComPtr<ID3D12Resource> mShaderTableReservoirSpatialReuse;
    D3D12_DISPATCH_RAYS_DESC mDispatchReservoirSpatialReuseRayDesc;
    ComPtr<ID3D12RootSignature> mGlobalRootSigReservoirSpatialReuse;
    std::unordered_map < std::string, u32> mRegisterMapGlobalRootSigReservoirSpatialReuse;

    //--Reservoir Temporal Reuse
    ComPtr<ID3D12StateObject> mRTPSOReservoirTemporalReuse;
    ComPtr<ID3D12Resource> mShaderTableReservoirTemporalReuse;
    D3D12_DISPATCH_RAYS_DESC mDispatchReservoirTemporalReuseRayDesc;
    ComPtr<ID3D12RootSignature> mGlobalRootSigReservoirTemporalReuse;
    std::unordered_map < std::string, u32> mRegisterMapGlobalRootSigReservoirTemporalReuse;

    //material binding
    ComPtr<ID3D12RootSignature> mLocalRootSigMaterial;
    std::unordered_map < std::string, u32> mRegisterMapGlobalLocalRootSigMaterial;
    ComPtr<ID3D12RootSignature> mLocalRootSigMaterialWithTex;
    std::unordered_map < std::string, u32> mRegisterMapGlobalLocalRootSigMaterialWithTex;

    ComPtr<ID3D12RootSignature> mRsBitonicSortLDS;
    std::unordered_map < std::string, u32> mRegisterMapBitonicSortLDS;
    ComPtr<ID3D12RootSignature> mRsTranspose;
    std::unordered_map < std::string, u32> mRegisterMapTranspose;
    ComPtr<ID3D12PipelineState> mBitonicSortLDSPSO;
    ComPtr<ID3D12PipelineState> mTransposePSO;

    ComPtr<ID3D12RootSignature> mRsBitonicSortSimple;
    std::unordered_map < std::string, u32> mRegisterMapBitonicSortSimple;
    ComPtr<ID3D12PipelineState> mBitonicSortSimplePSO;

    ComPtr<ID3D12RootSignature> mRsBuildGrid;
    std::unordered_map < std::string, u32> mRegisterMapBuildGrid;
    ComPtr<ID3D12RootSignature> mRsBuildGridIndices;
    std::unordered_map < std::string, u32> mRegisterMapBuildGridIndices;
    ComPtr<ID3D12RootSignature> mRsCopy;
    std::unordered_map < std::string, u32> mRegisterMapCopy;
    ComPtr<ID3D12RootSignature> mRsClearGridIndices;
    std::unordered_map < std::string, u32> mRegisterMapClearGridIndices;
    ComPtr<ID3D12RootSignature> mRsRearrangePhoton;
    std::unordered_map < std::string, u32> mRegisterMapRearrangePhoton;
    ComPtr<ID3D12PipelineState> mBuildGridPSO;
    ComPtr<ID3D12PipelineState> mBuildGridIndicesPSO;
    ComPtr<ID3D12PipelineState> mCopyPSO;
    ComPtr<ID3D12PipelineState> mClearGridIndicesPSO;
    ComPtr<ID3D12PipelineState> mRearrangePhotonPSO;

    ComPtr<ID3D12RootSignature> mRsComputeVariance;
    std::unordered_map < std::string, u32> mRegisterMapComputeVariance;
    ComPtr<ID3D12PipelineState> mComputeVariancePSO;

    ComPtr<ID3D12RootSignature> mRsA_TrousWaveletFilter;
    std::unordered_map < std::string, u32> mRegisterMapA_TrousWaveletFilter;
    ComPtr<ID3D12PipelineState> mA_TrousWaveletFilterPSO;

    ComPtr<ID3D12RootSignature> mRsDebugView;
    std::unordered_map < std::string, u32> mRegisterMapDebugView;
    ComPtr<ID3D12PipelineState> mDebugViewPSO;

    ComPtr<ID3D12RootSignature> mRsTemporalAccumulation;
    std::unordered_map < std::string, u32> mRegisterMapTemporalAccumulation;
    ComPtr<ID3D12PipelineState> mTemporalAccumulationPSO;

    ComPtr<ID3D12RootSignature> mRsFinalizePathtracedResult;
    std::unordered_map < std::string, u32> mRegisterMapFinalizePathtracedResult;
    ComPtr<ID3D12PipelineState> mFinalizePathtracedResultPSO;

    //Emission Guiding
    ComPtr<ID3D12RootSignature> mRsClearPhotonEmissionGuideMap;
    std::unordered_map < std::string, u32> mRegisterMapClearPhotonEmissionGuideMap;
    ComPtr<ID3D12PipelineState> mClearPhotonEmissionGuideMapPSO;

    ComPtr<ID3D12RootSignature> mRsAccumulatePhotonEmissionGuideMap;
    std::unordered_map < std::string, u32> mRegisterMapAccumulatePhotonEmissionGuideMap;
    ComPtr<ID3D12PipelineState> mAccumulatePhotonEmissionGuideMapPSO;

    ComPtr<ID3D12RootSignature> mRsCopyPhotonEmissionGuideMap;
    std::unordered_map < std::string, u32> mRegisterMapCopyPhotonEmissionGuideMap;
    ComPtr<ID3D12PipelineState> mCopyPhotonEmissionGuideMapPSO;

    ComPtr<ID3D12RootSignature> mRsGeneratePhotonEmissionGuideMap;
    std::unordered_map < std::string, u32> mRegisterMapGeneratePhotonEmissionGuideMap;
    ComPtr<ID3D12PipelineState> mGeneratePhotonEmissionGuideMapPSO;

    ComPtr<ID3D12RootSignature> mRsGeneratePhotonEmissionGuideMipMap;
    std::unordered_map < std::string, u32> mRegisterMapGeneratePhotonEmissionGuideMipMap;
    ComPtr<ID3D12PipelineState> mGeneratePhotonEmissionGuideMipMapPSO;

    ComPtr<ID3D12RootSignature> mRsGeneratePhotonEmissionGuideMipMap2x2;
    std::unordered_map < std::string, u32> mRegisterMapGeneratePhotonEmissionGuideMipMap2x2;
    ComPtr<ID3D12PipelineState> mGeneratePhotonEmissionGuideMipMap2x2PSO;

    ComPtr<ID3D12RootSignature> mRsGeneratePhotonEmissionGuideMipMap4x4;
    std::unordered_map < std::string, u32> mRegisterMapGeneratePhotonEmissionGuideMipMap4x4;
    ComPtr<ID3D12PipelineState> mGeneratePhotonEmissionGuideMipMap4x4PSO;

    //Screen Space Material
    ComPtr<ID3D12Resource> mScreenSpaceMaterialBuffer;
    dx12::Descriptor mScreenSpaceMaterialBufferDescriptorSRV;
    dx12::Descriptor mScreenSpaceMaterialBufferDescriptorUAV;

    u32 mRenderFrame = 0;
    u32 mSeedFrame = 0;

    std::wstring mAssetPath;

    f32 mLightRange;
    f32 mIntensityBoost;
    f32 mGatherRadius;
    u32 mGatherBlockRange;
    f32 mStandardPhotonNum;
    bool mIsDebug;

    f32 mLightPosX;
    f32 mLightPosY;
    f32 mLightPosZ;

    f32 mPhi;
    f32 mTheta;
    f32 mPhiDirectional;
    f32 mThetaDirectional;

    u32 mLightCount;
    bool mIsSpotLightPhotonMapper;

    f32 mCausticsBoost;

    f32 mModelMovingRange;

    std::array<ModelType, NormalObjs> mModelTypeTbl;

    bool mVisualizeLightRange;

    bool mInverseMove;
    bool mIsApplyCaustics;
    bool mIsUseDenoise;
    s32 mSpectrumMode;
    bool mIsMoveModel;
    bool mIsUseTexture;
    bool mIsIndirectOnly;
    bool mIsUseDebugView;
    bool mIsUseIBL;
    bool mIsUseEmissivePolygon;
    bool mIsUseMedianFiltering;

    LARGE_INTEGER mCpuFreq;
    LARGE_INTEGER mStartTime;
    LARGE_INTEGER mEndTime;

    u32 mLightLambdaNum;

    u32 mPhotonMapSize1D;

    StageType mStageType;
    SceneType mSceneType;

    u32 mTargetModelIndex;

    bool mIsUseTemporalAccumulation;
    bool mIsUseNEE;
    //bool mIsUseDirectionalLight;

    XMFLOAT3 mInitEyePos;
    XMFLOAT3 mInitTargetPos;

    u32 mRecursionDepth = 0;
    u32 mSceneLightNum = 0;

    f32 mStageOffsetY = 0;
    f32 mStageOffsetX = 0;
    f32 mStageOffsetZ = 0;

    bool mIsUseManySphereLightLighting;
    bool isPrimalLightSRVUpdate = true;
    bool mIsUseStreamingRIS = false;
    bool mIsTemporalAccumulationForceDisable = false;

    bool mIsUseReservoirTemporalReuse = false;
    bool mIsUseReservoirSpatialReuse = false;

    u32 mSpatialReuseTap = 1;

    bool mIsUseMetallicTest = false;

    bool mIsHistoryResetRequested = false;

    bool mIsAlbedoOne = false;

    f32 mMeanFreePath = 10e-3f;//mMeanFreePathRatio * mMeanFreePath must be 1[mm] - 10[mm]
    f32 mMeanFreePathRatio = 0.5;

    f32 mCameraSpeed = 1.0f;

    f32 mLightAreaScale = 1.0f;

    bool mIsUseDirectionalLight = true;

    u32 mMoveFrame = 0;
    f32 mExposure = 1.0f;
};

#endif
