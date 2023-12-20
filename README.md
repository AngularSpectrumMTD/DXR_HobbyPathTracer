# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![transparent](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/ca13c9ef-96a1-4f03-a4a9-f6e287287318)

![rough](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/55e4d168-997c-442b-9853-659e279422ba)

![metallic](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/23849bb3-1823-4f71-b8e6-ca381ff7f607)


### Directional Light / Cube Map

| Directional Light | Cube Map |
| ---- | ---- |
| ![Directional](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/8c98d66e-e775-4309-8bba-dd7546e8e1ce) | ![CubeMap](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/b409dca9-96b2-4b8e-8d74-a04daf6c38da) |

### SVGF

| OFF | ON |
| ---- | ---- |
| ![image](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/157a309e-52b0-485e-9287-ad5ff19f9c52) | ![svgf](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/9cbd093e-028e-4895-b3b3-4b013da78ef3) |

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