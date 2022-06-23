#version 450

#define OLD_TAA 0
#define SAMPLE_BICUBIC_HISTORY 0
#define MAGNITUDE_DILATION_VELOCITY 1

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D CurrentTexture;
layout (set = 0, binding = 1) uniform sampler2D HistoryTexture;
layout (set = 0, binding = 2) uniform sampler2D VelocityTexture;
layout (set = 0, binding = 3) uniform sampler2D HistoryVelocityTexture;

vec4 ClipHistory(vec4 History, vec4 Current, vec4 CurrentMin, vec4 CurrentMax)
{
	vec4 Result = History;

	float Length = length(Current.xyz - History.xyz);
	if (Length > 1e-6)
	{
		vec3 RayDir = (Current.xyz - History.xyz) / Length;
		vec3 RayDirInv = 1.0 / RayDir;

		vec3 tMax = (CurrentMax.xyz - History.xyz) * RayDirInv;
		vec3 tMin = (CurrentMin.xyz - History.xyz) * RayDirInv;

		float t = max(max(min(tMin.x, tMax.x), min(tMin.y, tMax.y)), min(tMin.z, tMax.z));
		t = clamp(t, 0.0, 1.0);

		Result.xyz += RayDir * t;
	}

	return Result;
}

#define BICUBIC_CATMULL_ROM_SAMPLES 5
struct SCatmullRomSamples
{
	// Constant number of samples (BICUBIC_CATMULL_ROM_SAMPLES)
	uint Count;

	// Bilinear sampling UV coordinates of the samples
	vec2 UV[BICUBIC_CATMULL_ROM_SAMPLES];

	// Weights of the samples
	float Weight[BICUBIC_CATMULL_ROM_SAMPLES];

	// Final multiplier (it is faster to multiply 3 RGB values than reweights the 5 weights)
	float FinalMultiplier;
};

void Bicubic2DCatmullRom(in vec2 UV, in vec2 Size, in vec2 InvSize, out vec2 Sample[3], out vec2 Weight[3])
{
	UV *= Size;

	vec2 tc = floor(UV - 0.5) + 0.5;
	vec2 f = UV - tc;
	vec2 f2 = f * f;
	vec2 f3 = f2 * f;

	vec2 w0 = f2 - 0.5 * (f3 + f);
	vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1;
	vec2 w3 = 0.5 * (f3 - f2);
	vec2 w2 = 1 - w0 - w1 - w3;

	Weight[0] = w0;
	Weight[1] = w1 + w2;
	Weight[2] = w3;

	Sample[0] = tc - 1;
	Sample[1] = tc + w2 / Weight[1];
	Sample[2] = tc + 2;

	Sample[0] *= InvSize;
	Sample[1] *= InvSize;
	Sample[2] *= InvSize;
}

SCatmullRomSamples GetBicubic2DCatmullRomSamples(vec2 UV, vec2 Size, in vec2 InvSize)
{
	SCatmullRomSamples Samples;
	Samples.Count = BICUBIC_CATMULL_ROM_SAMPLES;

	vec2 Weight[3];
	vec2 Sample[3];
	Bicubic2DCatmullRom(UV, Size, InvSize, Sample, Weight);

	// Optimized by removing corner samples
	Samples.UV[0] = vec2(Sample[1].x, Sample[0].y);
	Samples.UV[1] = vec2(Sample[0].x, Sample[1].y);
	Samples.UV[2] = vec2(Sample[1].x, Sample[1].y);
	Samples.UV[3] = vec2(Sample[2].x, Sample[1].y);
	Samples.UV[4] = vec2(Sample[1].x, Sample[2].y);

	Samples.Weight[0] = Weight[1].x * Weight[0].y;
	Samples.Weight[1] = Weight[0].x * Weight[1].y;
	Samples.Weight[2] = Weight[1].x * Weight[1].y;
	Samples.Weight[3] = Weight[2].x * Weight[1].y;
	Samples.Weight[4] = Weight[1].x * Weight[2].y;

	// Reweight after removing the corners
	float CornerWeights;
	CornerWeights = Samples.Weight[0];
	CornerWeights += Samples.Weight[1];
	CornerWeights += Samples.Weight[2];
	CornerWeights += Samples.Weight[3];
	CornerWeights += Samples.Weight[4];
	Samples.FinalMultiplier = 1 / CornerWeights;

	return Samples;
}

vec3 SampleBicubicTexture(sampler2D Tex, vec2 UV, vec2 Size, in vec2 InvSize)
{
	SCatmullRomSamples Samples = GetBicubic2DCatmullRomSamples(UV, Size, InvSize);

	vec3 OutColor = vec3(0.0);
	for (uint I = 0; I < Samples.Count; I++)
	{
		OutColor += texture(Tex, Samples.UV[I]).rgb * Samples.Weight[I];
	}
	OutColor *= Samples.FinalMultiplier;

	return OutColor;
}

