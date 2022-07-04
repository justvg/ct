#version 450

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"

layout (location = 0) out vec3 FragAlbedo;
layout (location = 1) out vec3 FragNormal;
layout (location = 2) out vec2 FragMaterial;
layout (location = 3) out vec2 FragVelocity;
layout (location = 4) out float FragLinearDepth;

layout (location = 0) in vec3 NormalWS;
layout (location = 1) in vec3 FragPosWS;
layout (location = 2) in vec3 VoxelPosWS;

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 View;
	mat4 Proj;
	mat4 ProjUnjittered;

	mat4 PrevView;
	mat4 PrevProj;
	
	vec4 CameraPosition; 
    vec4 Viewport; // Width, Height, Near, Far
	vec4 Frustum[6];
    vec4 FrustumCorners[6];
};

layout (set = 0, binding = 2) readonly buffer Voxels
{
	uint VoxelColorMatActive[LevelDimZ][LevelDimY][LevelDimX];
};

void main()
{
	vec3 Normal = normalize(NormalWS);

	int VoxelX = int(floor(VoxelPosWS.x / VoxelDim));
	int VoxelY = int(floor(VoxelPosWS.y / VoxelDim));
	int VoxelZ = int(floor(VoxelPosWS.z / VoxelDim));

	float VoxelColorR = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] >> 24) & 0xFF) / 255.0;
	float VoxelColorG = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] >> 16) & 0xFF) / 255.0;
	float VoxelColorB = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] >> 8) & 0xFF) / 255.0;

	float VoxelReflectivity = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] & 0xF0) >> 4) / 15.0;
	float VoxelRoughness = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] & 0xE) >> 1) / 7.0;

	// TODO(georgii): I think I can do this in vertex shader. I remember that in Crysis we did it in VS
	vec4 CurrentTexCoords = ProjUnjittered * View * vec4(FragPosWS, 1.0);
	CurrentTexCoords.xy /= CurrentTexCoords.w;
	CurrentTexCoords.xy = CurrentTexCoords.xy * vec2(0.5, -0.5) + vec2(0.5, 0.5);

	vec4 PrevTexCoords = PrevProj * PrevView * vec4(FragPosWS, 1.0);
	PrevTexCoords.xy /= PrevTexCoords.w;
	PrevTexCoords.xy = PrevTexCoords.xy * vec2(0.5, -0.5) + vec2(0.5, 0.5);
	
	vec2 Velocity = CurrentTexCoords.xy - PrevTexCoords.xy;

	FragAlbedo = vec3(VoxelColorR, VoxelColorG, VoxelColorB);
	FragNormal = Normal;
	FragMaterial = vec2(VoxelReflectivity, VoxelRoughness);
	FragVelocity = Velocity;
    FragLinearDepth = length(FragPosWS - CameraPosition.xyz) / Viewport.w;
}