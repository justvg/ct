#include "BlueNoise.incl.glsl"

layout (constant_id = 0) const int AO_SAMPLES = 4;

uint IsVoxelActive(int X, int Y, int Z)
{
    return VoxelColorMatActive[Z][Y][X] & 1;
}

float RaytraceDirectionalVox(vec3 Pos, vec3 Dir, float MaxDistance)
{
    int X = int(floor(Pos.x / VoxelDim));
	int Y = int(floor(Pos.y / VoxelDim));
	int Z = int(floor(Pos.z / VoxelDim));
    vec3 XYZ = vec3(X, Y, Z);

    if ((X < 0) || (X > LevelDimX - 1) || (Y < 0) || (Y > LevelDimY - 1) || (Z < 0) || (Z > LevelDimZ - 1))
    {
        return MaxDistance;
    }

    if (IsVoxelActive(X, Y, Z) > 0)
    {
        return 0.0;
    }

    vec3 DirSign = sign(Dir);
    vec3 SignPositive = step(vec3(0.0), DirSign);
    vec3 InvDir = vec3(1.0) / (abs(Dir) + vec3(0.00001));
    if (Dir.x < 0.0) InvDir.x = -InvDir.x;
    if (Dir.y < 0.0) InvDir.y = -InvDir.y;
    if (Dir.z < 0.0) InvDir.z = -InvDir.z;

    float Distance = 0.0;
	while (Distance < MaxDistance)
	{
        vec3 CurrentPos = Pos + Distance*Dir;
        vec3 PlaneD = XYZ*VoxelDim + SignPositive*VoxelDim;
        vec3 Distances = (PlaneD - CurrentPos) * InvDir;

        vec3 Min = step(Distances.xyz, Distances.zxy) * step(Distances.xyz, Distances.yzx);
        XYZ += DirSign*Min;
        Distance += dot(Distances, Min);

        X = int(XYZ.x); Y = int(XYZ.y); Z = int(XYZ.z);
        if ((X < 0) || (X > LevelDimX - 1) || (Y < 0) || (Y > LevelDimY - 1) || (Z < 0) || (Z > LevelDimZ - 1))
        {
            Distance = MaxDistance;
            break;
        }

        if (IsVoxelActive(X, Y, Z) > 0)
            break;
	}

    Distance = min(Distance, MaxDistance);
    return Distance;
}

float RaytraceDirectionalVox(vec3 Pos, vec3 Dir, float MaxDistance, out int OutX, out int OutY, out int OutZ, out vec3 OutNormal)
{
    int X = int(floor(Pos.x / VoxelDim));
	int Y = int(floor(Pos.y / VoxelDim));
	int Z = int(floor(Pos.z / VoxelDim));
    vec3 XYZ = vec3(X, Y, Z);

    if ((X < 0) || (X > LevelDimX - 1) || (Y < 0) || (Y > LevelDimY - 1) || (Z < 0) || (Z > LevelDimZ - 1))
    {
        return MaxDistance;
    }

    vec3 DirSign = sign(Dir);
    vec3 SignPositive = step(vec3(0.0), DirSign);
    vec3 InvDir = vec3(1.0) / (abs(Dir) + vec3(0.00001));
    if (Dir.x < 0.0) InvDir.x = -InvDir.x;
    if (Dir.y < 0.0) InvDir.y = -InvDir.y;
    if (Dir.z < 0.0) InvDir.z = -InvDir.z;

    float Distance = 0.0;
	while (Distance < MaxDistance)
	{
        vec3 CurrentPos = Pos + Distance*Dir;
        vec3 PlaneD = XYZ*VoxelDim + SignPositive*VoxelDim;
        vec3 Distances = (PlaneD - CurrentPos) * InvDir;

        vec3 Min = step(Distances.xyz, Distances.zxy) * step(Distances.xyz, Distances.yzx);
        XYZ += DirSign*Min;
		OutNormal = -DirSign*Min;
        Distance += dot(Distances, Min);

        X = int(XYZ.x); Y = int(XYZ.y); Z = int(XYZ.z);
        if ((X < 0) || (X > LevelDimX - 1) || (Y < 0) || (Y > LevelDimY - 1) || (Z < 0) || (Z > LevelDimZ - 1))
        {
            Distance = MaxDistance;
            break;
        }

        if (IsVoxelActive(X, Y, Z) > 0)
            break;
	}

	OutX = X;
	OutY = Y;
	OutZ = Z;
    Distance = min(Distance, MaxDistance);
    return Distance;
}

float RaytraceDirectional(vec3 Pos, vec3 Dir, float MaxDistance, float Step)
{
    float Distance = 0.0;
	while (Distance < MaxDistance)
	{
        Distance += Step;

        vec3 TestPos = Pos + Distance*Dir;
        int X = int(floor(TestPos.x / VoxelDim));
	    int Y = int(floor(TestPos.y / VoxelDim));
	    int Z = int(floor(TestPos.z / VoxelDim));

        if ((X < 0) || (X > LevelDimX - 1) || (Y < 0) || (Y > LevelDimY - 1) || (Z < 0) || (Z > LevelDimZ - 1))
        {
            Distance = MaxDistance;
        }

        if (IsVoxelActive(X, Y, Z) > 0)
            break;
	}

    Distance = min(Distance, MaxDistance);
    return Distance;
}

vec3 SampleHemisphere(vec2 V)
{
	float Radius = sqrt(V.x);
	float Theta = 6.28 * V.y;
	float X = Radius * cos(Theta);
	float Y = Radius * sin(Theta);
	return normalize(vec3(X, Y, sqrt(max(0.0, 1.0 - V.x))));
}

