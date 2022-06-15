#version 450

layout (set = 0, binding = 0, rgba16f) uniform writeonly image2D OutImage;
layout (set = 0, binding = 1) uniform sampler2D InImage;

layout (push_constant) uniform PushConstants
{
	vec2 ImageSize;
};

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
    uvec2 TexCoordinate = gl_GlobalInvocationID.xy;

    if ((TexCoordinate.x < ImageSize.x) && (TexCoordinate.y < ImageSize.y))
    {
        vec2 UV = (vec2(TexCoordinate) + vec2(0.5)) / ImageSize;

        vec4 Color = texture(InImage, UV);

        imageStore(OutImage, ivec2(TexCoordinate), Color);
    }
}