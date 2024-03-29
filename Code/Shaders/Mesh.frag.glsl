#version 450

#extension GL_GOOGLE_include_directive: require

layout (location = 0) out vec3 FragAlbedo;
layout (location = 1) out vec3 FragNormal;
layout (location = 2) out vec2 FragMaterial;
layout (location = 3) out vec2 FragVelocity;
layout (location = 4) out float FragLinearDepth;

layout (location = 0) in vec3 NormalWS;
layout (location = 1) in vec3 FragPosWS;
layout (location = 2) in vec3 FragPrevPosWS;
layout (location = 3) in vec3 LocalPos;

layout (push_constant) uniform PushConstants
{
	vec4 Position; // w - unused
	vec4 Scale; // w - unused
	vec4 Orientation;
	vec4 MeshColor;
	vec4 Offset; // w - unused

	vec4 PrevPosition; // w - unused
	vec4 PrevOrientation; // w - unused

	uint FirstPersonDepthTest;

	float Time;
	float ShaderValue0; // NOTE(georgii): Currently used for material parameters. Like MaxComponentNoise in door shader
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
	// TODO(georgii): This was kinad added for torches to have black outlines. Maybe I should add an ability to use different shaders for opaque objects.
	vec3 UV = abs(2.0 * vec3(LocalPos.x, LocalPos.y, LocalPos.z)) - vec3(0.9);
	float Dark = (UV.y > 0.0 && UV.z > 0.0) || (UV.x > 0.0 && UV.z > 0.0) || (UV.x > 0.0 && UV.y > 0.0) ? 0.0 : 1.0;

	vec3 Normal = normalize(NormalWS);
	vec3 Color = Dark * MeshColor.rgb;

	vec4 CurrentTexCoords = ProjUnjittered * View * vec4(FragPosWS, 1.0);
	CurrentTexCoords.xy /= CurrentTexCoords.w;
	CurrentTexCoords.xy = CurrentTexCoords.xy * vec2(0.5, -0.5) + vec2(0.5, 0.5);

	vec4 PrevTexCoords = PrevProj * PrevView * vec4(FragPrevPosWS, 1.0);
	PrevTexCoords.xy /= PrevTexCoords.w;
	PrevTexCoords.xy = PrevTexCoords.xy * vec2(0.5, -0.5) + vec2(0.5, 0.5);
	
	vec2 Velocity = CurrentTexCoords.xy - PrevTexCoords.xy;

	FragAlbedo = Color;
	FragNormal = Normal;
	FragMaterial = vec2(0.0, 0.0);
	FragVelocity = Velocity;
    FragLinearDepth = length(FragPosWS - CameraPosition.xyz) / Viewport.w;
}