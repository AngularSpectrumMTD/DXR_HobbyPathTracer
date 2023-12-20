# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![transparent](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/3a8e2fed-b438-49db-98c4-921845b53b9e)

![metallic](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/5c848d8f-9e5b-4509-9636-d344f1608595)

![rough](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/b1d28d38-900a-46e1-8411-29594e7a887d)

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
1.Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this



    mOBJFileName = "crytekSponza.obj";
    mOBJFolderName = "model/crytekSponza";



2.Build and Execute

**NOTE : This program can only handle triangular polygons**

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