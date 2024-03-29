#pragma once

struct SCameraBuffer
{
	mat4 View;
	mat4 Proj;
	mat4 ProjUnjittered;

	mat4 PrevView;
	mat4 PrevProj;

	vec4 Pos;
	vec4 Viewport;
	vec4 Frustums[6];
    vec4 FrustumCorners[6];
};

struct SHUDProjectionBuffer
{
	mat4 OrthoProj;
};

struct SLightBuffer
{
	vec4 AmbientColor; // w - unused
	vec4 AmbientConstant; // w - unused
};

enum EFont
{
	Font_KarminaRegular,
	Font_KarminaBold,

	Font_Count
};

struct SRenderer
{
	uint32_t StagingBufferOffsets[FramesInFlight];
	SBuffer StagingBuffers[FramesInFlight];
	SBuffer VertexBuffer;
	SBuffer IndexBuffer;
	SBuffer VoxelDrawBuffer;
	SBuffer VoxelVisibilityBuffer;
	SBuffer IndirectBuffer;
	SBuffer CountBuffer;
	SBuffer VoxelsBuffer;
	
	SBuffer QuadVB;
	SBuffer CubeVB;
	SBuffer CubeIB;

	VkSampler MaxReductionSampler;
	VkSampler PointRepeatSampler;
	VkSampler PointEdgeSampler;
	VkSampler LinearEdgeSampler;
	VkSampler LinearEdgeSamplerMips;
	VkSampler LinearBorderZeroSampler;

	VkDescriptorPool DescriptorPool;

	SImage AlbedoImage;
	SImage NormalsImage;
	SImage MaterialImage;
	SImage DepthImage;
	SImage LinearDepthImage;
	SImage VelocityImages[2];

	SImage DepthPyramidImage;
	uint32_t DepthPyramidMipCount;
	VkImageView DepthPyramidMipViews[16];

	SImage DiffuseLightImage;
	SImage DiffuseLightHistoryImages[2];
	SImage DiffuseImage;

	SImage SpecularLightImage;
	SImage SpecularLightHistoryImages[2];
	SImage CompositeImage;

	SImage BrightnessImage;
	VkImageView BrightnessMipViews[SExposureRenderPass::BrightnessImageMipCount];
	SImage ExposureImages[2];

	SImage BloomImage;
	VkImageView BloomMipViews[SBloomRenderPass::BloomMipsCount];

	SImage HistoryImages[2];
	SImage FinalImage;

	SCameraBuffer CameraBufferData;
	SBuffer CameraBuffers[FramesInFlight];
	SBuffer LightBuffers[FramesInFlight];
	SBuffer LightsBuffers[FramesInFlight];
	SBuffer HUDProjectionBuffers[FramesInFlight];

	SVoxelCullingComputePass CullingVoxComputePass;
	SGBufferVoxelRenderPass GBufferVoxelRenderPass;
	SGBufferRenderPass GBufferRenderPass;
	SDownscaleComputePass DownscaleComputePass;
	SDiffuseLightingPass DiffuseLightingPass;
	STransparentRenderPass TransparentRenderPass;
	SSpecularLightingPass SpecularLightingPass;
	SFogRenderPass FogRenderPass;
	SFirstPersonRenderPass FirstPersonRenderPass;
	SExposureRenderPass ExposureRenderPass;
	SBloomRenderPass BloomRenderPass;
	STaaRenderPass TaaRenderPass;
	SToneMappingRenderPass ToneMappingRenderPass;
	SHUDRenderPass HudRenderPass;
	SDebugRenderPass DebugRenderPass;

	SFont Fonts[Font_Count];

	SImage BlueNoiseTexture;

	VkDescriptorSet AimTextureDescrSet;
	SImage AimTexture;

	EAOQuality AOQuality;
};