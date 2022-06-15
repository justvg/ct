#version 450

layout (set = 0, binding = 0, r32f) uniform writeonly image2D OutImage;
layout (set = 0, binding = 1) uniform sampler2D InImage;

layout (push_constant) uniform PushConstants
{
	vec2 SrcImageSize;
	vec2 DstImageSize;
	uint MinMaxSampler;
};

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
	uvec2 TexCoordinate = gl_GlobalInvocationID.xy;

	if ((TexCoordinate.x < DstImageSize.x) && (TexCoordinate.y < DstImageSize.y))
	{
		vec2 UV = (vec2(TexCoordinate) + vec2(0.5)) / DstImageSize;
		
		float Depth;
		if (MinMaxSampler > 0)
		{
			// This computes max depth of 2x2 texel quad
			Depth = texture(InImage, UV).x;
		}
		else
		{
			vec2 SrcTexelSize = 1.0 / SrcImageSize;

			Depth = texture(InImage, UV + 0.51 * vec2(-SrcTexelSize.x, -SrcTexelSize.y)).x;
			Depth = max(Depth, texture(InImage, UV + 0.51 * vec2(SrcTexelSize.x, -SrcTexelSize.y)).x);
			Depth = max(Depth, texture(InImage, UV + 0.51 * vec2(-SrcTexelSize.x, SrcTexelSize.y)).x);
			Depth = max(Depth, texture(InImage, UV + 0.51 * vec2(SrcTexelSize.x, SrcTexelSize.y)).x);
		}
		
		imageStore(OutImage, ivec2(TexCoordinate), vec4(Depth));
	}
}