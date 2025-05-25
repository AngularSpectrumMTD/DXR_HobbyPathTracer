# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)

Room3

![Image](https://github.com/user-attachments/assets/0a4f5a4c-feb6-4456-8e7b-35655c16047b)

![Image](https://github.com/user-attachments/assets/f9638e9d-714f-42c9-888c-b5cce2b5a893)

![Image](https://github.com/user-attachments/assets/ee7a6be7-2abd-4640-b1e9-887cbda02d6c)

![Image](https://github.com/user-attachments/assets/e112bf30-f3a8-408d-996e-6f16db4e569c)

![Image](https://github.com/user-attachments/assets/645d16ef-61cd-4e4e-ad8a-a039f7496a76)

![Image](https://github.com/user-attachments/assets/e5461695-e2b7-48cc-85da-5648bdc6a69c)

![Image](https://github.com/user-attachments/assets/578b1f1d-0e91-41fc-980d-4d054de2c4c2)

![Image](https://github.com/user-attachments/assets/7fc41304-67e1-4850-a08a-6aa26782168b)

CausticsTest  
![Image](https://github.com/user-attachments/assets/1ad0a7a3-0843-4a2d-9321-e0990e26e826)

![Image](https://github.com/user-attachments/assets/6d0e4aa0-c3b9-4ecb-bee9-25b2f34fe474)

![Image](https://github.com/user-attachments/assets/68b0bbf2-ec02-4857-8873-486380debf2b)

![Image](https://github.com/user-attachments/assets/4761162a-5263-4421-9f66-6433c368416d)

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
![Image](https://github.com/user-attachments/assets/7989bb54-edeb-4b2d-b6bc-93313647f83c)

BistroInterior
![Image](https://github.com/user-attachments/assets/337bb238-cfdb-464c-83e0-d896fc3308bb)

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
![Image](https://github.com/user-attachments/assets/1ab30965-42c8-4ada-8330-37b51093762f)

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_CausticsTest" or "SceneType_Corridor" or "SceneType_PTTestRoom" or "SceneType_PTTestRoom2"  or "SceneType_PTTestRoom3" or "SceneType_PTTestRobot" or "SceneType_PTTestBrick" or SceneType_MaterialTest" or "SceneType_PTTest" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  **(CausticsTest/MaterialTest/Corridor/Kitchen/GITest/PTTest/PTTestBrick/PTTestRobot/PTTestRoom/PTTestRoom2/PTTestRoom3Exp.mtl/obj are made by owner of this repository(AngularSpectrumMTD))**
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

### Textures References
https://ambientcg.com/
https://www.cgbookcase.com/