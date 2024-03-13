# DXRHybridPhotonMapper
Hybrid Photon Mapper By Use Of DirectX Raytracing(Simple Pathtracing + Photon Mapping[Caustics])

![interior](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/bba581ae-955a-4978-ab4c-f68623357d63)

![exterior](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/fdbfda03-80f7-4ea1-a3b0-b1b2feb9b9d6)

![sponza](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/a31c6d38-5254-44ce-81db-3a771022f0f6)

### Caustics
Caustics are rendered by spectral rendering

![spectral](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/7008dbae-1b27-46e3-b1bc-cc206841311a)

### Light Type
Photon emitters emit photons from a spotlight or rectangular light

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### How To Use
**NOTE : This program can only handle triangular polygons**

#### Simple Scene
1. Set "SceneType_Simple" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]

2. Build and Execute

#### With Sponza
1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

2. Open sponza.obj on blender. Set the size to 0.1 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "sponza.obj")

4. Create a new folder "sponza" in "DXRPhotonMapper/PhotonMapper/model" and copy the "textures" and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.

5. Set "SceneType_Sponza" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]

6. Build and Execute

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0b4f954b-4875-4a5f-816b-26174ce90bea" width="300">

#### With Bistro
1. Download model(bistro) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

2. Open exterior.obj / interior.obj on blender. Set the size to 0.1 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "exterior.obj"/"interior.obj")

4. Create a new folder "bistro" in "DXRPhotonMapper/PhotonMapper/model". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj / interior.obj file in the "Exterior" / "Interior".

5. Set "SceneType_BistroExterior" / "SceneType_BistroInterior" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]

6. Build and Execute

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/274017c1-d198-4dc1-81b2-2fc23efd323f" width="300">

### Graphics API
DirectX12

### Action

- I : determine other button meaning (+ / -)
- J : start / stop model
- G : increase / decrease photon gather radius
- X / Y / Z : light position
- L : increase / decrease light emission range
- T / P : light emission angle(theta / phi)
- K : increase / decrease light intensity
- B : increase / decrease photon gathering block num
- N : visualize caustics
- D : enable / disable SVGF(A-Trous wavelet filtering)
- Q : increase / decrease caustics boost
- U : enable / disable texture
- R : increase / decrease roughness
- S : increase / decrease translucency
- M : increase / decrease metallic
- C : change photon emitter type (spot light/rectangular light)
- MOUSE_RIGHT : rotate the camera around the gazing point
- MOUSE_LEFT : move the camera back and forth
- ↑↓→← : move the camera up, down, left, or right