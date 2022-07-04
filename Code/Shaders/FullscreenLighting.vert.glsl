#version 450

layout (location = 0) out vec2 TexCoords;
layout (location = 1) out vec3 CameraVector;

layout (location = 0) in vec2 Position;

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
    TexCoords = Position * vec2(0.5, -0.5) + vec2(0.5, 0.5);
	CameraVector = FrustumCorners[gl_VertexIndex].xyz;

    gl_Position = vec4(Position, 0.0, 1.0);
}