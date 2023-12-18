# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

Pure Image
![image](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/157a309e-52b0-485e-9287-ad5ff19f9c52)

SVGF
![svgf](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/9cbd093e-028e-4895-b3b3-4b013da78ef3)

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

- I : determine other button meaning (+ / -)
- J : start / stop model
- G : increase / decrease photon gather radius
- X / Y / Z : light position
- L : increase / decrease light emission range
- T / P : light emission angle(theta / phi)
- K : increase / decrease light intensity
- B : increase / decrease photon gathering block num
- V : visualize DI emission element
- N : visualize caustics
- D : enable / disable SVGF
- Q : increase / decrease caustics boost
- U : enable / disable texture
- R : increase / decrease roughness
- S : increase / decrease translucency
- M : increase / decrease metallic