void main()
{
#if !OLD_TAA

	const vec2 TextureSize = textureSize(CurrentTexture, 0).xy;
    const vec2 TexelSize = 1.0 / TextureSize;

#if MAGNITUDE_DILATION_VELOCITY
	float MaxVelocityLength = 0.0;
	vec2 Velocity = vec2(0.0);
	for (int J = -1; J <= 1; J++)
	{
		for (int I = -1; I <= 1; I++)
		{
			const vec2 TestVelocity = texture(VelocityTexture, TexCoords + vec2(I, J) * TexelSize).rg;
			const float TestVelocityLength = length(TestVelocity);

			if (TestVelocityLength > MaxVelocityLength)
			{
				MaxVelocityLength = TestVelocityLength;
				Velocity = TestVelocity;
			}
		}	
	}
#else
	const vec2 Velocity = texture(VelocityTexture, TexCoords).rg;
#endif

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

#if SAMPLE_BICUBIC_HISTORY
		vec3 History = SampleBicubicTexture(HistoryTexture, HistoryTexCoords, TextureSize, 1.0 / TextureSize);
#else
		vec3 History = texture(HistoryTexture, HistoryTexCoords).rgb;
#endif
		vec3 HistoryClamped = clamp(History, CurrentMin, CurrentMax);

		vec3 AccumulatedColor = mix(HistoryClamped, Current, 0.1);

		const vec2 HistoryVelocity = texture(HistoryVelocityTexture, HistoryTexCoords).rg;
		float VelocityDifferenceLength = clamp(length((HistoryVelocity - Velocity) / TexelSize) * 0.15, 0.0, 1.0);
		float VelocityDisocclusion = clamp((VelocityDifferenceLength - 0.001), 0.0, 1.0);

		FragColor = vec4(mix(AccumulatedColor, Current, VelocityDisocclusion), 1.0);
	}
	else
	{
		FragColor = vec4(texture(CurrentTexture, CurrentTexCoords).rgb, 1.0);
	}

#else

	const vec2 TextureSize = textureSize(CurrentTexture, 0).xy;
    const vec2 TexelSize = 1.0 / TextureSize;

	const vec2 Velocity = texture(VelocityTexture, TexCoords).rg;
    const vec2 CurrentTexCoords = TexCoords;
    const vec2 HistoryTexCoords = TexCoords - Velocity;

	if ((HistoryTexCoords.x >= 0.0) && (HistoryTexCoords.x <= 1.0) && (HistoryTexCoords.y >= 0.0) && (HistoryTexCoords.y <= 1.0))
	{
		vec4 Current = texture(CurrentTexture, CurrentTexCoords);
		vec4 CurrentLT = texture(CurrentTexture, CurrentTexCoords + vec2(-TexelSize.x, TexelSize.y));
		vec4 CurrentRT = texture(CurrentTexture, CurrentTexCoords + vec2(TexelSize.x, TexelSize.y));
		vec4 CurrentLB = texture(CurrentTexture, CurrentTexCoords + vec2(-TexelSize.x, -TexelSize.y));
		vec4 CurrentRB = texture(CurrentTexture, CurrentTexCoords + vec2(TexelSize.x, -TexelSize.y));
		
		vec4 CurrentMin = min(Current, min(CurrentLT, min(CurrentRT, min(CurrentLB, CurrentRB))));
		vec4 CurrentMax = max(Current, max(CurrentLT, max(CurrentRT, max(CurrentLB, CurrentRB))));

		vec4 History = texture(HistoryTexture, HistoryTexCoords);
		vec4 HistoryLT = texture(HistoryTexture, HistoryTexCoords + vec2(-TexelSize.x, TexelSize.y));
		vec4 HistoryRT = texture(HistoryTexture, HistoryTexCoords + vec2(TexelSize.x, TexelSize.y));
		vec4 HistoryLB = texture(HistoryTexture, HistoryTexCoords + vec2(-TexelSize.x, -TexelSize.y));
		vec4 HistoryRB = texture(HistoryTexture, HistoryTexCoords + vec2(TexelSize.x, -TexelSize.y));

		vec4 HistoryMin = min(History, min(HistoryLT, min(HistoryRT, min(HistoryLB, HistoryRB))));
		vec4 HistoryMax = max(History, max(HistoryLT, max(HistoryRT, max(HistoryLB, HistoryRB))));

		float CurrentMaxSum = (CurrentMax.r + CurrentMax.g + CurrentMax.b);
		float CurrentMinSum = (CurrentMin.r + CurrentMin.g + CurrentMin.b);
		float HistoryMaxSum = (HistoryMax.r + HistoryMax.g + HistoryMax.b);
		float HistoryMinSum = (HistoryMin.r + HistoryMin.g + HistoryMin.b);

		float VelocityLength = clamp(length(Velocity / TexelSize), 0.0, 1.0);
		float AccVelocityLength = texture(HistoryTexture, CurrentTexCoords).a;
		AccVelocityLength = clamp(max(AccVelocityLength - 0.1, VelocityLength), 0.0, 1.0);

		float ClampFactor = AccVelocityLength;
		ClampFactor = max(ClampFactor, CurrentMinSum / (HistoryMaxSum + 0.001) - 0.5);
		ClampFactor = max(ClampFactor, HistoryMinSum / (CurrentMaxSum + 0.001) - 0.5);
		ClampFactor = clamp(ClampFactor, 0.0, 1.0);

		History = mix(History, clamp(History, CurrentMin, CurrentMax), ClampFactor);
		History = clamp(History, vec4(0.0), vec4(10.0));

		FragColor = mix(History, Current, mix(0.1, 0.4, ClampFactor));
		FragColor.a = AccVelocityLength;
	}
	else
	{
		float VelocityLength = clamp(length(Velocity / TexelSize), 0.0, 1.0);
		FragColor = vec4(texture(CurrentTexture, CurrentTexCoords).rgb, VelocityLength);
	}

#endif
}