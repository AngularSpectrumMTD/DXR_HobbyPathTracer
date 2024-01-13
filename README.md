# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/d1f0f61c-e12a-45e1-bbdb-dfa516ad1b17)

![2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/b2f6f984-a491-46ae-a0be-2334eaa741a0)

![3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/e6b3bb1a-51ae-4b91-a6ac-c09ee8b8f8bf)

![4](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/ac4bbe5f-ea75-47d7-8b36-6bcb17f734b8)

![5](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/c059e765-97ca-4b89-8c7f-7c1fc86e4aff)

![sponza](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/9ae8b4ff-60cb-485b-b9a4-1306fbe2f2a8)

![simple](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/6e195eb2-03a6-4b9d-9d91-57908cabd5c6)

### Caustics
Caustics are rendered by spectral rendering

![ChromaticAberration](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/e4db77f7-c360-4198-b367-0db220a56bda)

### Light Type
Photon emitters emit photons from a spotlight or rectangular light

| Spot Light | Rectangular Light |
| ---- | ---- |
| <img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/120a1ec4-81ff-40bf-9ba9-95aa95039ba3" width="500"> | <img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/8d27e7e7-3109-4d5a-a240-8c1c954d74d0" width="500"> |

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

2. Open sponza.obj on blender. Set the size to 0.05 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "exterior.obj")

4. Create a new folder "bistro" in "DXRPhotonMapper/PhotonMapper/model". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj file in the "Exterior".

5. Set "SceneType_Bistro" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]

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