bool BlockedInScreenspace(vec3 Pos, vec3 Dir)
{
    const float InvFar = 1.0f / Viewport.w;
	const float Thickness = VoxelDim * InvFar;

    vec3 NewPosWS = Pos + Dir * (VoxelDim * 0.35 * BlueNoiseFloat());
    for (int I = 0; I < 2; I++)
    {
        vec4 NewPosHS = Proj * View * vec4(NewPosWS, 1.0);
        vec2 NewTexCoords = vec2(0.5, -0.5) * (NewPosHS.xy / NewPosHS.w) + vec2(0.5);

        float NewDepth = length(NewPosWS - CameraPosition.xyz) * InvFar;    
        float ActualDepth = texture(LinearDepthTexture, NewTexCoords).r;

        if ((NewDepth > ActualDepth) && (NewDepth < ActualDepth + Thickness))
            return true;

        NewPosWS += Dir * (VoxelDim * 0.35 * BlueNoiseFloat());
    }

	return false;
}

float TraceAmbient(vec3 Pos, vec3 Dir, vec3 Normal, float MaxDistance)
{
    if (BlockedInScreenspace(Pos, Dir))
		return 0.0;

    vec3 Jitter = BlueNoiseVec3() - vec3(0.5);
    Jitter -= Normal * dot(Normal, Jitter);
    Jitter = normalize(Jitter) * VoxelDim * (0.5 * BlueNoiseFloat());
    Pos += Jitter;

    float HitDist = RaytraceDirectional(Pos, Dir, MaxDistance, 0.6 * VoxelDim);
    return HitDist;
}

vec3 CalculateAmbient(vec3 FragPosWS, vec3 Normal)
{
    vec3 Ambient = vec3(0.0, 0.0, 0.0);

    vec3 Tangent = abs(Normal.z) > 0.5 ? vec3(0.0, -Normal.z, Normal.y) : vec3(-Normal.y, Normal.x, 0.0);
    Tangent = normalize(Tangent - Normal*dot(Normal, Tangent));
    vec3 Bitangent = cross(Normal, Tangent);

	const float MaxDistance = 4.0;
    for(int I = 0; I < AO_SAMPLES; I++)
    {
        vec3 StartPos = FragPosWS;
        vec3 DirTS = SampleHemisphere(BlueNoiseVec2());
        vec3 Dir = Tangent*DirTS.x + Bitangent*DirTS.y + Normal*DirTS.z;

        float HitDist = TraceAmbient(StartPos, Dir, Normal, MaxDistance);
        float t = clamp(HitDist / MaxDistance, 0.0, 1.0);
        t = pow(t, 2.0);

        Ambient += AmbientColor.rgb * t;
        Ambient += AmbientConstant.rgb * max(1.0, HitDist / (2.0 * VoxelDim));
    }
    Ambient /= float(AO_SAMPLES);

    return Ambient;
}

float GetAttenuation(float Distance, float Range)
{
	float A = (Distance / Range);
	A = A * A * A * A;
	A = max(0.0, 1.0 - A);
	A = A * A;
	float Attenuation = A / (Distance * Distance + 0.1);

	Attenuation = max(Attenuation, 0.0);

	return Attenuation;
}

struct SLight
{
	vec3 Pos;
	float Radius;
	vec3 Color;
    uint LightType;

    // NOTE(georgii): These are for specular lights
	vec3 Direction;
	float Cutoff;
};

vec3 CalculateLight(SLight Light, vec3 FragPosWS, vec3 Normal)
{
    vec3 Result = vec3(0.0, 0.0, 0.0);

    const float PenumbraSize = 0.05;
    const float LightRange = (1.0 + PenumbraSize) * Light.Radius;
    const vec3 LightPos = Light.Pos + PenumbraSize * (2.0 * BlueNoiseVec3() - 1.0);
    const float DistanceToLight = length(LightPos - FragPosWS);
    if ((DistanceToLight >= 0.01) && (DistanceToLight <= LightRange))
    {
        vec3 LightVec = (LightPos - FragPosWS) / DistanceToLight;
        float Incoming = max(dot(LightVec, Normal), 0.0);

        // SpotLight
        [[branch]]
        if (Light.LightType == 1)
        {
            const vec3 SpotLightDir = normalize(Light.Direction);
            const float SpotCutoff = Light.Cutoff;

            const float SpotCutoffCos = cos((SpotCutoff / 180.0) * 3.14159265358979323846);
            const float AngleToFragCos = dot(-LightVec, SpotLightDir);
            if (AngleToFragCos <= SpotCutoffCos)
                return Result;

            Incoming *= (AngleToFragCos - SpotCutoffCos) / (1.0 - SpotCutoffCos);
        }

        if (Incoming > 0)
        {
            float Attenuation = GetAttenuation(DistanceToLight, LightRange);

            // Calculate jitter
			vec3 Jitter = BlueNoiseVec3();
			Jitter -= Normal * dot(Normal, Jitter);
			Jitter = normalize(Jitter) * VoxelDim * (0.5 * BlueNoiseFloat());

            // Apply offsets
			FragPosWS += 0.5 * Jitter;
			FragPosWS += LightVec * VoxelDim * 0.5;
			FragPosWS += Normal * VoxelDim * (0.6 * (1.0 - Incoming));

            float NewDistanceToLight = length(LightPos - FragPosWS);
            float HitDist = RaytraceDirectionalVox(FragPosWS, (LightPos - FragPosWS) / NewDistanceToLight, NewDistanceToLight);
            float Shadow = (HitDist == NewDistanceToLight) ? 1.0 : 0.0;

            Result = Shadow * Attenuation * Incoming * Light.Color.rgb;
        }
    }

    return Result;
}