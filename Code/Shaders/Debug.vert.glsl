#version 450

layout (location = 0) in vec3 LocalPosition;
layout (location = 1) in vec3 LocalNormal;

layout (push_constant) uniform PushConstants
{
	mat4 Model;
	vec4 Color;
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
    vec4 FrustumCorners[6];
};

void main()
{
    gl_Position = ProjUnjittered * View * Model * vec4(LocalPosition, 1.0);
}