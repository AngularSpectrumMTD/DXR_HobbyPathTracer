# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)

CausticsTest  
![Image](https://github.com/user-attachments/assets/cd86e3f1-dc4e-4198-9503-1a5b800977cc)

Room2

![Image](https://github.com/user-attachments/assets/306eb43a-a478-49f2-a730-5a3fa13786ce)

![Image](https://github.com/user-attachments/assets/d7d710eb-7a70-4c6c-ad19-010bc7a04dfa)

MaterialTest  
![Image](https://github.com/user-attachments/assets/c0639c7c-a2b8-45f0-bd41-55d1d2165b56)

Corridor
![Image](https://github.com/user-attachments/assets/f0493f5d-c690-47b9-9e86-ec89efe15070)

Sponza
![Image](https://github.com/user-attachments/assets/ec2c5244-89da-46b2-80f4-4b1b6831e48b)
![Image](https://github.com/user-attachments/assets/4a92bbbc-4fe8-4758-a0ed-7645a4f53758)

BistroExterior
![Image](https://github.com/user-attachments/assets/67305315-6771-4c90-90fd-35b34a599bb2)
![Image](https://github.com/user-attachments/assets/cda019d7-532a-48dd-b8d5-7074ac934a89)

BistroInterior
![Image](https://github.com/user-attachments/assets/3570e10b-135a-43ed-940b-f42e4fa7d7dd)
![Image](https://github.com/user-attachments/assets/d839ecc6-bc7c-4b63-9ea2-f1a09e011d84)

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
![Image](https://github.com/user-attachments/assets/38b3b351-aac5-44bf-a2bb-6f46c003b43d)


### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_CausticsTest" or "SceneType_Corridor" or "SceneType_PTTestRoom" or "SceneType_PTTestRoom2" or "SceneType_PTTestRobot" or "SceneType_PTTestBrick" or SceneType_MaterialTest" or "SceneType_PTTest" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  **(CausticsTest/MaterialTest/Corridor/Kitchen/GITest/PTTest/PTTestBrick/PTTestRobot/PTTestRoom/PTTestRoom2.mtl/obj are made by owner of this repository(AngularSpectrumMTD))**
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