# DXRHybridPhotonMapper
Hybrid Photon Mapper By Use Of DirectX Raytracing  
(Pathtracing[NEE + Streaming RIS(DI = ReSTIR)] + Photon Mapping[Spectral Rendered Caustics)  

https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/65fa88a4-5a59-45cc-8520-5cf09ed7a195

![ReSTIRDI](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/0e17407d-f343-46b3-8925-34ddb2c181f6)

![ReSTIRDI2](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/1ea78419-94cc-4719-afcf-6ac914a0c1a5)

![Caustics](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/0d450f8b-279a-494e-b640-0cad271a81f4)

"Next Event Estimation" and Weighted Reservoir Sampling based Resampled Importance Sampling is implemented for many lights.      
Right : Uniform Sampling (from 400 lights. Not Accumulated) 
Left : Streaming RIS (30 stream inputs from 400 lights. Not Accumulated)  
![RIS_comparison](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/325660bd-0008-4a77-848f-56c600b32054)

This sampling technique can easily generate beautiful rendering images of scenes with many light sources.  
(Doing) ReSTIR DI is implemented for improving the quality RIS.   
Left : Initial Sampling  
Right : Spatial Reuse  
![RIS_comparison_Spatial](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/12bcfad7-f1f3-4a26-a091-f04c88132674)

### Algorithm
Photon Mapping : Hashed Grid  
Denoiser : SVGF (Currently Disabled)  
Shading : GGX  
Sampling : Next Event Estimation / ReSTIR(DI) / Streaming RIS

### Debug View
Enable to check (Diffuse)Albedo / Depth / Normal  
![debugView](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/84182a96-4723-494a-b478-df1070e60d4a)

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