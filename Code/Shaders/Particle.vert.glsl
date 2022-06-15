#version 450

layout (location = 0) out vec3 NormalWS;
layout (location = 1) out vec3 FragPosWS;
layout (location = 2) out vec3 Color;

layout (location = 0) in vec3 LocalPosition;
layout (location = 1) in vec3 LocalNormal;

struct SParticleDraw
{
    vec3 Position;
    float Scale;

    vec3 Color;
    float _Padding1;
};

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 View;
	mat4 Proj;
	mat4 ProjUnjittered;

	mat4 PrevView;
	mat4 PrevProj;
	
	vec4 CameraPosition; 
    vec4 Viewport; // Width, Height, Near, Far
	vec4 Frustum[6];
};

layout (set = 0, binding = 1) readonly buffer Draws
{
    SParticleDraw Draw[];
};

void main()
{
    vec3 P = Draw[gl_InstanceIndex].Position;
    float S = Draw[gl_InstanceIndex].Scale;
    vec3 C = Draw[gl_InstanceIndex].Color;

    NormalWS = LocalNormal;
    FragPosWS = S*LocalPosition + P;
    Color = C;

    gl_Position = Proj * View * vec4(FragPosWS, 1.0);
}