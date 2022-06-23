#version 450

layout (location = 0) out vec2 TexCoords;

layout (location = 0) in vec2 Position;

layout (push_constant) uniform PushConstants
{
    vec4 PositionScale;
	vec4 MaxMinTC;
	
    uint FontRendering;
	float BlendFactor;
	vec4 FontColor;
};

layout (set = 0, binding = 0) uniform Projection
{
	mat4 OrtoProj;
};

void main()
{
	TexCoords = Position * vec2(0.5, 0.5) + vec2(0.5, 0.5);
	TexCoords = mix(MaxMinTC.zw, MaxMinTC.xy, TexCoords);

	vec2 P = PositionScale.xy;
	vec2 S = PositionScale.zw;

    gl_Position = OrtoProj * vec4(0.5*S*Position + P, 0.0, 1.0);
}