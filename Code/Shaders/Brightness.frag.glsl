#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D InputTexture;

void main()
{
    vec3 Color = texture(InputTexture, TexCoords).rgb;
    float Grayscale = dot(Color, vec3(0.299, 0.587, 0.114));

    FragColor = vec4(Grayscale, Grayscale, Grayscale, 1.0);
}