# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

### Bistro Interior

![bistroInterior0](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/09e3261f-7860-4c18-9f34-5d00754f5009)

![bistroInterior1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/e3d7059e-0fbe-40fe-87b2-7697fa78e7c8)

![bistroInterior2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/7988214f-5823-4d28-a5a2-731bc5b434af)

![bistroInterior3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/6f46ea26-61de-4528-9d47-075ead005b79)

### Bistro Exterior

![bistroExterior0](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/021661ba-2057-4000-86d6-07a61353bf32)

![bistroExterior1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/b110b7b1-246d-4dee-a525-58f3d2fa9cc4)

![bistroExterior2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/8b79d35e-c353-4366-b1d3-6debf363fbf8)

### Sponza

![sponza](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/f1ae0be4-af18-467a-aeea-fa8803c2b428)

![sponzaM](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/362cd500-f714-4d47-a11f-c3f95a0982ab)

![sponzaR](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/d2b3f0c7-2614-46b0-8c2b-7cf72513325f)

### Simple Scene

![simple](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/521ce159-4530-4986-9a17-8effbd8c0c9e)

### Caustics
Caustics are rendered by spectral rendering

![chromatic](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/ff5aaeff-70dd-4401-9aa4-c3f1adf5c3fa)

### Light Type
Photon emitters emit photons from a spotlight or rectangular light

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### How To Use
**NOTE : This program can only handle triangular polygons**

#### Normal
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