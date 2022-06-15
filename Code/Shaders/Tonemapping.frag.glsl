#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D InputTexture;
layout (set = 0, binding = 1) uniform sampler2D ExposureTexture;
layout (set = 0, binding = 2) uniform sampler2D BloomTexture;

void main()
{
    vec3 Color = texture(InputTexture, TexCoords).rgb + texture(BloomTexture, TexCoords).rgb;
    
    // Exposure
    float Exposure = texture(ExposureTexture, vec2(0.5, 0.5)).r;
    Color *= 1.0;//Exposure;

    // Tone mapping
    vec3 ColorFinal = vec3(1.0) - exp(-Color);

    // Gamma correction
    float Gamma = 2.2;
    ColorFinal = pow(ColorFinal, vec3(1.0 / Gamma));
    FragColor = vec4(ColorFinal, 1.0);
}