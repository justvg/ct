@echo off

REM Add -g to DebugFlags to compile debug information for shaders
REM Add -Od to DebugFlags to disable optimizations for shaders

set DebugFlags=

%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\BloomBrightness.frag.glsl" -o Content\Shaders\BloomBrightness.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\BloomDownscale.comp.glsl" -o Content\Shaders\BloomDownscale.comp.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\BloomUpscale.frag.glsl" -o Content\Shaders\BloomUpscale.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Brightness.frag.glsl" -o Content\Shaders\Brightness.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Debug.vert.glsl" -o Content\Shaders\Debug.vert.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Debug.frag.glsl" -o Content\Shaders\Debug.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\DiffuseLighting.frag.glsl" -o Content\Shaders\DiffuseLighting.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\DiffuseLightingDenoise.frag.glsl" -o Content\Shaders\DiffuseLightingDenoise.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Door.frag.glsl" -o Content\Shaders\Door.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Downscale.comp.glsl" -o Content\Shaders\Downscale.comp.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Exposure.frag.glsl" -o Content\Shaders\Exposure.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Fireball.frag.glsl" -o Content\Shaders\Fireball.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\FirstPerson.frag.glsl" -o Content\Shaders\FirstPerson.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Fullscreen.vert.glsl" -o Content\Shaders\Fullscreen.vert.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\FullscreenLighting.vert.glsl" -o Content\Shaders\FullscreenLighting.vert.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\HUD.vert.glsl" -o Content\Shaders\HUD.vert.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\HUD.frag.glsl" -o Content\Shaders\HUD.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Mesh.vert.glsl" -o Content\Shaders\Mesh.vert.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Mesh.frag.glsl" -o Content\Shaders\Mesh.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\MeshTransparent.frag.glsl" -o Content\Shaders\MeshTransparent.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\MipGeneration.comp.glsl" -o Content\Shaders\MipGeneration.comp.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Portal.frag.glsl" -o Content\Shaders\Portal.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\SpecularLighting.frag.glsl" -o Content\Shaders\SpecularLighting.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\SpecularLightingDenoise.frag.glsl" -o Content\Shaders\SpecularLightingDenoise.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Fog.frag.glsl" -o Content\Shaders\Fog.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\TAA.frag.glsl" -o Content\Shaders\TAA.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Tonemapping.frag.glsl" -o Content\Shaders\Tonemapping.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Voxel.vert.glsl" -o Content\Shaders\Voxel.vert.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\Voxel.frag.glsl" -o Content\Shaders\Voxel.frag.spv
%VULKAN_SDK%\Bin\glslangValidator -V %DebugFlags% "Code\Shaders\VoxelCulling.comp.glsl" -o Content\Shaders\VoxelCulling.comp.spv