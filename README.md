# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)
- ReSTIR DI
- ReSTIR GI (Prototype)
- Emission Guided Photon Mapping  

**NOTE : 
MaterialTest.mtl/obj
Corridor.mtl/obj
Kitchen.mtl/obj
GITest.mtl/obj
PTTest.mtl/obj
PTTestBrick.mtl/obj
PTTestRobot.mtl/obj
PTTestRoom.mtl/obj
roomTestExp.mtl/obj 
are made by owner of this repository(AngularSpectrumMTD)**

![Image](https://github.com/user-attachments/assets/1441ae05-31d2-42c0-baee-9e77678237d8)

![Image](https://github.com/user-attachments/assets/7b03603d-8893-401c-8d73-9f8fce60c930)

![Image](https://github.com/user-attachments/assets/758e3e65-4f1a-49bd-9efe-3be881d80698)

![Image](https://github.com/user-attachments/assets/de8a84f6-ac51-4e41-b163-6ffa5c00728e)

![Image](https://github.com/user-attachments/assets/37914beb-863a-4990-add4-938f01510a7e)

![Image](https://github.com/user-attachments/assets/00de085d-804f-4fc5-a531-c3a928daacf8)

![Image](https://github.com/user-attachments/assets/af7fb92e-c273-4f0c-b311-a14c15783022)

![Image](https://github.com/user-attachments/assets/e903502b-3c03-4df0-85db-23f6701ada9d)

Results of the verification of the ReSTIR DI/GI

![Image](https://github.com/user-attachments/assets/baaa5071-d8e1-4c85-a36f-fadffa6fbe27)

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_PTTestRoom" or "SceneType_PTTestRobot" or "SceneType_PTTestBrick" or SceneType_MaterialTest" or "SceneType_PTTest" or "SceneType_GITest" or "SceneType_Kitchen" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  
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
- F9 : enable / disable Emissive Polygons
- F11 : enable / disable Median Filter