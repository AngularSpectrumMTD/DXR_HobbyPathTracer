# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![bistro1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/28d60b62-47e2-4d28-b6fa-a720b66c5c0e)

![bistro2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/f6ff4f52-4e9f-4341-95ed-518159c9caf0)

![bistro3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/2929e0e5-255c-495e-8d4d-db103bd23164)

![bistro4](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/5bb2f5e1-921c-4904-be38-b38e26cd8c00)

![transparent](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/4858ab5c-ba06-4526-b9fb-b1dddeac4742)

![metallic](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/3f97fb0c-5980-477d-8e73-8ab642776e72)

![rough](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/8a75a7b8-14b1-4805-93d5-54ecb47f6938)

### Directional Light / Cube Map

| Directional Light | Cube Map |
| ---- | ---- |
| ![Directional](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/85d5195e-f664-4037-824f-36c812d78b3d) | ![CubeMap](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/19fc8f4f-9e6a-425c-8542-239c02c7ebc7) |

### Caustics
Caustics are rendered by spectral rendering

![spectral](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/d7580bab-4ba6-4408-8c1a-788c410f7536)

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### How To Use
**NOTE : This program can only handle triangular polygons**

#### Normal
1.Build and Execute

![sample](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/6ab59527-27d9-4606-9d9f-465ba5dd9673)

#### With Sponza
1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Exp1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0b4f954b-4875-4a5f-816b-26174ce90bea)

2. Open sponza.obj on blender. Set the size to 0.1 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "sponza.obj")

4. Create a new folder (ex. "sponza") in "DXRPhotonMapper/PhotonMapper/model" and copy the "textures" and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.

5. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

```
    mOBJFileName = "sponza.obj";
    mOBJFolderName = "model/sponza";
```

6. Build and Execute

#### With Bistro
1. Download model(bistro) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Bis](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/274017c1-d198-4dc1-81b2-2fc23efd323f)

2. Open sponza.obj on blender. Set the size to 0.05 -> Execute Triangulation.

3. Export 3D model as obj file(Name the file "exterior.obj")

4. Create a new folder (e.g. "bistro") in "DXRPhotonMapperPhotonMappermodel". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj file in the "Exterior".

5. Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

```
    mOBJFileName = "exterior.obj";
    mOBJFolderName = "model/bistro/Exterior";
```

6. Build and Execute

### Graphics API
DirectX12

### Action

- I : determine other button meaning (+ / -)
- J : start / stop model
- H : enable / disable Directional Light
- G : increase / decrease photon gather radius
- X / Y / Z : light position
- L : increase / decrease light emission range
- T / P : light emission angle(theta / phi)
- K : increase / decrease light intensity
- B : increase / decrease photon gathering block num
- V : visualize DI emission element
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