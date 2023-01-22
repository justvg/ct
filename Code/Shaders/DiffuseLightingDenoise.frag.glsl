#version 450

#extension GL_GOOGLE_include_directive: require

layout (location = 0) out vec3 FragAmbient;
layout (location = 1) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 CameraVector;

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

layout (set = 0, binding = 1) uniform sampler2D BlueNoiseTexture;
layout (set = 0, binding = 2) uniform sampler2D CurrentTexture;
layout (set = 0, binding = 3) uniform sampler2D HistoryTexture;
layout (set = 0, binding = 4) uniform sampler2D VelocityTexture;
layout (set = 0, binding = 5) uniform sampler2D HistoryVelocityTexture;
layout (set = 0, binding = 6) uniform sampler2D AlbedoTexture;

layout (push_constant) uniform PushConstants
{
    uint FrameNumber;
	uint LightCount;
};

#include "BlueNoise.incl.glsl"

void main()
{
	const vec2 TextureSize = textureSize(CurrentTexture, 0).xy;
    const vec2 TexelSize = 1.0 / TextureSize;

	const vec2 Velocity = texture(VelocityTexture, TexCoords).rg;

    const vec2 CurrentTexCoords = TexCoords;
    const vec2 HistoryTexCoords = TexCoords - Velocity;

	if ((HistoryTexCoords.x >= 0.0) && (HistoryTexCoords.x <= 1.0) && (HistoryTexCoords.y >= 0.0) && (HistoryTexCoords.y <= 1.0))
	{
		vec3 Current = texture(CurrentTexture, CurrentTexCoords).rgb;
		vec3 CurrentLT = texture(CurrentTexture, CurrentTexCoords + vec2(-TexelSize.x, TexelSize.y)).rgb;
		vec3 CurrentRT = texture(CurrentTexture, CurrentTexCoords + vec2(TexelSize.x, TexelSize.y)).rgb;
		vec3 CurrentLB = texture(CurrentTexture, CurrentTexCoords + vec2(-TexelSize.x, -TexelSize.y)).rgb;
		vec3 CurrentRB = texture(CurrentTexture, CurrentTexCoords + vec2(TexelSize.x, -TexelSize.y)).rgb;

		vec3 CurrentColorBlurred = (Current + CurrentLT + CurrentRT + CurrentLB + CurrentRB) * 0.2;
		
		vec3 CurrentMin = min(Current, min(CurrentLT, min(CurrentRT, min(CurrentLB, CurrentRB))));
		vec3 CurrentMax = max(Current, max(CurrentLT, max(CurrentRT, max(CurrentLB, CurrentRB))));

		vec3 History = texture(HistoryTexture, HistoryTexCoords).rgb;
		vec3 HistoryClamped = clamp(History, CurrentMin, CurrentMax);

		vec3 AccumulatedColor = mix(HistoryClamped, Current, 0.1);

		const vec2 HistoryVelocity = texture(HistoryVelocityTexture, HistoryTexCoords).rg;
		float VelocityDifferenceLength = clamp(length((HistoryVelocity - Velocity) / TexelSize) * 0.1, 0.0, 1.0);
		float VelocityDisocclusion = clamp((VelocityDifferenceLength - 0.001), 0.0, 1.0);

		FragAmbient = mix(AccumulatedColor, CurrentColorBlurred, VelocityDisocclusion);
	}
	else
	{
		FragAmbient = texture(CurrentTexture, CurrentTexCoords).rgb;
	}

	FragColor = vec4(FragAmbient * texture(AlbedoTexture, TexCoords).rgb, 1.0);
	FragColor.rgb = clamp(FragColor.rgb, 0.0, 3.0);
}