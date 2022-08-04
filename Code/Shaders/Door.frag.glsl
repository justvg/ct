#version 450

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"
#include "Noise.incl.glsl"

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 NormalWS;
layout (location = 1) in vec3 FragPosWS;
layout (location = 2) in vec3 FragPrevPosWS;
layout (location = 3) in vec3 LocalPos;

layout (push_constant) uniform PushConstants
{
	vec4 Position; // w - unused
	vec4 Scale; // w - unused
	vec4 Orientation;
	vec4 MeshColor;
	vec4 Offset; // w - unused

	vec4 PrevPosition; // w - unused
	vec4 PrevOrientation; // w - unused

	uint FirstPersonDepthTest;

	float Time;
	float ShaderValue0; // NOTE(georgii): Currently used for material parameters. Like MaxComponentNoise in door shader
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

void main()
{
	vec2 UV = vec2(LocalPos.x, -LocalPos.y) + vec2(0.5);

	float RadiusNoise = 0.3 * SimpleNoise(UV + 0.15 * vec2(Time), 10.0);
	float Radius = length(LocalPos.xy);
	float NewRadius = Radius + RadiusNoise;
	float RadiusFactor = (1.0 - pow(smoothstep(0.0, NewRadius, Radius), 16));

	float MaxComponentNoise = ShaderValue0 * SimpleNoise(UV + ShaderValue0 * 0.5 * vec2(Time) + 10 * normalize(FragPosWS).xy, 20.0);
	float MaxComponent = 2.0 * max(abs(LocalPos.x), abs(LocalPos.y));
	float NewMaxComponent = MaxComponent + MaxComponentNoise;
	float MaxComponentFactor = step(NewMaxComponent, 1.0); // * (1.0 - smoothstep(0.6, max(0.6, NewMaxComponent), MaxComponent));

	float Alpha = MaxComponentFactor * RadiusFactor * MeshColor.a;

	vec3 Color = MeshColor.rgb;

	FragColor = vec4(Color, min(0.99999, Alpha));
}