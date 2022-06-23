#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 1, binding = 0) uniform sampler2D Texture;

layout (push_constant) uniform PushConstants
{
    vec4 PositionScale;
	vec4 MaxMinTC;

    uint FontRendering;
    float BlendFactor;
	vec4 FontColor;
};

void main()
{
    if (FontRendering > 0)
    {
        FragColor = vec4(FontColor.rgb, FontColor.a * BlendFactor * texture(Texture, TexCoords).a);
    }
    else
    {
        FragColor = texture(Texture, TexCoords);
    }
}