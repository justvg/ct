#version 460

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"

layout (location = 0) out vec3 NormalWS;
layout (location = 1) out vec3 FragPosWS;
layout (location = 2) out vec3 VoxelPosWS;

layout (location = 0) in vec3 LocalPosition;
layout (location = 1) in vec3 LocalNormal;

struct SVoxelDraw
{
	vec3 Position;
	uint FirstInstance;
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

layout (set = 0, binding = 1) readonly buffer Draws
{
	SVoxelDraw Draw[];
};

void main()
{
	vec3 P = Draw[gl_BaseInstance].Position;
	float S = VoxelDim;

	NormalWS = LocalNormal;
	FragPosWS = LocalPosition * S + P;
	VoxelPosWS = P + 0.5*vec3(S, S, S);

	gl_Position = Proj * View * vec4(FragPosWS, 1.0);
}