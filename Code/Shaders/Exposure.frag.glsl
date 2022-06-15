#version 450

layout (location = 0) out float FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D BrightnessTexture;
layout (set = 0, binding = 1) uniform sampler2D LastExposureTexture;

void main()
{
    float Center = textureLod(BrightnessTexture, vec2(0.5, 0.5), 6).r;
    float TopLeft = textureLod(BrightnessTexture, vec2(0.25, 0.25), 6).r;
    float TopRight = textureLod(BrightnessTexture, vec2(0.75, 0.25), 6).r;
    float BotLeft = textureLod(BrightnessTexture, vec2(0.25, 0.75), 6).r;
    float BotRight = textureLod(BrightnessTexture, vec2(0.75, 0.75), 6).r;

    float Brightness = 0.4 * Center + 0.15 * TopLeft + 0.15 * TopRight + 0.15 * BotLeft + 0.15 * BotRight;
    float NewExposure = clamp(0.1 / (Brightness + 0.0001), 1.0, 2.5);

    float LastExposure = texture(LastExposureTexture, vec2(0.5, 0.5)).r;

    FragColor = mix(LastExposure, NewExposure, 0.025);
}