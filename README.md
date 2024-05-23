# DXRHybridPhotonMapper
Hybrid Photon Mapper By Use Of DirectX Raytracing  
(Pathtracing[NEE + Streaming RIS(DI = ReSTIR)] + Photon Mapping[Spectral Rendered Caustics)  

![resultExt](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/ee53f0ea-6a3b-4a17-8344-e527c493a7ac)
![resultSponza](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/2ebaa9db-fc76-4a98-8d08-58c44d4afcf8)
![resultInt](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/f43b6a0d-582a-435d-93df-aead936b932f)
![Caustics](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/0d450f8b-279a-494e-b640-0cad271a81f4)

"Next Event Estimation" and Weighted Reservoir Sampling based Resampled Importance Sampling is implemented for many lights. 

This sampling technique can easily generate beautiful rendering images of scenes with many light sources.  
(Doing) ReSTIR DI is implemented for improving the quality RIS.   

### Algorithm
Photon Mapping : Hashed Grid  
Denoiser : SVGF (Currently Disabled)  
Shading : GGX  
Sampling : Next Event Estimation / ReSTIR(DI) / Streaming RIS

### Debug View
Enable to check (Diffuse)Albedo / Depth / Normal  / Velocity

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy)
1. Set "SceneType_Simple" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  
2. Build and Execute  
---
#### With Sponza
1. Download model(sponza) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)  
2. Open sponza.obj on blender. Set the size to 0.1 -> Execute Triangulation.  
4. Export 3D model as obj file(Name the file "sponza.obj")  
5. Create a new folder "sponza" in "DXRPhotonMapper/PhotonMapper/model" and copy the "textures" and "sponza.mtl" file from the downloaded folder and the obj file you just exported to the folder you just created.  
6. Set "SceneType_Sponza" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  
7. Build and Execute
---
#### With Bistro
1. Download model(bistro) from "McGuire Computer Graphics Archive"(https://casual-effects.com/data/)  
2. Open exterior.obj / interior.obj on blender. Set the size to 0.1 -> Execute Triangulation.  
3. Export 3D model as obj file(Name the file "exterior.obj"/"interior.obj")  
4. Create a new folder "bistro" in "DXRPhotonMapper/PhotonMapper/model". Then copy the "BuildingTextures" and "Exterior" and "OtherTextures" and "PropTextures" from the downloaded folder to the created folder. Then place the created exterior.obj / interior.obj file in the "Exterior" / "Interior".  
5. Set "SceneType_BistroExterior" / "SceneType_BistroInterior" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  
6. Build and Execute  
---

### Graphics API
DirectX12

### Action

- I : determine other button meaning (+ / -)
- J : start / stop model
- E : enable / disable NEE
- G : increase / decrease photon gather radius
- X / Y / Z : light position
- L : increase / decrease light emission range
- T / P : light emission angle (theta / phi)
- O / W : directional light angle (theta / phi)
- K : increase / decrease light intensity
- B : increase / decrease photon gathering block num
- N : visualize caustics
- D : increase / decrease number of bounce
- Q : increase / decrease caustics boost
- U : enable / disable texture
- R : increase / decrease roughness
- S : increase / decrease translucency
- M : increase / decrease metallic
- A : enable / disable direct lighting
- C : change photon emitter type (spot light/rectangular light)
- V : enable / disable debug view
- F : enable / disable lighting by use of many sphere lights
- MOUSE_RIGHT : rotate the camera around the gazing point
- MOUSE_LEFT : move the camera back and forth
- ↑↓→← : move the camera up, down, left, or right
- SPACE : change the target to edit the material
- CTRL : enable / disable Streaming RIS
- TAB : enable / disable Temporal Accumulation  
- F1 : enable / disable Temporal Reuse of Reservoir
- F3 : enable / disable Spatial Reuse of Reservoir
- F4 : increase / decrease the number of taps for Spatial Reuse of Reservoir
- F5 : enable / disable Metal Test