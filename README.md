# DXR_HobbyPathTracer
Basic Path Tracer (Implemented in DirectX Raytracing)

### Algorithm
- ReSTIR DI
- ReSTIR GI
- Emission Guided Photon Mapping  

Results of the verification of the ReSTIR   

<img width="1242" height="695" alt="Image" src="https://github.com/user-attachments/assets/84b9df98-0420-429e-bac8-08b63eecb912" />

### Results (SceneType_Room)

<img width="1269" height="712" alt="Image" src="https://github.com/user-attachments/assets/b9110711-ff09-4aef-8088-1eaca89f2ab7" />

### Debug View
Enable to check Albedo / Depth / Normal / Roughness / Metallic / Specular / Translucent Color / Emission Color     

### How To Use
**NOTE : This program can only handle triangular polygons**

---
#### With Simple Scene (Easy. These 3D models(,obj/.mtl) are made by AngularSpectrumMTD))
1. Set 
"SceneType_PTTest (PTTest.obj/mtl)"  ,
"SceneType_PTTestBrick (PTTestBrick.obj/mtl)"  ,
"SceneType_PTTestRobot (PTTestRobot.obj/mtl)"  ,
"SceneType_PTTestRoom (PTTestRoom.obj/mtl)"  ,
"SceneType_PTTestRoom2 (PTTestRoom2.obj/mtl)"  ,
"SceneType_PTTestRoom3 (PTTestRoom3Exp.obj/mtl)"  ,
"SceneType_PTTestRoom4 (PTTestRoom4Exp.obj/mtl)"  ,
"SceneType_PTTestRoom5 (PTTestRoom5.obj/mtl)"  ,
"SceneType_MaterialTest (MaterialTest.obj/mtl)"  ,
"SceneType_Corridor (Corridor.obj/mtl)"  ,
"SceneType_CausticsTest (CausticsTest.obj/mtl)"  ,
"SceneType_EmissionTest (EmissionTest.obj/mtl)"  ,
"SceneType_ResamplingTest (ResamplingTest.obj/mtl)"  ,
"SceneType_CornellBox (CornellBox.obj/mtl)"  ,
"SceneType_MetallicTest (MetallicTest.obj/mtl)"  ,
"SceneType_Room (room.obj/mtl)"  [Recommended]
to the variable "mSceneType" at void DxrPhotonMapper::Setup()[DxrPhotonMapper.cpp] 
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