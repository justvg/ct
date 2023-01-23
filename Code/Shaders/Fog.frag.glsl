#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 CameraVector;

layout (push_constant) uniform PushConstants
{
	vec3 FogInscatteringColor;
	float FogDensity;
	float FogHeightFalloff;
	float MinFogOpacity;
	float FogHeight;
	float FogCutoffDistance;
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

layout (set = 0, binding = 1) uniform sampler2D LinearDepthTexture;

const float FLT_EPSILON = 0.001f;
const float FLT_EPSILON2 = 0.01f;

float Pow2(float Value)
{
	return Value * Value;
}

// Calculate the line integral of the ray from the camera to the receiver position through the fog density function
// The exponential fog density function is d = GlobalDensity * exp(-HeightFalloff * y)
float CalculateLineIntegralShared(float FogHeightFalloff, float RayDirectionY, float RayOriginTerms)
{
	float Falloff = max(-127.0f, FogHeightFalloff * RayDirectionY);    // if it's lower than -127.0, then exp2() goes crazy in OpenGL's GLSL.
	float LineIntegral = ( 1.0f - exp2(-Falloff) ) / Falloff;
	float LineIntegralTaylor = log(2.0) - ( 0.5 * Pow2( log(2.0) ) ) * Falloff;		// Taylor expansion around 0
	
	return RayOriginTerms * ( abs(Falloff) > FLT_EPSILON2 ? LineIntegral : LineIntegralTaylor );
}

void main()
{
    vec3 ToPix = normalize(CameraVector);
	float LinearDepth = texture(LinearDepthTexture, TexCoords).r;
	vec3 WorldPositionRelativeToCamera = LinearDepth * ToPix * Viewport.w;

	// min and max exponent values for IEEE floating points (http://en.wikipedia.org/wiki/IEEE_floating_point)
	const float CollapsedFogParameterPower = clamp(-FogHeightFalloff * (CameraPosition.y - FogHeight), -126.0 + 1.0, +127.0 - 1.0);
	const float RayLength = length(WorldPositionRelativeToCamera);
	const float RayOriginTerms = FogDensity * pow(2.0f, CollapsedFogParameterPower);
	const float RayDirectionY = WorldPositionRelativeToCamera.y;

	// Calculate the "shared" line integral (this term is also used for the directional light inscattering) by adding the two line integrals together (from two different height falloffs and densities)
	float ExponentialHeightLineIntegralShared = CalculateLineIntegralShared(FogHeightFalloff, RayDirectionY, RayOriginTerms);
	float ExponentialHeightLineIntegral = ExponentialHeightLineIntegralShared * RayLength;

	vec3 InscatteringColor = FogInscatteringColor;

	// Calculate the amount of light that made it through the fog using the transmission equation
	float ExpFogFactor = clamp(exp2(-ExponentialHeightLineIntegral), 0.0, 1.0);

	if (FogCutoffDistance > 0 && RayLength > FogCutoffDistance)
	{
		ExpFogFactor = 0;
	}

	float Absorption = max(1.0 - ExpFogFactor, MinFogOpacity);

	FragColor = vec4(InscatteringColor, Absorption);
}