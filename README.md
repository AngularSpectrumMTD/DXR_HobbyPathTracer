# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)

https://github.com/user-attachments/assets/8d6d50cd-3ef6-43cc-89b2-e760781940da

MaterialTest  
![Image](https://github.com/user-attachments/assets/c0639c7c-a2b8-45f0-bd41-55d1d2165b56)

Corridor
![Image](https://github.com/user-attachments/assets/f0493f5d-c690-47b9-9e86-ec89efe15070)

Sponza
![Image](https://github.com/user-attachments/assets/3fc94f4d-a5fa-46f4-8bff-707b4a965a2c)

BistroExterior
![Image](https://github.com/user-attachments/assets/5937affe-b9de-4c75-904f-451a96efcaa3)

BistroInterior
![Image](https://github.com/user-attachments/assets/981619e1-03a7-47dd-8694-b29a7d22bb86)

Brick
![Image](https://github.com/user-attachments/assets/47c50ff5-509d-4b15-8819-631598e45860)

Robot
![Image](https://github.com/user-attachments/assets/7f7fd6ca-1521-4ab3-ad51-1e5a44171802)

Room
![Image](https://github.com/user-attachments/assets/4060169f-233c-406c-9e4f-ea0af8c8db94)

### Algorithm
- ReSTIR DI
- ReSTIR GI
- Emission Guided Photon Mapping  

Results of the verification of the ReSTIR DI/GI  
![Image](https://github.com/user-attachments/assets/219bf28d-36bb-431c-9970-fda52eecacfb)

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_Corridor" or "SceneType_PTTestRoom" or "SceneType_PTTestRobot" or "SceneType_PTTestBrick" or SceneType_MaterialTest" or "SceneType_PTTest" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  (MaterialTest/Corridor/Kitchen/GITest/PTTest/PTTestBrick/PTTestRobot/PTTestRoom.mtl/obj are made by owner of this repository(AngularSpectrumMTD))
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
- F12 : increase / decrease the value of Exposure