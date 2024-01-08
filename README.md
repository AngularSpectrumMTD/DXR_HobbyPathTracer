# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/7532867e-994c-48fc-aa14-a705eb45974e)

![2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/2c982f4c-d22a-46be-bd72-4cf8f0405388)

![3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/283cabd3-5e6a-44e0-a9d0-160b973c7804)

![4](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/17e4c509-8e60-4f4f-8a87-de66af9ee610)

![5](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/69bc2c71-85a4-45b2-ab40-34bbda95802d)

![sponza](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/c1434100-2b3f-4395-a09a-973456577290)

![normal](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/f8ea1100-034a-45c5-9c83-b7b8bada4614)

### Caustics
Caustics are rendered by spectral rendering

![ChromaticAberration](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/dbe3554d-057b-4d7c-ab2d-3154159fe808)

### Light Type
Photon emitters emit photons from a spotlight or rectangular light

| Spot Light | Rectangular Light |
| ---- | ---- |
| <img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/4070e03e-e94c-4a32-895b-de311779f358" width="500"> | <img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/1e68f2f4-2c65-4932-bf34-60d59c9c8f84" width="500"> |

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

#### With Sponza
1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Exp1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0b4f954b-4875-4a5f-816b-26174ce90bea)

2. Open sponza.obj on blender. Set the size to 0.1 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "sponza.obj")

4. Create a new folder (ex. "sponza") in "DXRPhotonMapper/PhotonMapper/model" and copy the "textures" and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.

5. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/3e48b1be-90f6-4663-86c0-228fb3d42e87" width="50%">

6. Build and Execute

#### With Bistro
1. Download model(bistro) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Bis](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/274017c1-d198-4dc1-81b2-2fc23efd323f)

2. Open sponza.obj on blender. Set the size to 0.05 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "exterior.obj")

4. Create a new folder (e.g. "bistro") in "DXRPhotonMapper/PhotonMapper/model". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj file in the "Exterior".

5. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/2e1afbf9-3e0c-41f0-8cc0-9bc62251a8cf" width="50%">

6. Build and Execute

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