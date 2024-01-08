# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/937b31f4-41f0-4d50-aebc-a66e57b5984a)

![2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/3c9e0fe5-8890-498c-9cc4-7fe949091e37)

![3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/7d5fcea6-9f4f-4921-968c-43df17c09627)

![4](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/29ce2c23-dbee-4800-b523-2161f22f3348)

![5](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/feb34846-984e-4c6b-96c9-9b42ac95936b)


### Caustics
Caustics are rendered by spectral rendering

![ChromaticAberration](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/f31c398d-45a2-49a5-95da-fc78ecf6af16)

### Light Type
Photon emitters emit photons from a spotlight or rectangular light

| Spot Light | Rectangular Light |
| ---- | ---- |
| <img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/4540c81d-fbf7-402f-b237-68cd8bc8c2d7" width="500"> | <img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/8974e050-9dd5-4ea7-bd76-b7c0e6585c06" width="500"> |

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### How To Use
**NOTE : This program can only handle triangular polygons**

#### Normal
1. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/e69367eb-6044-46dd-ae79-4cb7c294d18f" width="50%">

2. Build and Execute

![normal](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/61e83c9a-c0f0-41c1-810b-0c3747417374)

#### With Sponza
1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Exp1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0b4f954b-4875-4a5f-816b-26174ce90bea)

2. Open sponza.obj on blender. Set the size to 0.1 -> Execute Triangulation.

4. Export 3D model as obj file(Name the file "sponza.obj")

5. Create a new folder (ex. "sponza") in "DXRPhotonMapper/PhotonMapper/model" and copy the "textures" and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.

6. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/3e48b1be-90f6-4663-86c0-228fb3d42e87" width="50%">

7. Build and Execute

![sponza](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/e61fbf1c-a0ca-4e9a-88b6-ca6ba308943f)

#### With Bistro
1. Download model(bistro) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Bis](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/274017c1-d198-4dc1-81b2-2fc23efd323f)

2. Open sponza.obj on blender. Set the size to 0.05 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "exterior.obj")

4. Create a new folder (e.g. "bistro") in "DXRPhotonMapper/PhotonMapper/model". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj file in the "Exterior".

5. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/2e1afbf9-3e0c-41f0-8cc0-9bc62251a8cf" width="50%">

6. Build and Execute

![bistro](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/ee5b29c4-5281-4991-b599-85346c5d22b9)

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