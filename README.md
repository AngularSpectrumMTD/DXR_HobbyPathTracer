# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)

<img width="1992" height="1121" alt="Image" src="https://github.com/user-attachments/assets/a03f9458-ca04-4dd0-8f10-f7d8c46faaf7" />

### Algorithm
- ReSTIR DI
- ReSTIR GI
- Emission Guided Photon Mapping  

Results of the verification of the ReSTIR
<img width="2652" height="1508" alt="Image" src="https://github.com/user-attachments/assets/77c42b62-a992-4d6f-89da-4d42ff6d1ce4" />

### Results
Room5

<img width="1994" height="1120" alt="Image" src="https://github.com/user-attachments/assets/be39470b-f2a3-4868-814b-e59dfc523078" />

<img width="1994" height="1125" alt="Image" src="https://github.com/user-attachments/assets/6fa602db-4824-4a7a-9c19-0512c94f0be1" />

<img width="1991" height="1122" alt="Image" src="https://github.com/user-attachments/assets/acfde6a6-d8e9-4d39-86e2-9d45563c5be9" />

<img width="1999" height="1122" alt="Image" src="https://github.com/user-attachments/assets/21d340ae-a822-4f9e-8f06-edfd2c93b82b" />

<img width="1990" height="1121" alt="Image" src="https://github.com/user-attachments/assets/eec792bb-60d6-4480-9408-43d2f15ab5cf" />

BistroExterior
<img width="1999" height="1118" alt="Image" src="https://github.com/user-attachments/assets/a4855cd1-4e62-4b2c-ba6b-e79bb0aed5ad" />

Room2
![Image](https://github.com/user-attachments/assets/cccb9c7f-4f46-4e41-b3a2-3246f3ef1df8)

EmissionTest
![Image](https://github.com/user-attachments/assets/64500e4a-599e-4ef2-94e6-6aafb4a17efb)

Room4
![Image](https://github.com/user-attachments/assets/4308a0d8-2649-4947-909a-5b87e9d66003)

Room3
![Image](https://github.com/user-attachments/assets/0a4f5a4c-feb6-4456-8e7b-35655c16047b)

CausticsTest  
<img width="1996" height="1117" alt="Image" src="https://github.com/user-attachments/assets/d65d6905-a0da-4d03-8c64-73e1448ba7f5" />

MaterialTest  
<img width="1995" height="1114" alt="Image" src="https://github.com/user-attachments/assets/f1f65b94-3ce7-4e64-b656-6633c8fd24c3" />

Corridor
![Image](https://github.com/user-attachments/assets/f0493f5d-c690-47b9-9e86-ec89efe15070)

Sponza
![Image](https://github.com/user-attachments/assets/ec2c5244-89da-46b2-80f4-4b1b6831e48b)
![Image](https://github.com/user-attachments/assets/4a92bbbc-4fe8-4758-a0ed-7645a4f53758)

BistroInterior
<img width="1993" height="1124" alt="Image" src="https://github.com/user-attachments/assets/3eec9d5f-a26c-43aa-b0fc-39bbb438789e" />

Brick
<img width="1991" height="1117" alt="Image" src="https://github.com/user-attachments/assets/7ecb72c7-4641-4893-9757-8e905be811ad" />

Robot
<img width="1999" height="1122" alt="Image" src="https://github.com/user-attachments/assets/19455117-c511-4264-8f17-4f368396160b" />

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

![Image](https://github.com/user-attachments/assets/2bedc671-c2eb-4b27-8c42-121932bfc6fe)

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_CausticsTest" or "SceneType_EmissionTest"or "SceneType_ResamplingTest" or "SceneType_Corridor" or "SceneType_PTTestRoom" or "SceneType_PTTestRoom2"  or "SceneType_PTTestRoom3" or "SceneType_PTTestRoom4" or "SceneType_PTTestRoom4" or "SceneType_PTTestRoom5" or "SceneType_PTTestRobot" or "SceneType_PTTestBrick" or SceneType_MaterialTest" or "SceneType_PTTest" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  **(EmissionTest/CausticsTest/MaterialTest/Corridor/Kitchen/GITest/PTTest/PTTestBrick/PTTestRobot/PTTestRoom/PTTestRoom2/PTTestRoom3Exp/PTTestRoom4Exp.mtl/obj are made by owner of this repository(AngularSpectrumMTD))**
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
- H : set "(1, 1, 1)" to value of albedo
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

### Textures References
https://ambientcg.com/
https://www.cgbookcase.com/