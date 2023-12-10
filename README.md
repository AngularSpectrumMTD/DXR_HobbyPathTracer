# DXRPhotonMapper
Photon Mapper By Use Of DirectX Raytracing

![image](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/61e15fa0-4ca3-4df4-ae53-a6c3fb9c7a9e)

![Beautiful](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/4e7c237a-4c42-4098-b627-ff0582b509b3)

![caustics](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/80c8eb25-4e5a-47ab-8071-e4a96004cfb7)

![SVGF](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/afbf735e-1958-4975-9843-abd79c8d8288)

![models](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/5e011057-4728-4d4d-979d-1e8f254361ed)

### Algorithm
Photon Mapping : Hashed Grid

Denoiser : SVGF

Shading : GGX

### How To Use
1.Set ｍOBJFileName and mOBJFolderName at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] like this

![setting](https://github.com/AngularSpectrumMTD/DXR_PhotonMapper/assets/65929274/207261ca-41ed-4a5f-a64d-8a2e989270b2)

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
- C : increase / decrease temporal accumulation factor(EMA)
- V : visualize DI emission element
- N : visualize caustics
- D : enable / disable SVGF
- Q : increase / decrease caustics boost
- U : enable / disable texture
- R : increase / decrease roughness
- S : increase / decrease translucency
- M : increase / decrease metallic