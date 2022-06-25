vec2 BlueNoiseTC = gl_FragCoord.xy / textureSize(BlueNoiseTexture, 0).x;

const float Ratio1 = 0.618034005;
const vec2 Ratio2 = vec2(0.75487762, 0.56984027);
const vec3 Ratio3 = vec3(0.819172502, 0.671043575, 0.549700439);

float BlueNoiseFloat()
{
    float Noise = texture(BlueNoiseTexture, BlueNoiseTC).r;
    float Result = fract(Noise + Ratio1 * FrameNumber);
    BlueNoiseTC += Ratio2;
    
    return Result;
}

vec2 BlueNoiseVec2()
{
    vec2 Noise = texture(BlueNoiseTexture, BlueNoiseTC).rg;
    vec2 Result = fract(Noise + Ratio2 * FrameNumber);
    BlueNoiseTC += Ratio2;

    return Result;
}

vec3 BlueNoiseVec3()
{
    vec3 Noise = texture(BlueNoiseTexture, BlueNoiseTC).rgb;
    vec3 Result = fract(Noise + Ratio3 * FrameNumber);
    BlueNoiseTC += Ratio2;

    return Result;
}