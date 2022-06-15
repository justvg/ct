#version 460

layout (location = 0) out vec3 NormalWS;
layout (location = 1) out vec3 FragPosWS;
layout (location = 2) out vec3 FragPrevPosWS;
layout (location = 3) out vec3 Color;

layout (location = 0) in vec3 LocalPosition;
layout (location = 1) in vec3 LocalNormal;

layout (push_constant) uniform PushConstants
{
	vec4 Position; // w - unused
	vec4 Scale; // w - unused
	vec4 Orientation; // w - unused
	vec4 MeshColor;
	vec4 Offset; // w contains point lights count

	vec4 PrevPosition; // w - unused
	vec4 PrevOrientation; // w - unused

    uint FrameNumber;
	uint FPWeaponDepthTest;
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

vec3 RotateQuaternion(vec3 V, vec4 Q)
{
	return V + 2.0 * cross(Q.xyz, cross(Q.xyz, V) + Q.w * V);
}

void main()
{
	vec3 P = Position.xyz;
	vec3 S = Scale.xyz;
	vec4 O = Orientation;
	vec3 C = MeshColor.rgb;
	
	vec3 PrevP = PrevPosition.xyz;
	vec4 PrevO = PrevOrientation;

	NormalWS = normalize(RotateQuaternion(LocalNormal * (1.0 / S), O));
	FragPosWS = RotateQuaternion((LocalPosition * S) + Offset.xyz, O) + P;
	FragPrevPosWS = RotateQuaternion((LocalPosition * S) + Offset.xyz, PrevO) + PrevP;
	Color = C;

	gl_Position = Proj * View * vec4(FragPosWS, 1.0);
	
	if (FPWeaponDepthTest == 0)
	{
		gl_Position.z = 0.0;
	}
}