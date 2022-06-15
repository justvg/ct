#version 450

layout (location = 0) out vec4 FragColor;

layout (push_constant) uniform PushConstants
{
	mat4 Model;
	vec4 Color;
};

void main()
{
    FragColor = Color;
}