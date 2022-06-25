// NOTE(georgii): Interleaved gradient noise. Taken from UE4 source code
float InterleavedGradientNoise(vec2 UV, float FrameID)
{
	UV += FrameID * (vec2(47, 17) * 0.695);

    const vec3 Magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(Magic.z * fract(dot(UV, Magic.xy)));
}

// NOTE(georgii): Gradient noise. Taken from Unity source code
vec2 GradientNoiseDir(vec2 P)
{
    P = mod(P, 289);
    float x = mod((34 * P.x + 1) * P.x, 289) + P.y;
    x = mod((34 * x + 1) * x, 289);
    x = fract(x / 41) * 2 - 1;
    return normalize(vec2(x - floor(x + 0.5), abs(x) - 0.5));
}

float GradientNoise(vec2 P)
{
    vec2 ip = floor(P);
    vec2 fp = fract(P);
    float d00 = dot(GradientNoiseDir(ip), fp);
    float d01 = dot(GradientNoiseDir(ip + vec2(0, 1)), fp - vec2(0, 1));
    float d10 = dot(GradientNoiseDir(ip + vec2(1, 0)), fp - vec2(1, 0));
    float d11 = dot(GradientNoiseDir(ip + vec2(1, 1)), fp - vec2(1, 1));
    fp = fp * fp * fp * (fp * (fp * 6 - 15) + 10);
    return mix(mix(d00, d01, fp.y), mix(d10, d11, fp.y), fp.x);
}

float GradientNoiseFloat(vec2 UV, float Scale)
{
    return GradientNoise(UV * Scale) + 0.5;
}

// NOTE(georgii): Simple noise. Taken from Unity source code
// float NoiseRandomValue(vec2 UV)
// {
//     return fract(sin(dot(UV, vec2(12.9898, 78.233))) * 43758.5453);
// }

float NoiseRandomValue(vec2 UV)
{
    return fract(tan(distance(UV * 1.61803398874989484820459, UV)) * UV.x);
}

float NoiseValue(vec2 UV)
{
    vec2 i = floor(UV);
    vec2 f = fract(UV);
    f = f * f * (3.0 - 2.0 * f);

    UV = abs(fract(UV) - 0.5);
    vec2 c0 = i + vec2(0.0, 0.0);
    vec2 c1 = i + vec2(1.0, 0.0);
    vec2 c2 = i + vec2(0.0, 1.0);
    vec2 c3 = i + vec2(1.0, 1.0);
    float r0 = NoiseRandomValue(c0);
    float r1 = NoiseRandomValue(c1);
    float r2 = NoiseRandomValue(c2);
    float r3 = NoiseRandomValue(c3);

    float BottomOfGrid = mix(r0, r1, f.x);
    float TopOfGrid = mix(r2, r3, f.x);
    float t = mix(BottomOfGrid, TopOfGrid, f.y);
    return t;
}

float SimpleNoise(vec2 UV, float Scale)
{
    float t = 0.0;

    t += NoiseValue(vec2(UV.x * Scale, UV.y * Scale)) * 0.125;
    t += NoiseValue(vec2(UV.x * Scale / 2.0, UV.y * Scale / 2.0)) * 0.25;
    t += NoiseValue(vec2(UV.x * Scale / 4.0, UV.y * Scale / 4.0)) * 0.5;

    return t;
}