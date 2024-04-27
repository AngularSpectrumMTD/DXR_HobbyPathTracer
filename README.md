# DXRHybridPhotonMapper
Hybrid Photon Mapper By Use Of DirectX Raytracing  
(Simple Pathtracing + Photon Mapping[Spectral Rendered Caustics])  
![all](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/ee152d98-33a3-4c21-86a4-e51d5bb0e525)

"Next Event Estimation" is implemented  
Left : "NEE" Right : "Simple Pathtracing"
![NEE_comparison](https://github.com/AngularSpectrumMTD/DXR_HybridPhotonMapper/assets/65929274/b060029b-0dc0-49c9-bf97-2c36b79ec325)

### Algorithm
Photon Mapping : Hashed Grid  
Denoiser : SVGF(Currently Disabled)  
Shading : GGX  
Sampling : Next Event Estimation

### Debug View
Enable to check (Diffuse)Albedo / Depth / Normal

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
- T / P : light emission angle(theta / phi)
- O / W : directional light angle(theta / phi)
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
- MOUSE_RIGHT : rotate the camera around the gazing point
- MOUSE_LEFT : move the camera back and forth
- ↑↓→← : move the camera up, down, left, or right