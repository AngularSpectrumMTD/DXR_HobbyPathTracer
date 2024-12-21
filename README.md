# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)
- ReSTIR DI
- ReSTIR GI (Prototype)
- Emission Guided Photon Mapping  

Scene Name "PTTestBrick"(Made by AngularSpectrumMTD)
![Test2](https://github.com/user-attachments/assets/b6f73506-9174-45e5-85f0-60d3a81bd112)

Scene Name "PTTest"(Made by AngularSpectrumMTD)
![Test](https://github.com/user-attachments/assets/5ae6ce91-c5c4-48cc-a4be-493337a29c80)

Scene Name "BistroExtorior"(External Resource)
![Test3](https://github.com/user-attachments/assets/831c97c5-fa21-473d-a3d0-123a9cb1e2ae)

![ReSTIR2](https://github.com/user-attachments/assets/465e2f34-8aeb-4c13-afb0-7e476029f5d3)

![ReSTIR1](https://github.com/user-attachments/assets/3077561f-84b1-4779-b617-f21332fe0775)

![comparison](https://github.com/user-attachments/assets/627a5dc6-fb2a-4d77-b5aa-7259d93e1738)

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     
![debugView](https://github.com/user-attachments/assets/125c907f-bc08-4971-828c-8b1b0e9a1598)  
![debugView2](https://github.com/user-attachments/assets/7fd86339-24fe-4128-800b-9d160555b9b5)

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_PTTestBrick" or ""SceneType_PTTest" or "SceneType_GITest" or "SceneType_Kitchen" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  
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

### Control

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
- MOUSE_RIGHT : rotate the camera around the camera position
- ↑↓→← : move the camera forward, backward, left, or right
- SPACE : change the target to edit the material
- CTRL : enable / disable Streaming RIS
- TAB : enable / disable Temporal Accumulation  
- F1 : enable / disable Temporal Reuse of Reservoir
- F3 : enable / disable Spatial Reuse of Reservoir
- F4 : increase / decrease the number of taps for Spatial Reuse of Reservoir
- F5 : enable / disable Metal Test
- F6 : enable / disable SSS
- F7 : enable / disable IBL
- F8 : enable / disable Directional Light