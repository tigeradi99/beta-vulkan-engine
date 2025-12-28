# Vulkan Renderer

[Sponza](https://github.com/tigeradi99/beta-vulkan-engine/blob/master/assets/x_engine_1.png)
[Amazon Lumberyard](https://github.com/tigeradi99/beta-vulkan-engine/blob/master/assets/x_engine_2.png)

### Introduction
This is a Vulkan rendering engine that I built after following Brendan Galea's Vulkan tutorials on YouTube. 

Link to the tutorial series: https://youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR&si=npyVk1wUucQ-8uEf

### Requirements
If you are interested in running this on your system, ensure that you use Vulkan SDL version 1.3.268.0 .
Rest of the external dependencies such as stb_image, Assimp, GLM, Dear ImGUI, GLFW etc. have been added as submodules in the ```external/``` subdirectory.

### Features added

Some features added to this renderer project:
 - Directional Lighting
 - Cascaded Shadow Mapping
 - Basic Texture/Material management
 - Memory management with VMA
