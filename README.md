# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![transparent](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/7ec60056-ac8f-4bbd-b8fe-45d7fca8152e)

![metallic](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/28c568a8-f44a-427c-ba14-5bbadadeb8ce)

![rough](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/84050c16-426a-4929-8ce9-bcbe8331d384)


### Directional Light / Cube Map

| Directional Light | Cube Map |
| ---- | ---- |
| ![directional](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/f79b543c-3cb1-4fa0-ab90-2298759cb7d1) | ![cubemap](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0ab775c7-04ee-46c7-8eb7-024662984328) |

### Caustics

| Reflected Ray | Refracted Ray |
| ---- | ---- |
| ![caustics_reflectedRay](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/6d3c87d1-36fd-43e4-acb2-a056928092e4) | ![caustics_refractedRay](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/824763a0-cd0a-497e-9345-a10cb7809055) |

Caustics are rendered by spectral rendering

![spectral](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/73d3736b-fb17-4b0b-a05e-329a3c5fb193)

### SVGF

| OFF | ON |
| ---- | ---- |
| ![nodenoise](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/765e0837-9594-42aa-9025-0656e9ba448d) | ![denoise](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/c333b130-b252-4511-bfdd-b4e586f8809a) |

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### How To Use
**NOTE : This program can only handle triangular polygons**

1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)

![Exp1](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/0b4f954b-4875-4a5f-816b-26174ce90bea)

2. Open sponza.obj on blender, and do these actions.

![Exp2](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/317aa562-2fbb-4605-badc-c04a505ff24d)

3. Export 3D model as obj file(Name the file "sponza.obj")

![Exp3](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/211b09be-de71-4165-b8bb-a9a78914ebd8)

4.Create a new folder (ex. "sponza") in "DXRPhotonMapper\PhotonMapper\model" and copy the "textures" folder and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.

5.Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

    mOBJFileName = "sponza.obj";
    mOBJFolderName = "model/sponza";

6.Build and Execute

### Graphics API
DirectX12

### Action

- A : enable / disable temporal accumulation
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