#version 450

#extension GL_GOOGLE_include_directive: require

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D InputTexture;
layout (set = 0, binding = 1) uniform sampler2D ExposureTexture;
layout (set = 0, binding = 2) uniform sampler2D BloomTexture;
layout (set = 0, binding = 3) uniform sampler2D BlueNoiseTexture;

layout (push_constant) uniform PushConstants
{
    uint MenuOpened;
	float MenuOpenedBlend;
    uint VignetteEnabled;
	uint FrameNumber;
};

#include "BlueNoise.incl.glsl"

float GetVignetteMask()
{
	vec2 TC = TexCoords + 0.1 * (BlueNoiseVec2() - vec2(0.5));
	float DistanceFromCenter = length(TC - vec2(0.5, 0.5));
   
	float X = DistanceFromCenter * 2.0 - 0.55;
	X = clamp(X * 1.219512 , 0.0, 1.0);

	float X2 = X * X;
	float X3 = X2 * X;
	float X4 = X2 * X2;

	float VignetteMask = dot(vec4(X4, X3, X2, X), vec4(-0.10, -0.105, 1.12, 0.09));
	VignetteMask = min(VignetteMask, 0.94);  

	return VignetteMask; 
}

const float Gamma = 2.2;

void main()
{
    vec3 Color = texture(InputTexture, TexCoords).rgb + texture(BloomTexture, TexCoords).rgb;
    
    // Exposure
    // float Exposure = texture(ExposureTexture, vec2(0.5, 0.5)).r;
	float Exposure = 2.0f;
    Color *= Exposure;

    // Tone mapping
    vec3 ColorFinal = vec3(1.0) - exp(-Color);

	// Vignette effect
	if (VignetteEnabled > 0)
	{
		vec3 GammaCorrectedColor = pow(ColorFinal, vec3(1.0 / Gamma));

		float VignetteWeight = clamp(1.0 - dot(GammaCorrectedColor, vec3(1.0)), 0.0, 1.0);
		float VignetteMask = GetVignetteMask();
		float Vignette = VignetteWeight * VignetteMask;

		ColorFinal = mix(ColorFinal, vec3(0.0), Vignette);
	}

    // Gamma correction
    ColorFinal = pow(ColorFinal, vec3(1.0 / Gamma));

	if (MenuOpened > 0)
	{
		float BlendFactor = mix(1.0, 0.2, MenuOpenedBlend);
		ColorFinal = mix(vec3(0.0), ColorFinal, BlendFactor);
	}

    FragColor = vec4(ColorFinal, 1.0);
}