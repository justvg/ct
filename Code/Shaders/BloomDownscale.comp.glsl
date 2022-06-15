#version 450

layout (set = 0, binding = 0, rgba16f) uniform writeonly image2D OutImage;
layout (set = 0, binding = 1) uniform sampler2D InImage;

layout (push_constant) uniform PushConstants
{
	vec2 ImageSizeSrc;
    vec2 ImageSizeDst;
};

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
    uvec2 TexCoordinate = gl_GlobalInvocationID.xy;

    if ((TexCoordinate.x < ImageSizeDst.x) && (TexCoordinate.y < ImageSizeDst.y))
    {
        vec2 UV = (vec2(TexCoordinate) + vec2(0.5)) / ImageSizeDst;
        vec2 SrcTexelSize = vec2(1.0 / ImageSizeSrc);

        vec4 Center = vec4(0.0);
        Center += texture(InImage, UV + vec2(-SrcTexelSize.x, -SrcTexelSize.y));
        Center += texture(InImage, UV + vec2(SrcTexelSize.x, -SrcTexelSize.y));
        Center += texture(InImage, UV + vec2(-SrcTexelSize.x, SrcTexelSize.y));
        Center += texture(InImage, UV + vec2(SrcTexelSize.x, SrcTexelSize.y));
        Center *= 0.25;

        vec4 LeftTop = vec4(0.0);
        LeftTop += texture(InImage, UV + vec2(-SrcTexelSize.x, -SrcTexelSize.y) + vec2(-SrcTexelSize.x, -SrcTexelSize.y));
        LeftTop += texture(InImage, UV + vec2(-SrcTexelSize.x, -SrcTexelSize.y) + vec2(SrcTexelSize.x, -SrcTexelSize.y));
        LeftTop += texture(InImage, UV + vec2(-SrcTexelSize.x, -SrcTexelSize.y) + vec2(-SrcTexelSize.x, SrcTexelSize.y));
        LeftTop += texture(InImage, UV + vec2(-SrcTexelSize.x, -SrcTexelSize.y) + vec2(SrcTexelSize.x, SrcTexelSize.y));
        LeftTop *= 0.25;

        vec4 RightTop = vec4(0.0);
        RightTop += texture(InImage, UV + vec2(SrcTexelSize.x, -SrcTexelSize.y) + vec2(-SrcTexelSize.x, -SrcTexelSize.y));
        RightTop += texture(InImage, UV + vec2(SrcTexelSize.x, -SrcTexelSize.y) + vec2(SrcTexelSize.x, -SrcTexelSize.y));
        RightTop += texture(InImage, UV + vec2(SrcTexelSize.x, -SrcTexelSize.y) + vec2(-SrcTexelSize.x, SrcTexelSize.y));
        RightTop += texture(InImage, UV + vec2(SrcTexelSize.x, -SrcTexelSize.y) + vec2(SrcTexelSize.x, SrcTexelSize.y));
        RightTop *= 0.25;

        vec4 LeftBot = vec4(0.0);
        LeftBot += texture(InImage, UV + vec2(-SrcTexelSize.x, SrcTexelSize.y) + vec2(-SrcTexelSize.x, -SrcTexelSize.y));
        LeftBot += texture(InImage, UV + vec2(-SrcTexelSize.x, SrcTexelSize.y) + vec2(SrcTexelSize.x, -SrcTexelSize.y));
        LeftBot += texture(InImage, UV + vec2(-SrcTexelSize.x, SrcTexelSize.y) + vec2(-SrcTexelSize.x, SrcTexelSize.y));
        LeftBot += texture(InImage, UV + vec2(-SrcTexelSize.x, SrcTexelSize.y) + vec2(SrcTexelSize.x, SrcTexelSize.y));
        LeftBot *= 0.25;

        vec4 RightBot = vec4(0.0);
        RightBot += texture(InImage, UV + vec2(SrcTexelSize.x, SrcTexelSize.y) + vec2(-SrcTexelSize.x, -SrcTexelSize.y));
        RightBot += texture(InImage, UV + vec2(SrcTexelSize.x, SrcTexelSize.y) + vec2(SrcTexelSize.x, -SrcTexelSize.y));
        RightBot += texture(InImage, UV + vec2(SrcTexelSize.x, SrcTexelSize.y) + vec2(-SrcTexelSize.x, SrcTexelSize.y));
        RightBot += texture(InImage, UV + vec2(SrcTexelSize.x, SrcTexelSize.y) + vec2(SrcTexelSize.x, SrcTexelSize.y));
        RightBot *= 0.25;

        vec4 Color = 0.5 * Center + 0.125 * LeftTop + 0.125 * RightTop + 0.125 * LeftBot + 0.125 * RightBot;
        imageStore(OutImage, ivec2(TexCoordinate), Color);
    }
}