# DXRHybridPhotonMapper
Hybrid Photon Mapper By Use Of DirectX Raytracing  
(Pathtracing[NEE + WRS based RIS] + Photon Mapping[Spectral Rendered Caustics])  
![ManyLights](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/8e7185df-84c7-48ed-8f7a-d3822337dcac)
![RIS_converged](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/036da7b4-9524-45be-9569-7cf2c9228f16)
![all](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/4d1a14ce-2f81-4827-a33c-b6b22ec8bdf6)

"Next Event Estimation" is implemented  
Left : "NEE" / Right : "Simple Pathtracing"
![NEE_comparison](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/b060029b-0dc0-49c9-bf97-2c36b79ec325)

Weighted Reservoir Sampling based Resampled Importance Sampling is implemented for many lights  
Left :  WRS (30 stream inputs from 400 lights. Not Accumulated) / Right : Uniform Sampling (from 400 lights. Not Accumulated)  
![RIS_comparison](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/da8a02e4-09c3-4a17-a0a5-3d02f4c2e875)

This sampling technique can easily generate beautiful rendering images of scenes with many light sources.  
Left :  WRS (30 stream inputs from 400 lights. 400 frame Accumulated) / Right : Uniform Sampling (from 400 lights. 400 frame Accumulated)  
![RIS_comparison_converged](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/fc76ea80-ea54-4baf-854d-0c925ead0d5a)

### Algorithm
Photon Mapping : Hashed Grid  
Denoiser : SVGF (Currently Disabled)  
Shading : GGX  
Sampling : Next Event Estimation / Streaming RIS

### Debug View
Enable to check (Diffuse)Albedo / Depth / Normal  
![debugView](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/cc8bf838-8cfd-4ba9-90cc-1b9174afe542)

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