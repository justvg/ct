@echo off

REM Add -g to DebugFlags to compile debug information for shaders
REM Add -Od to DebugFlags to disable optimizations for shaders

set VulkanSDKPath=C:\VulkanSDK\1.3.211.0\
set DebugFlags=

%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\BloomBrightness.frag.glsl" -o Content\Shaders\BloomBrightness.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\BloomDownscale.comp.glsl" -o Content\Shaders\BloomDownscale.comp.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\BloomUpscale.frag.glsl" -o Content\Shaders\BloomUpscale.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Brightness.frag.glsl" -o Content\Shaders\Brightness.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Debug.vert.glsl" -o Content\Shaders\Debug.vert.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Debug.frag.glsl" -o Content\Shaders\Debug.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Downscale.comp.glsl" -o Content\Shaders\Downscale.comp.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Exposure.frag.glsl" -o Content\Shaders\Exposure.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Fullscreen.vert.glsl" -o Content\Shaders\Fullscreen.vert.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\HUD.vert.glsl" -o Content\Shaders\HUD.vert.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\HUD.frag.glsl" -o Content\Shaders\HUD.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Mesh.vert.glsl" -o Content\Shaders\Mesh.vert.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Mesh.frag.glsl" -o Content\Shaders\Mesh.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\MeshTransparent.frag.glsl" -o Content\Shaders\MeshTransparent.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\MipGeneration.comp.glsl" -o Content\Shaders\MipGeneration.comp.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Particle.vert.glsl" -o Content\Shaders\Particle.vert.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Particle.frag.glsl" -o Content\Shaders\Particle.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\TAA.frag.glsl" -o Content\Shaders\TAA.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Tonemapping.frag.glsl" -o Content\Shaders\Tonemapping.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Voxel.vert.glsl" -o Content\Shaders\Voxel.vert.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Voxel.frag.glsl" -o Content\Shaders\Voxel.frag.spv
%VulkanSDKPath%Bin\glslangValidator -V %DebugFlags% "Code\Shaders\VoxelCulling.comp.glsl" -o Content\Shaders\VoxelCulling.comp.spv