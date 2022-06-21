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
	vec4 Offset; // w contains point lights count

	vec4 PrevPosition; // w - unused
	vec4 PrevOrientation; // w - unused

    uint FrameNumber;
	uint FPWeaponDepthTest;

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

const float Pi = 3.14159265358979323846;
vec2 PolarCoordinates(vec2 UV, vec2 Center, float RadialScale, float LengthScale)
{
    vec2 Delta = UV - Center;
    float Radius = length(Delta) * 2 * RadialScale;
    float Angle = abs(atan(Delta.y, Delta.x)) * LengthScale * (1.0 / (2 * Pi));

    return vec2(Radius, Angle);
}

void main()
{
	vec2 UV = vec2(LocalPos.x, -LocalPos.y) + vec2(0.5);

	float NoiseBasedOnUV = 0.1 * (2.0 * GradientNoiseFloat(UV + vec2(Time), 3.0) - 1.0);

	vec2 PolarCoords = PolarCoordinates(UV, vec2(0.5), 1.0, 1.0);
	float Radius = PolarCoords.x;
	float Angle = PolarCoords.y;

	vec2 SampleUV = vec2(Radius + Time, Angle + Radius) + vec2(NoiseBasedOnUV);
	float ColorNoise = pow(SimpleNoise(SampleUV, 2), 3.0);

	float RadiusNoise = 0.2 * SimpleNoise(UV + 0.5 * vec2(Time), 10.0);
	float NewRadius = Radius + RadiusNoise;
	float Alpha = step(NewRadius, 1.0) * (1.0 - pow(smoothstep(0.2, max(0.2, NewRadius), Radius), 64));

	vec3 BrightPointUV = 10 * normalize(FragPosWS) + vec3(0.05 * Time);
	float BrightPoint = 2 * smoothstep(0.7, 0.9, SimpleNoise(BrightPointUV.xy, 1000));

	vec3 Color = MeshColor.rgb * (ColorNoise + pow(NewRadius, 8.0)) + BrightPoint;

	FragColor = vec4(Color, Alpha);
}