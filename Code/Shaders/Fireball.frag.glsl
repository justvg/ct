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
	float ShaderValue0; // NOTE(georgii): Currently used for material parameters. Like LengthNoise in door shader
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

float FresnelEffect(vec3 Normal, vec3 ViewDir, float Power)
{
    float Fresnel = pow(1.0 - max(dot(Normal, ViewDir), 0.0), Power);
	
	return Fresnel;
}

void main()
{
	const vec3 Normal = normalize(NormalWS);
	const vec3 ViewDir = normalize(CameraPosition.xyz - FragPosWS);

	// vec2 UV = vec2(LocalPos.x, -LocalPos.y) + vec2(0.5);
	// float Length = length(2.0 * vec2(max(abs(LocalPos.x), abs(LocalPos.z)), LocalPos.y));
	// float LengthNoise = 0.1 * GradientNoiseFloat(UV + 0.5 * vec2(Time) + 10 * normalize(FragPosWS).xy, 50.0);
	// float NewLength = Length + LengthNoise;
	// float LengthFactor = step(NewLength, 1.0);

	const float VerticalScaling = 70.0;
	float Y = FragPosWS.y + 0.3 * Time + 0.05 * GradientNoiseFloat(FragPosWS.xy + vec2(Time), 50.0);

	float Sine = clamp(sin(VerticalScaling * Y), 0.0, 1.0);
	float Fract = pow(fract(Y + 0.5 * Time), 2);

	float Fresnel = FresnelEffect(Normal, ViewDir, 1.5);

	vec3 Color = MeshColor.rgb;// * (Sine + Fract + Fresnel);

	float Alpha = Sine + Fract + Fresnel;
	FragColor = vec4(Color, min(0.99999, Alpha));
}