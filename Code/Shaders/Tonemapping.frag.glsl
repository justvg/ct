#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D InputTexture;
layout (set = 0, binding = 1) uniform sampler2D ExposureTexture;
layout (set = 0, binding = 2) uniform sampler2D BloomTexture;

layout (push_constant) uniform PushConstants
{
    uint MenuOpened;
    uint VignetteEnabled;
};

float GetVignetteMask()
{
	float DistanceFromCenter = length(TexCoords - vec2(0.5, 0.5));
   
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
    float Exposure = texture(ExposureTexture, vec2(0.5, 0.5)).r;
    Color *= 1.0;//Exposure;

    // Tone mapping
    vec3 ColorFinal = vec3(1.0) - exp(-Color);

	// Vignette effect
	if ((VignetteEnabled > 0) && (MenuOpened == 0))
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
		ColorFinal = mix(vec3(0.0), ColorFinal, 0.2);
	}

    FragColor = vec4(ColorFinal, 1.0);
}