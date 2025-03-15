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

![Image](https://github.com/user-attachments/assets/5a6ee6b5-7896-44d7-aef9-55491c47c571)

![Image](https://github.com/user-attachments/assets/4776ccb1-032c-44b8-89ad-e369f2caf939)

![Image](https://github.com/user-attachments/assets/2718445f-fe24-4c53-be03-19ee9680daf9)

![Image](https://github.com/user-attachments/assets/4c139b81-9a05-4bb5-a8a7-e47ffff8cc39)

![Image](https://github.com/user-attachments/assets/ea70f742-d7b6-430e-8068-ff13134bc7b6)

![Image](https://github.com/user-attachments/assets/2b12b3d5-b8fd-41d2-848c-10a16ccc27f7)

![Image](https://github.com/user-attachments/assets/e4091777-5256-42af-9cd8-b2c333fbfaa8)

![Image](https://github.com/user-attachments/assets/740bb41f-93d1-483c-a963-2350b914f00f)

Results of the verification of the ReSTIR DI/GI

![Image](https://github.com/user-attachments/assets/0cbe082e-a4e7-4822-8225-31fe062f791f)

![Image](https://github.com/user-attachments/assets/8ddb74ed-572a-47de-a641-34ee3c62ab81)

![Image](https://github.com/user-attachments/assets/593e0571-f1e7-423e-8163-42507e56ee56)

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models are made by AngularSpectrumMTD))
1. Set "SceneType_Corridor" or "SceneType_PTTestRoom" or "SceneType_PTTestRobot" or "SceneType_PTTestBrick" or SceneType_MaterialTest" or "SceneType_PTTest" or "SceneType_GITest" or "SceneType_Kitchen" to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp]  
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