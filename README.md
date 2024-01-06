# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/1f0d89c7-7df6-4b1c-921f-3bae93bde4d2)

![2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/42298d2c-b18d-4689-8897-bdb85b66a55b)

![3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/9182cbad-51b2-46cb-8c27-534e976bffcf)

![5](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/d957f59e-bed4-4eee-b2ec-99434508c757)

### Caustics
Caustics are rendered by spectral rendering

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

![normal](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/9b87a3e7-5f84-4cb5-be00-c0c1881d6b94)

#### With Sponza
1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Exp1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0b4f954b-4875-4a5f-816b-26174ce90bea)

2. Open sponza.obj on blender. Set the size to 0.1 -> Execute Triangulation.

4. Export 3D model as obj file(Name the file "sponza.obj")

5. Create a new folder (ex. "sponza") in "DXRPhotonMapper/PhotonMapper/model" and copy the "textures" and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.

6. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/3e48b1be-90f6-4663-86c0-228fb3d42e87" width="50%">

7. Build and Execute

![sponza](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/efa9e347-1ef1-4ac4-b144-ff4e1ded7f73)

#### With Bistro
1. Download model(bistro) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Bis](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/274017c1-d198-4dc1-81b2-2fc23efd323f)

2. Open sponza.obj on blender. Set the size to 0.05 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "exterior.obj")

4. Create a new folder (e.g. "bistro") in "DXRPhotonMapper/PhotonMapper/model". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj file in the "Exterior".

5. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

<img src="https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/2e1afbf9-3e0c-41f0-8cc0-9bc62251a8cf" width="50%">

6. Build and Execute

![bistro](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/1099a01c-64cd-4d96-a5f5-2473f3e62b26)

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
- MOUSE_RIGHT : rotate the camera around the gazing point
- MOUSE_LEFT : move the camera back and forth
- ↑↓→← : move the camera up, down, left, or right