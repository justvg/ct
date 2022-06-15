#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D CurrentTexture;
layout (set = 0, binding = 1) uniform sampler2D HistoryTexture;
layout (set = 0, binding = 2) uniform sampler2D VelocityTexture;

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

void main()
{
	const vec2 Velocity = texture(VelocityTexture, TexCoords).rg;

    const vec2 TexelSize = 1.0 / textureSize(CurrentTexture, 0).xy;
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
}