#version 450

#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_control_flow_attributes: require
#include "VoxelDimension.h"

layout (location = 0) out vec4 FragSpecular;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 CameraVector;

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

#include "Shading.incl.glsl"

layout (set = 0, binding = 4) uniform sampler2D NormalsTexture;
layout (set = 0, binding = 5) uniform sampler2D LinearDepthTexture;
layout (set = 0, binding = 6) uniform sampler2D DiffuseTexture;
layout (set = 0, binding = 7) uniform sampler2D MaterialTexture;

layout (set = 0, binding = 8) readonly buffer PointsLights
{
	SPointLight PointLight[];
};

void main()
{
	vec3 Normal = texture(NormalsTexture, TexCoords).xyz;

	[[branch]]
    if (dot(Normal, Normal) == 0.0)
	{
		FragSpecular = vec4(0.0);
		return;
	}

	vec3 ToPix = normalize(CameraVector);
	float LinearDepth = texture(LinearDepthTexture, TexCoords).r;
	vec3 FragPosWS = CameraPosition.xyz + LinearDepth * ToPix * Viewport.w;

	vec3 ReflDir = normalize(ToPix - Normal*(2.0 * dot(Normal, ToPix)));

	vec2 Material = texture(MaterialTexture, TexCoords).rg;
	float Roughness = Material.g;
	float Deviation = 0.1 * Roughness;

	float t = abs(dot(ToPix, Normal));
	t = exp2(-8.656170 * t); //  1/ln(2) * 6 from  https://seblagarde.wordpress.com/2011/08/17/hello-world/
	float RoughFresnel = (0.1 + 0.9 * (1.0 - Roughness)); // Rough surfaces should have less fresnel effect

	float Reflectivity = t * Material.r * RoughFresnel;
	Reflectivity -= 0.01;

	[[branch]]
	if (Reflectivity <= 0.0)
	{
		FragSpecular = vec4(0.0);
		return;
	}

	vec3 RandomDir = 2.0 * BlueNoiseVec3() - vec3(1.0);
	vec3 Dir = normalize(ReflDir + RandomDir * Deviation);

	const float MaxDistance = 20.0;
	const vec3 RayStartPos = FragPosWS + Normal * 0.5 * VoxelDim;

	vec3 HitNormal;
	int VoxelX, VoxelY, VoxelZ;
	float HitDistance = RaytraceDirectionalVox(RayStartPos, Dir, MaxDistance, VoxelX, VoxelY, VoxelZ, HitNormal);

	vec3 Environment = AmbientColor.rgb;
	if (HitDistance < MaxDistance)
	{
		vec3 HitPos = RayStartPos + HitDistance * Dir;
		vec4 HitPosCS = Proj * View * vec4(HitPos, 1.0);
		vec2 HitPosTexCoords =  vec2(0.5, -0.5) * (HitPosCS.xy / HitPosCS.w) + vec2(0.5);
		float HitPosLinearDepth = length(HitPos - CameraPosition.xyz) / Viewport.w;

		vec2 AbsNDC = abs(clamp(HitPosCS.xy / HitPosCS.w, vec2(-1.0), vec2(1.0)));
		float ScreenPosFactor = (1.0 - AbsNDC.x) * (1.0 - AbsNDC.y);

		float DepthDiff = abs(texture(LinearDepthTexture, HitPosTexCoords).r - HitPosLinearDepth);
		float Thickness = 2.0 * VoxelDim / Viewport.w;
		float DepthFactor = smoothstep(Thickness, 0.0, DepthDiff);

		vec4 SampledDiffuse = texture(DiffuseTexture, HitPosTexCoords);

		bool bTransparent = SampledDiffuse.a < 1.0;
		float BlendFactor = ScreenPosFactor * DepthFactor;

		[[branch]]
		if ((BlendFactor < 0.01) || bTransparent)
		{
			float VoxelColorR = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] >> 24) & 0xFF) / 255.0;
			float VoxelColorG = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] >> 16) & 0xFF) / 255.0;
			float VoxelColorB = ((VoxelColorMatActive[VoxelZ][VoxelY][VoxelX] >> 8) & 0xFF) / 255.0;

			vec3 Ambient = CalculateAmbient(HitPos, HitNormal);
			vec3 PointLightsColor = vec3(0.0, 0.0, 0.0);
			for (uint I = 0; I < PointLightCount; I++)
			{
				PointLightsColor += CalculatePointLight(PointLight[I], HitPos, HitNormal);
			}

			FragSpecular.rgb = vec3(VoxelColorR, VoxelColorG, VoxelColorB) * (Ambient + PointLightsColor);
			FragSpecular.rgb = mix(FragSpecular.rgb, SampledDiffuse.rgb, uint(bTransparent) * ScreenPosFactor);
		}
		else
		{
			FragSpecular.rgb = SampledDiffuse.rgb;
		}

		float EnvironmentT = pow(HitDistance / MaxDistance, 3);
		FragSpecular.rgb = mix(FragSpecular.rgb, Environment, EnvironmentT);
		FragSpecular.a = Reflectivity;
	}
	else
	{
		FragSpecular = vec4(Environment, Reflectivity);
	}
}