#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D InputTexture;

layout (push_constant) uniform PushConstants
{
	vec2 ImageSizeSrc;
    vec2 ImageSizeDst;
};

void main()
{
    vec2 TexelSizeSrc = vec2(1.0) / ImageSizeSrc;
    float Radius = 1.5;

    vec4 TopLeft = texture(InputTexture, TexCoords + Radius * vec2(-TexelSizeSrc.x, -TexelSizeSrc.y));
    vec4 Top = 2.0 * texture(InputTexture, TexCoords + Radius * vec2(0.0, -TexelSizeSrc.y));
    vec4 TopRight = texture(InputTexture, TexCoords + Radius * vec2(TexelSizeSrc.x, -TexelSizeSrc.y));

    vec4 Left = 2.0 * texture(InputTexture, TexCoords + Radius * vec2(-TexelSizeSrc.x, 0.0));
    vec4 Center = 4.0 * texture(InputTexture, TexCoords);
    vec4 Right = 2.0 * texture(InputTexture, TexCoords + Radius * vec2(TexelSizeSrc.x, 0.0));

    vec4 BotLeft = texture(InputTexture, TexCoords + Radius * vec2(-TexelSizeSrc.x, TexelSizeSrc.y));
    vec4 Bot = 2.0 * texture(InputTexture, TexCoords + Radius * vec2(0.0, TexelSizeSrc.y));
    vec4 BotRight = texture(InputTexture, TexCoords + Radius * vec2(TexelSizeSrc.x, TexelSizeSrc.y));

    FragColor = (TopLeft + Top + TopRight + Left + Center + Right + BotLeft + Bot + BotRight) / 16.0;
}