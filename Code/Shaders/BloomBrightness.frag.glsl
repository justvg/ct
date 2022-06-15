#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D InputTexture;

void main()
{
    vec2 TargetTextureSize = 0.5 * textureSize(InputTexture, 0).xy;
    vec2 TexelSize = 1.0 / TargetTextureSize;
    vec2 HalfTexelSize = 0.5 * TexelSize;

    vec4 TopLeft = texture(InputTexture, TexCoords + vec2(-HalfTexelSize.x, -HalfTexelSize.y));
    vec4 TopRight = texture(InputTexture, TexCoords + vec2(HalfTexelSize.x, -HalfTexelSize.y));
    vec4 BotLeft = texture(InputTexture, TexCoords + vec2(-HalfTexelSize.x, HalfTexelSize.y));
    vec4 BotRight = texture(InputTexture, TexCoords + vec2(HalfTexelSize.x, HalfTexelSize.y));

    vec4 Max = max(max(TopLeft, TopRight), max(BotLeft, BotRight));
    float MaxComponent = max(Max.r, max(Max.g, Max.b)) + 0.001;

    const float Threshold = 1.0;
    Max *= clamp(MaxComponent - Threshold, 0.0, 0.5) / MaxComponent;

    FragColor = 0.12 * Max;
}