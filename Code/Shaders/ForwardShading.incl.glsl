#include "BlueNoise.incl.glsl"

uint IsVoxelActive(int X, int Y, int Z)
{
    return VoxelActive[Z][Y][X] & 1;
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

vec3 CalculateAmbient(vec3 FragPosWS, vec3 Normal)
{
    vec3 Ambient = vec3(0.0, 0.0, 0.0);

    vec3 Tangent = abs(Normal.z) > 0.5 ? vec3(0.0, -Normal.z, Normal.y) : vec3(-Normal.y, Normal.x, 0.0);
    Tangent = normalize(Tangent - Normal*dot(Normal, Tangent));
    vec3 Bitangent = cross(Normal, Tangent);

    const int Samples = 4;
    for(int I = 0; I < Samples; I++)
    {
        const float MaxDistance = 2.5;
        vec3 StartPos = FragPosWS;
        vec3 DirTS = SampleHemisphere(BlueNoiseVec2());
        vec3 Dir = Tangent*DirTS.x + Bitangent*DirTS.y + Normal*DirTS.z;

        vec3 Jitter = BlueNoiseVec3() - vec3(0.5);
        Jitter -= Normal * dot(Normal, Jitter);
        Jitter = normalize(Jitter) * VoxelDim * (0.1*BlueNoiseFloat());
        StartPos += Jitter;
        
        float HitDist = RaytraceDirectional(StartPos, Dir, MaxDistance, 0.6 * VoxelDim);
        float t = clamp(HitDist / MaxDistance, 0.0, 1.0);
        t = pow(t, 2.0);

        Ambient += AmbientColor.rgb * t;
        Ambient += AmbientConstant.rgb * max(1.0, HitDist / (2.0 * VoxelDim));
    }
    Ambient /= float(Samples);

    return Ambient;
}

float GetAttenuation(float Distance, float Range)
{
	float Attenuation = 1.0 - smoothstep(0.1*Range, Range, Distance);

	return Attenuation;
}

struct SPointLight
{
	vec3 Pos;
	float Radius;
	vec4 Color;
};

vec3 CalculatePointLight(SPointLight PointLight, vec3 FragPosWS, vec3 Normal)
{
    vec3 Result = vec3(0.0, 0.0, 0.0);

    const float PenumbraSize = 0.05;
    const float LightRange = (1.0 + PenumbraSize) * PointLight.Radius;
    const vec3 LightPos = PointLight.Pos + PenumbraSize * (2.0 * BlueNoiseVec3() - 1.0);
    const float DistanceToLight = length(LightPos - FragPosWS);
    if (DistanceToLight <= LightRange)
    {
        vec3 LightVec = (LightPos - FragPosWS) / DistanceToLight;
        
        float Incoming = max(dot(LightVec, Normal), 0.0);
        if (Incoming > 0)
        {
            float Attenuation = GetAttenuation(DistanceToLight, LightRange);

            float HitDist = RaytraceDirectionalVox(FragPosWS, LightVec, DistanceToLight);
            float Shadow = (HitDist == DistanceToLight) ? 1.0 : 0.0;

            Result = Shadow * Attenuation * Incoming * PointLight.Color.rgb;
        }
    }

    return Result;
}