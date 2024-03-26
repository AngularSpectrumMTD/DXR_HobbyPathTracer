# DXRHybridPhotonMapper
Hybrid Photon Mapper By Use Of DirectX Raytracing(Simple Pathtracing + Photon Mapping[Caustics])

![exterior](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/c0979367-2817-4341-93ed-c633e2308e4b)

![interior](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/a8d9dd0a-7678-48be-8476-77b1087aee7d)

![sponza](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/3fde63b0-98fe-49a1-ad79-9d205aa1b7a0)

### Caustics
Caustics are rendered by spectral rendering

![spectral2](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/2888a709-2811-47b0-8348-4d7161b7e684)

### Light Type
Photon emitters emit photons from a spotlight or rectangular light

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### Debug View
Enable to check (Diffuse)Albedo / Depth / Normal

![debugView](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/2ebd7f50-8a61-4353-a636-3f755f663a93)

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
- V : enable / disable debug view
- MOUSE_RIGHT : rotate the camera around the gazing point
- MOUSE_LEFT : move the camera back and forth
- ↑↓→← : move the camera up, down, left, or right