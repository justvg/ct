#version 450

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 NormalWS;
layout (location = 1) in vec3 FragPosWS;
layout (location = 2) in vec3 FragPrevPosWS;
layout (location = 3) in vec3 Color;

layout (push_constant) uniform PushConstants
{
	vec4 Position; // w - unused
	vec4 Scale; // w - unused
	vec4 Orientation;
	vec4 MeshColor;
	vec4 Offset; // w contains point lights count

	vec4 PrevPosition; // w - unused
	vec4 PrevOrientation; // w - unused

    uint FrameNumber;
	uint FPWeaponDepthTest;
};

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
};

layout (set = 0, binding = 1) readonly buffer Voxels
{
	uint VoxelActive[LevelDimZ][LevelDimY][LevelDimX];
};

layout (set = 0, binding = 2) uniform sampler2D BlueNoiseTexture;

layout (set = 0, binding = 3) uniform LightBuffer
{
	vec4 AmbientColor; // w - unused
	vec4 AmbientConstant; // w - unused
};

#include "ForwardShading.incl.glsl"

layout (set = 0, binding = 4) readonly buffer PointsLights
{
	SPointLight PointLight[];
};

void main()
{
	vec3 Normal = normalize(NormalWS);

	vec3 Ambient = AmbientColor.rgb;
	
	vec3 Diffuse = vec3(0.0, 0.0, 0.0);
	uint PointLightCount = uint(Offset.w);
	for (uint I = 0; I < PointLightCount; I++)
	{
		Diffuse += CalculatePointLight(PointLight[I], FragPosWS, Normal);
	}

	vec3 ColorFinal = (Ambient + Diffuse) * Color;

	FragColor = vec4(ColorFinal, MeshColor.w);
}