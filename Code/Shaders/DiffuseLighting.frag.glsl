#version 450

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"

layout (location = 0) out vec3 FragAmbient;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 CameraVector;

layout (push_constant) uniform PushConstants
{
	uint FrameNumber;
	uint LightCount;
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
    vec4 FrustumCorners[6];
};

layout (set = 0, binding = 1) readonly buffer Voxels
{
	uint VoxelColorMatActive[LevelDimZ][LevelDimY][LevelDimX];
};

layout (set = 0, binding = 2) uniform sampler2D BlueNoiseTexture;

layout (set = 0, binding = 3) uniform LightBuffer
{
	vec4 AmbientColor; // w - unused
	vec4 AmbientConstant; // w - unused
};

layout (set = 0, binding = 5) uniform sampler2D NormalsTexture;
layout (set = 0, binding = 6) uniform sampler2D LinearDepthTexture;

#include "Shading.incl.glsl"

layout (set = 0, binding = 4) readonly buffer PointsLights
{
	SLight Light[];
};

void main()
{
	vec3 Normal = texture(NormalsTexture, TexCoords).xyz;

    if (dot(Normal, Normal) == 0.0)
	{
		FragAmbient = vec3(0.5);
		return;
	}

	Normal = normalize(Normal);

	float LinearDepth = texture(LinearDepthTexture, TexCoords).r;
	vec3 FragPosWS = CameraPosition.xyz + LinearDepth * normalize(CameraVector) * Viewport.w;

	vec3 Ambient = CalculateAmbient(FragPosWS, Normal);
	vec3 LightsColor = vec3(0.0, 0.0, 0.0);
	for (uint I = 0; I < LightCount; I++)
	{
		LightsColor += CalculateLight(Light[I], FragPosWS, Normal);
	}

	FragAmbient = Ambient + LightsColor;
}