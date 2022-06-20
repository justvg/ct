#version 450

#extension GL_GOOGLE_include_directive: require
#include "VoxelDimension.h"

layout (location = 0) out vec4 FragColor;
layout (location = 1) out float FragLinearDepth;
layout (location = 2) out vec2 FragVelocity;

layout (location = 0) in vec3 NormalWS;
layout (location = 1) in vec3 FragPosWS;
layout (location = 2) in vec3 VoxelPosWS;

layout (push_constant) uniform PushConstants
{
    uint FrameNumber;
	uint PointLightCount;
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

layout (set = 0, binding = 2) readonly buffer Voxels
{
	uint VoxelActive[LevelDimZ][LevelDimY][LevelDimX];
};

layout (set = 0, binding = 3) uniform sampler2D BlueNoiseTexture;

layout (set = 0, binding = 4) uniform LightBuffer
{
	vec4 AmbientColor; // w - unused
	vec4 AmbientConstant; // w - unused
};

#include "ForwardShading.incl.glsl"

layout (set = 0, binding = 5) readonly buffer PointsLights
{
	SPointLight PointLight[];
};

void main()
{
	vec3 Normal = normalize(NormalWS);

	vec3 Ambient = CalculateAmbient(FragPosWS, Normal);

	// TODO(georgii): Add specular component?
	vec3 Diffuse = vec3(0.0, 0.0, 0.0);
	for (uint I = 0; I < PointLightCount; I++)
	{
		Diffuse += CalculatePointLight(PointLight[I], FragPosWS, Normal);
	}

    int VoxelX = int(floor(VoxelPosWS.x / VoxelDim));
	int VoxelY = int(floor(VoxelPosWS.y / VoxelDim));
	int VoxelZ = int(floor(VoxelPosWS.z / VoxelDim));
	
    float VoxelColorR = ((VoxelActive[VoxelZ][VoxelY][VoxelX] >> 24) & 0xFF) / 255.0;
	float VoxelColorG = ((VoxelActive[VoxelZ][VoxelY][VoxelX] >> 16) & 0xFF) / 255.0;
	float VoxelColorB = ((VoxelActive[VoxelZ][VoxelY][VoxelX] >> 8) & 0xFF) / 255.0;
 
    float LineThickness = 0.025 * VoxelDim;
    vec3 InBlockPos = abs(mod(FragPosWS, VoxelDim) - VoxelDim*0.5);
    float LineFactor0 = 1.0 - smoothstep(VoxelDim - LineThickness, VoxelDim, (2 * InBlockPos.y / VoxelDim) * max(InBlockPos.x, InBlockPos.z) + 0.5*VoxelDim);
    float LineFactor1 = 1.0 - smoothstep(VoxelDim - LineThickness, VoxelDim, (2 * InBlockPos.z / VoxelDim) * max(InBlockPos.x, InBlockPos.y) + 0.5*VoxelDim);

    vec3 VoxelColor = max(LineFactor0 * LineFactor1, 0.5) * vec3(VoxelColorR, VoxelColorG, VoxelColorB);
	vec3 ColorFinal = (Ambient + Diffuse) * VoxelColor;

	// TODO(georgii): I think I can do this in vertex shader. I remember that in Crysis we did it in VS
	vec4 CurrentTexCoords = ProjUnjittered * View * vec4(FragPosWS, 1.0);
	CurrentTexCoords.xy /= CurrentTexCoords.w;
	CurrentTexCoords.xy = CurrentTexCoords.xy * vec2(0.5, -0.5) + vec2(0.5, 0.5);

	vec4 PrevTexCoords = PrevProj * PrevView * vec4(FragPosWS, 1.0);
	PrevTexCoords.xy /= PrevTexCoords.w;
	PrevTexCoords.xy = PrevTexCoords.xy * vec2(0.5, -0.5) + vec2(0.5, 0.5);
	
	vec2 Velocity = CurrentTexCoords.xy - PrevTexCoords.xy;

	FragColor = vec4(ColorFinal, 1.0);
    FragLinearDepth = length(FragPosWS - CameraPosition.xyz) / Viewport.w;
	FragVelocity = Velocity;
}