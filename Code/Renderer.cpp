#include "Renderer.h"

void InitializeRenderer(SRenderer* Renderer, const SVulkanContext& Vulkan, const SGeometry* Geometry)
{
	for (uint32_t I = 0; I < ArrayCount(Renderer->StagingBuffers); I++)
	{
		Renderer->StagingBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(64), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	}
	Renderer->VertexBuffer = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(8), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->IndexBuffer = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(8), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->VoxelDrawBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SEngineState::VoxelDraws), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->VoxelVisibilityBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SEngineState::VoxelVisibilities), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	if (!Vulkan.vkCmdDrawIndexedIndirectCount)
	{
		Renderer->IndirectBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(VkDrawIndexedIndirectCommand) * ArrayCount(SEngineState::VoxelDraws), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}
	else
	{
		Renderer->IndirectBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(VkDrawIndexedIndirectCommand) * ArrayCount(SEngineState::VoxelDraws), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}
	Renderer->CountBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(uint32_t), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->VoxelsBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SVoxels), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->QuadVB = CreateBuffer(Vulkan.MemoryAllocator, 6*sizeof(vec2), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->CubeVB = CreateBuffer(Vulkan.MemoryAllocator, 24*sizeof(SVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->CubeIB = CreateBuffer(Vulkan.MemoryAllocator, 36*sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	
	Renderer->BlueNoiseTexture = LoadTexture(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->StagingBuffers[0], Vulkan.MemoryAllocator, "Textures\\blue_noise.bmp");
	
	Renderer->MaxReductionSampler = CreateSampler(Vulkan.Device, Vulkan.bMinMaxSampler ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 16.0f, Vulkan.bMinMaxSampler ? VK_SAMPLER_REDUCTION_MODE_MAX : VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);
	Renderer->PointRepeatSampler = CreateSampler(Vulkan.Device, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	Renderer->PointEdgeSampler = CreateSampler(Vulkan.Device, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	Renderer->LinearEdgeSampler = CreateSampler(Vulkan.Device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	Renderer->LinearEdgeSamplerMips = CreateSampler(Vulkan.Device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 16.0f);
	Renderer->LinearBorderZeroSampler = CreateSampler(Vulkan.Device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	
	Renderer->DescriptorPool = CreateDescriptorPool(Vulkan.Device);
	
	Renderer->AlbedoImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.DepthFormat, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	Renderer->NormalsImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R8G8B8A8_SNORM, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->MaterialImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->LinearDepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	for (uint32_t I = 0; I < ArrayCount(Renderer->VelocityImages); I++)
	{
		Renderer->VelocityImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	SDepthPyramidInfoResult DepthPyramidInfo = GetDepthPyramidInfo(Vulkan.InternalWidth, Vulkan.InternalHeight);
	Renderer->DepthPyramidImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, DepthPyramidInfo.Width, DepthPyramidInfo.Height, 0, DepthPyramidInfo.MipCount, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthPyramidMipCount = DepthPyramidInfo.MipCount;
	for (uint32_t I = 0; I < Renderer->DepthPyramidMipCount; I++)
	{
		Renderer->DepthPyramidMipViews[I] = CreateImageView(Vulkan.Device, Renderer->DepthPyramidImage.Image, Renderer->DepthPyramidImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Renderer->DiffuseLightImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DiffuseImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->DiffuseLightHistoryImages); I++)
	{
		Renderer->DiffuseLightHistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Renderer->SpecularLightImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->CompositeImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->SpecularLightHistoryImages); I++)
	{
		Renderer->SpecularLightHistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	uint32_t BrightnessImageMipCount = ArrayCount(Renderer->BrightnessMipViews);
	Renderer->BrightnessImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, SExposureRenderPass::BrightnessImageWidth, SExposureRenderPass::BrightnessImageHeight, 0, BrightnessImageMipCount, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < BrightnessImageMipCount; I++)
	{
		Renderer->BrightnessMipViews[I] = CreateImageView(Vulkan.Device, Renderer->BrightnessImage.Image, Renderer->BrightnessImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	for(uint32_t I = 0; I < ArrayCount(Renderer->ExposureImages); I++)
	{
		Renderer->ExposureImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, 1, 1, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Renderer->BloomImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Renderer->AlbedoImage.Width >> 1, Renderer->AlbedoImage.Height >> 1, 0, ArrayCount(Renderer->BloomMipViews), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->BloomMipViews); I++)
	{
		Renderer->BloomMipViews[I] = CreateImageView(Vulkan.Device, Renderer->BloomImage.Image, Renderer->BloomImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	for(uint32_t I = 0; I < ArrayCount(Renderer->HistoryImages); I++)
	{
		Renderer->HistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Renderer->FinalImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.SwapchainFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	for (uint32_t I = 0; I < FramesInFlight; I++)
	{
		Renderer->CameraBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SCameraBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Renderer->LightBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SLightBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Renderer->LightsBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, ArrayCount(SLevel::Lights) * sizeof(SLight), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Renderer->HUDProjectionBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SHUDProjectionBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
	Renderer->CullingVoxComputePass = SVoxelCullingComputePass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelDrawBuffer, Renderer->IndirectBuffer, Renderer->CountBuffer, Renderer->VoxelVisibilityBuffer, Renderer->MaxReductionSampler, Renderer->DepthPyramidImage);
	Renderer->GBufferVoxelRenderPass = SGBufferVoxelRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelDrawBuffer, Renderer->VoxelsBuffer, Renderer->AlbedoImage, Renderer->NormalsImage, Renderer->MaterialImage, Renderer->VelocityImages, Renderer->LinearDepthImage, Renderer->DepthImage);
	Renderer->GBufferRenderPass = SGBufferRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->AlbedoImage, Renderer->NormalsImage, Renderer->MaterialImage, Renderer->VelocityImages, Renderer->LinearDepthImage, Renderer->DepthImage);
	Renderer->DownscaleComputePass = SDownscaleComputePass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearDepthImage, Renderer->DepthPyramidMipCount, Renderer->DepthPyramidMipViews, Renderer->PointEdgeSampler, Renderer->MaxReductionSampler);
	Renderer->DiffuseLightingPass = SDiffuseLightingPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelsBuffer, Renderer->PointRepeatSampler, Renderer->BlueNoiseTexture, Renderer->LightsBuffers, Renderer->LightBuffers, Renderer->PointEdgeSampler, Renderer->NormalsImage, Renderer->LinearDepthImage, Renderer->VelocityImages, Renderer->LinearEdgeSampler, Renderer->AlbedoImage, Renderer->DiffuseLightHistoryImages, Renderer->DiffuseLightImage, Renderer->DiffuseImage);
	Renderer->TransparentRenderPass = STransparentRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->DiffuseImage, Renderer->DepthImage);
	Renderer->SpecularLightingPass = SSpecularLightingPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelsBuffer, Renderer->PointRepeatSampler, Renderer->BlueNoiseTexture, Renderer->LightsBuffers, Renderer->LightBuffers, Renderer->PointEdgeSampler, Renderer->NormalsImage, Renderer->LinearDepthImage, Renderer->LinearEdgeSampler, Renderer->MaterialImage, Renderer->DiffuseImage, Renderer->VelocityImages, Renderer->AlbedoImage, Renderer->SpecularLightImage, Renderer->SpecularLightHistoryImages, Renderer->CompositeImage);
	Renderer->FogRenderPass = SFogRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->PointEdgeSampler, Renderer->LinearDepthImage, Renderer->DiffuseImage);
	Renderer->FirstPersonRenderPass = SFirstPersonRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->CompositeImage, Renderer->VelocityImages);
	Renderer->ExposureRenderPass = SExposureRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->CompositeImage, Renderer->LinearEdgeSamplerMips, Renderer->BrightnessImage, Renderer->BrightnessMipViews, Renderer->PointEdgeSampler, Renderer->ExposureImages);
	Renderer->BloomRenderPass = SBloomRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->BloomImage, Renderer->BloomMipViews, Renderer->LinearBorderZeroSampler);
	Renderer->TaaRenderPass = STaaRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->CompositeImage, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->VelocityImages);
	Renderer->ToneMappingRenderPass = SToneMappingRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->PointEdgeSampler, Renderer->HistoryImages, Renderer->ExposureImages, Renderer->LinearEdgeSampler, Renderer->BloomImage, Renderer->PointRepeatSampler, Renderer->BlueNoiseTexture, Renderer->FinalImage);
	Renderer->HudRenderPass = SHUDRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->HUDProjectionBuffers, Renderer->FinalImage);
	Renderer->DebugRenderPass = SDebugRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->FinalImage);
	
	Renderer->AimTexture = LoadTexture(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->StagingBuffers[0], Vulkan.MemoryAllocator, "Textures\\aim_texture.bmp");
	Renderer->AimTextureDescrSet = CreateDescriptorSet(Vulkan.Device, Renderer->DescriptorPool, Renderer->HudRenderPass.GetTextureDescrSetLayout());
	UpdateDescriptorSetImage(Vulkan.Device, Renderer->AimTextureDescrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Renderer->LinearEdgeSampler, Renderer->AimTexture.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	Renderer->Fonts[Font_KarminaRegular] = LoadFont(Vulkan.Device, Renderer->DescriptorPool, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->StagingBuffers[0], Vulkan.MemoryAllocator, Renderer->HudRenderPass.GetTextureDescrSetLayout(), "KarminaRegular");
	Renderer->Fonts[Font_KarminaBold] = LoadFont(Vulkan.Device, Renderer->DescriptorPool, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->StagingBuffers[0], Vulkan.MemoryAllocator, Renderer->HudRenderPass.GetTextureDescrSetLayout(), "KarminaBold");

	Renderer->AOQuality = AOQuality_High;

	vec2 QuadVertices[] =
	{
		Vec2(-1.0f, 1.0f),
		Vec2(-1.0f, -1.0f),
		Vec2(1.0f, 1.0f),
		
		Vec2(1.0f, 1.0f),
		Vec2(-1.0f, -1.0f),
		Vec2(1.0f, -1.0f)
	};
	
	SVertex CubeVertices[] = 
	{
		{ Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(0.5f, 0.5f, -0.5f),   Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(0.5f, -0.5f, -0.5f),  Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(-0.5f, 0.5f, -0.5f),  Vec3(0.0f, 0.0f, -1.0f) },
		
		{ Vec3(-0.5f, -0.5f, 0.5f),  Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.5f, -0.5f, 0.5f),   Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.5f, 0.5f, 0.5f),    Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(-0.5f, 0.5f, 0.5f),   Vec3(0.0f, 0.0f, 1.0f) },
		
		{ Vec3(-0.5f, 0.5f, 0.5f),   Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(-0.5f, 0.5f, -0.5f),  Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(-0.5f, -0.5f, -0.5f), Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(-0.5f, -0.5f, 0.5f),  Vec3(-1.0f, 0.0f, 0.0f) },
		
		{ Vec3(0.5f, 0.5f, 0.5f),    Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, -0.5f),  Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(0.5f, 0.5f, -0.5f),   Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, 0.5f),   Vec3(1.0f, 0.0f, 0.0f) },
		
		{ Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, -0.5f),  Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, 0.5f),   Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(-0.5f, -0.5f, 0.5f),  Vec3(0.0f, -1.0f, 0.0f) },
		
		{ Vec3(-0.5f, 0.5f, -0.5f),  Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(0.5f, 0.5f, 0.5f),    Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(0.5f, 0.5f, -0.5f),   Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(-0.5f, 0.5f, 0.5f),   Vec3(0.0f, 1.0f, 0.0f) },
	};
	
	uint32_t CubeIndices[] = 
	{
		0, 1, 2, 1, 0, 3, 4, 5, 6, 6, 7, 4, 8, 9, 10, 10, 11, 8, 12, 13, 14, 13, 12, 15, 16, 17, 18, 18, 19, 16, 20, 21, 22, 21, 20, 23
	};
	
	UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VertexBuffer, Renderer->StagingBuffers[0], (void*) Geometry->Vertices, Geometry->VertexCount*sizeof(SVertex));
	UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->IndexBuffer, Renderer->StagingBuffers[0], (void*) Geometry->Indices, Geometry->IndexCount*sizeof(uint32_t));
	UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->QuadVB, Renderer->StagingBuffers[0], QuadVertices, sizeof(QuadVertices));
	UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->CubeVB, Renderer->StagingBuffers[0], CubeVertices, sizeof(CubeVertices));
	UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->CubeIB, Renderer->StagingBuffers[0], CubeIndices, sizeof(CubeIndices));
}

VkImage RenderScene(SEngineState* EngineState, SRenderer* Renderer, const SVulkanContext& Vulkan, SLevel* Level, uint32_t LightInFrustumCount, uint32_t LightCount, uint32_t FrameID, STempMemoryArena* MemoryArena, float GameTime, bool bSwapchainChanged, vec2 MousePos)
{
	// Rendering
	BEGIN_PROFILER_BLOCK("RENDERING");


	BEGIN_PROFILER_BLOCK("ENTITIES_CULLING_AND_SORTING");

	uint32_t EntityCount = Level->EntityCount;
	SEntity* RenderEntities = (SEntity*) PushMemory(MemoryArena->Arena, sizeof(SEntity) * EntityCount);

	// Frustum culling
	uint32_t EntityCountAfterCulling = 0;
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		const SEntity& Entity = Level->Entities[I];
		const SMesh& Mesh = EngineState->Geometry.Meshes[Entity.MeshIndex];

		vec3 Points[8];
		PointsCenterDimOrientation(Points, Entity.Pos, Hadamard(Entity.Dim, Mesh.Dim), EulerToQuat(Entity.Orientation.xyz));

		bool bInsideFrustum = true;
		for (uint I = 0; (I < 6) && bInsideFrustum; I++)
		{
			bool bInsidePlane = false;
			for (uint J = 0; J < 8; J++)
			{
				bInsidePlane = bInsidePlane || (Dot(Vec4(Points[J], -1.0f), Renderer->CameraBufferData.Frustums[I]) >= 0.0f);
			}
			bInsideFrustum = bInsideFrustum && bInsidePlane;
		}

		if (bInsideFrustum || (Entity.Type == Entity_Hero))
		{
			RenderEntities[EntityCountAfterCulling++] = Entity;
		}
	}
	EntityCount = EntityCountAfterCulling;

	// Calculate distance to camera for sorting
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		RenderEntities[I].DistanceToCam = Length(RenderEntities[I].Pos - EngineState->Camera.Pos); 
	}

	// Bubble sort 
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		bool bSwapped  = false;
		for (uint32_t J = 0; J < EntityCount - I - 1; J++)
		{
			if ((RenderEntities[J].Alpha == 1.0f) && (RenderEntities[J + 1].Alpha == 1.0f))
			{
				if ((RenderEntities[J].DistanceToCam > RenderEntities[J + 1].DistanceToCam) && (RenderEntities[J].Type != Entity_Hero))
				{
					SEntity Temp = RenderEntities[J];
					RenderEntities[J] = RenderEntities[J + 1];
					RenderEntities[J + 1] = Temp;

					bSwapped = true;
				}
			} 
			else if ((RenderEntities[J].Alpha != 1.0f) && (RenderEntities[J + 1].Alpha != 1.0f))
			{
				if (RenderEntities[J].DistanceToCam < RenderEntities[J + 1].DistanceToCam)
				{
					SEntity Temp = RenderEntities[J];
					RenderEntities[J] = RenderEntities[J + 1];
					RenderEntities[J + 1] = Temp;

					bSwapped = true;
				}
			}
			else if (RenderEntities[J].Alpha != 1.0f)
			{
				SEntity Temp = RenderEntities[J];
				RenderEntities[J] = RenderEntities[J + 1];
				RenderEntities[J + 1] = Temp;

				bSwapped = true;
			}
		}

		if (!bSwapped)
        {
			break;
        }
	}

	// Find opaque and transparent count
	uint32_t EntityCountOpaque = 0;
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		if (RenderEntities[I].Alpha == 1.0f)
		{
			EntityCountOpaque++;
		}
	}
	uint32_t EntityCountTransparent = EntityCount - EntityCountOpaque;

	// Preprocess some entities for render
	const SCamera& Camera = EngineState->Camera;
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		SEntity& Entity = RenderEntities[I];

		if (Entity.Type == Entity_Hero)
		{
			if (EngineState->EngineMode == EngineMode_Game)
			{
				// NOTE(georgii): These hacks are here so a player can always see the lamp.
				float CameraPitch = Clamp(Camera.Pitch, -55.0f, 50.0f);
				float CameraHead = Camera.Head;

				vec3 CameraDir;
				CameraDir.x = Cos(Radians(CameraPitch)) * Sin(Radians(CameraHead));
				CameraDir.y = Sin(Radians(CameraPitch));
				CameraDir.z = Cos(Radians(CameraPitch)) * Cos(Radians(CameraHead));
				CameraDir = Normalize(CameraDir);
				
				vec3 CameraRight = Camera.Right;
				vec3 CameraUp = Camera.Up;

				float CameraPrevPitch = Clamp(Camera.PrevPitch, -55.0f, 50.0f);
				float CameraPrevHead = Camera.PrevHead;
				
				vec3 CameraPrevDir = Camera.PrevDir;
				vec3 CameraPrevRight = Camera.PrevRight;
				vec3 CameraPrevUp = Camera.PrevUp;

				Entity.Pos += (Entity.LampOffset.x * Entity.Dim.x * CameraRight) + (Entity.LampOffset.z * CameraDir) + (Entity.LampOffset.y * CameraUp);
				Entity.Orientation.xyz = Vec3(0.0f);
				Entity.PrevPos += (Entity.PrevLampOffset.x * Entity.Dim.x * CameraPrevRight) + (Entity.PrevLampOffset.z * CameraPrevDir) + (Entity.PrevLampOffset.y * CameraPrevUp);
				Entity.PrevOrientation.xyz = Vec3(0.0f);
			}
		}
	}

	END_PROFILER_BLOCK("ENTITIES_CULLING_AND_SORTING");

    
	// Set viewport
	VkViewport Viewport = { 0.0f, float(Vulkan.InternalHeight), float(Vulkan.InternalWidth), -float(Vulkan.InternalHeight), 0.0f, 1.0f };
	VkRect2D Scissor = { {0, 0}, {Vulkan.InternalWidth, Vulkan.InternalHeight} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);
    
	BEGIN_GPU_PROFILER_BLOCK("RENDER", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	// Frustum cull voxels visible last frame
	Renderer->CullingVoxComputePass.Dispatch(Vulkan, Renderer->IndirectBuffer, Renderer->CountBuffer, Renderer->DepthPyramidImage, ArrayCount(EngineState->VoxelDraws), FrameID, false, bSwapchainChanged);
    
	// Render voxels visibile last frame and frustum culled in this frame to GBuffer
	Renderer->GBufferVoxelRenderPass.RenderEarly(Vulkan, Renderer->VertexBuffer, Renderer->IndexBuffer, Renderer->IndirectBuffer, Renderer->CountBuffer, ArrayCount(EngineState->VoxelDraws), Level->AmbientColor, FrameID, !EngineState->bHideVoxels || (EngineState->EngineMode == EngineMode_Game));
    
	// Downscale linear depth buffer
	Renderer->DownscaleComputePass.Dispatch(Vulkan, Renderer->LinearDepthImage, Renderer->DepthPyramidImage, Renderer->DepthPyramidMipCount);
    
	// Frustum and occlusion cull all voxels
	Renderer->CullingVoxComputePass.Dispatch(Vulkan, Renderer->IndirectBuffer, Renderer->CountBuffer, Renderer->DepthPyramidImage, ArrayCount(EngineState->VoxelDraws), FrameID, true, bSwapchainChanged);
    
	// Render voxels visible this frame that are not already rendered
	Renderer->GBufferVoxelRenderPass.RenderLate(Vulkan, Renderer->VertexBuffer, Renderer->IndexBuffer, Renderer->IndirectBuffer, Renderer->CountBuffer, ArrayCount(EngineState->VoxelDraws), FrameID, !EngineState->bHideVoxels || (EngineState->EngineMode == EngineMode_Game));
    
	// Render entities to GBuffer
	if (!EngineState->bHideEntities || (EngineState->EngineMode == EngineMode_Game))
	{
		Renderer->GBufferRenderPass.Render(Vulkan, RenderEntities, EntityCountOpaque, EngineState->Geometry, Renderer->VertexBuffer, Renderer->IndexBuffer, FrameID, EngineState->EngineMode == EngineMode_Game, GameTime);
	}

	VkImageMemoryBarrier GBufferFinishedBarriers[] = 
	{
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->AlbedoImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->NormalsImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->MaterialImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->VelocityImages[FrameID % 2].Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->LinearDepthImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
	};
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(GBufferFinishedBarriers), GBufferFinishedBarriers);

	if (FrameID == 0)
	{
		VkImageMemoryBarrier VelocityHistoryBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->VelocityImages[(FrameID + 1) % FramesInFlight].Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &VelocityHistoryBarrier);
	}

	// Calculate diffuse lighting
	Renderer->DiffuseLightingPass.Render(Vulkan, Renderer->DiffuseLightImage, Renderer->DiffuseLightHistoryImages, Renderer->QuadVB, Renderer->AOQuality, LightInFrustumCount, FrameID, bSwapchainChanged);

	// Calculate fog
	if (!EngineState->bHideFog || (EngineState->EngineMode == EngineMode_Game))
	{
		Renderer->FogRenderPass.Render(Vulkan, Renderer->QuadVB, *Level, FrameID);
	}

	// Render transparent objects
	if (!EngineState->bHideEntities || (EngineState->EngineMode == EngineMode_Game))
	{
		Renderer->TransparentRenderPass.Render(Vulkan, RenderEntities + EntityCountOpaque, EntityCountTransparent, EngineState->Geometry, Renderer->VertexBuffer, Renderer->IndexBuffer, FrameID, GameTime);
	}

	VkImageMemoryBarrier DiffuseRenderEndBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->DiffuseImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &DiffuseRenderEndBarrier);

	// Calculate specular lighting
	Renderer->SpecularLightingPass.Render(Vulkan, Renderer->SpecularLightImage, Renderer->SpecularLightHistoryImages, Renderer->QuadVB, LightCount, FrameID, bSwapchainChanged);

	// Render first person
	if (EngineState->EngineMode == EngineMode_Game)
	{
		Renderer->FirstPersonRenderPass.Render(Vulkan, RenderEntities[0], EngineState->Geometry, Renderer->VertexBuffer, Renderer->IndexBuffer, FrameID);

		VkImageMemoryBarrier FirstPersonEndBarriers[] = 
		{
			CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->CompositeImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->VelocityImages[FrameID % 2].Image, VK_IMAGE_ASPECT_COLOR_BIT),
		};
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(FirstPersonEndBarriers), FirstPersonEndBarriers);	
	}
	else
	{
		VkImageMemoryBarrier FirstPersonEndBarriers[] = 
		{
			CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->CompositeImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		};
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(FirstPersonEndBarriers), FirstPersonEndBarriers);	
	}

	// Get scene exposure
	// TODO(georgii): Delete this not to waste ms?
	Renderer->ExposureRenderPass.Render(Vulkan, Renderer->QuadVB, Renderer->BrightnessImage, Renderer->ExposureImages, FrameID);
    
	// TAA
	Renderer->TaaRenderPass.Render(Vulkan, Renderer->HistoryImages, Renderer->QuadVB, FrameID, bSwapchainChanged);
    
	// Calculate bloom
	Renderer->BloomRenderPass.Render(Vulkan, Renderer->QuadVB, Renderer->BloomImage, FrameID);

	// Set viewport
	Viewport = { 0.0f, float(Vulkan.Height), float(Vulkan.Width), -float(Vulkan.Height), 0.0f, 1.0f };
	Scissor = { {0, 0}, {Vulkan.Width, Vulkan.Height} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);
    
	// Tone mapping
	Renderer->ToneMappingRenderPass.Render(Vulkan, Renderer->QuadVB, FrameID, EngineState->bMenuOpened, EngineState->MenuOpenedBlend, EngineState->bVignetteEnabled);
    
	// HUD
	SHUDProjectionBuffer HUDProjectionData = { Orthographic(-0.5f*Vulkan.Width, 0.5f*Vulkan.Width, -0.5f*Vulkan.Height, 0.5f*Vulkan.Height, -1.0f, 1.0f)  };
	memcpy(Renderer->HUDProjectionBuffers[Vulkan.FrameInFlight].Data, &HUDProjectionData, sizeof(SHUDProjectionBuffer));
    
	Renderer->HudRenderPass.BeginRender(Vulkan, Renderer->QuadVB);
	
	if (!PlatformIsCursorShowed())
	{
		if (!PlatformIsCursorEnabled())
		{
			Renderer->HudRenderPass.Render(Vulkan, Renderer->AimTextureDescrSet, Vec2(0.0f, 0.0f), Vec2i(Renderer->AimTexture.Width, Renderer->AimTexture.Height));
		}
		else
		{
			Renderer->HudRenderPass.Render(Vulkan, Renderer->AimTextureDescrSet, MousePos, Vec2i(Renderer->AimTexture.Width, Renderer->AimTexture.Height));
		}
	}

	for (uint32_t I = 0; I < EngineState->TextsToRenderCount; I++)
	{
		const SText& Text = EngineState->TextsToRender[I];

		if ((EngineState->bMenuOpened && Text.bMenuText) || (!EngineState->bMenuOpened && !Text.bMenuText))
		{
			const SFont* Font = &Renderer->Fonts[Text.Font];

			vec2 ScreenSizeScale = Vec2(Vulkan.Width / 1920.0f, Vulkan.Height / 1080.0f);
			vec2 TextScale = Hadamard(Text.Scale, ScreenSizeScale);
			vec2 ScreenPosition = Hadamard(Text.Pos, 0.5f * Vec2i(Vulkan.Width, Vulkan.Height));

			vec2 TextSize = GetTextSize(Font, TextScale, Text.String);
			if (Text.Alignment == TextAlignment_Center)
			{
				ScreenPosition -= 0.5f * TextSize;
			}
			else if (Text.Alignment == TextAlignment_Left)
			{
				ScreenPosition.y -= 0.5f * TextSize.y;
			}
			else
			{
				Assert(Text.Alignment == TextAlignment_Right);
				ScreenPosition -= Vec2(TextSize.x, 0.5f * TextSize.y);
			}

			if (Text.bAppearance)
			{
				if (Text.Time - Text.CurrentTime < 1.0f)
				{
					float BlendFactor = Text.Time - Text.CurrentTime;
					Renderer->HudRenderPass.RenderString(Vulkan, Font, ScreenPosition, TextScale, Text.String, Text.Color, BlendFactor);
				}
				else if (Text.CurrentTime >= Text.TimeToStartAppear)
				{
					float AppearanceFactor = Clamp((Text.CurrentTime - Text.TimeToStartAppear) / Text.TimeToAppear, 0.0f, 1.0f);
					Renderer->HudRenderPass.RenderStringWithAppearance(Vulkan, Font, ScreenPosition, TextScale, Text.String, AppearanceFactor);
				}
			}
			else
			{
				Renderer->HudRenderPass.RenderString(Vulkan, Font, ScreenPosition, TextScale, Text.String, Text.Color, Text.BlendFactor);
			}
		}
	}
	
	Renderer->HudRenderPass.EndRender(Vulkan);
    
	Renderer->DebugRenderPass.Render(Vulkan);
    
#ifndef ENGINE_RELEASE
	// DearImgui
	RenderDearImgui(EngineState, &Vulkan, Renderer->HudRenderPass.GetFramebuffer(), FrameID);
#endif
    
	VkImageMemoryBarrier FinalRenderBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->FinalImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &FinalRenderBarrier);
    
	END_GPU_PROFILER_BLOCK("RENDER", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	END_PROFILER_BLOCK("RENDERING");
    
	return Renderer->FinalImage.Image;
}

void RendererHandleChanges(SRenderer* Renderer, const SVulkanContext& Vulkan)
{
	VkCheck(vkDeviceWaitIdle(Vulkan.Device));

	// Destroy images
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->AlbedoImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->NormalsImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->MaterialImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DepthImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->LinearDepthImage);

	for (uint32_t I = 0; I < ArrayCount(Renderer->VelocityImages); I++)
	{
		DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->VelocityImages[I]);
	}
	
	for (uint32_t I = 0; I < Renderer->DepthPyramidMipCount; I++)
	{
		vkDestroyImageView(Vulkan.Device, Renderer->DepthPyramidMipViews[I], 0);
	}
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DepthPyramidImage);
	
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DiffuseLightImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DiffuseImage);
	for (uint32_t I = 0; I < ArrayCount(Renderer->DiffuseLightHistoryImages); I++)
	{
		DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DiffuseLightHistoryImages[I]);
	}

	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->SpecularLightImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->CompositeImage);
	for (uint32_t I = 0; I < ArrayCount(Renderer->SpecularLightHistoryImages); I++)
	{
		DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->SpecularLightHistoryImages[I]);
	}

	for (uint32_t I = 0; I < ArrayCount(Renderer->BloomMipViews); I++)
	{
		vkDestroyImageView(Vulkan.Device, Renderer->BloomMipViews[I], 0);
	}
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->BloomImage);
	
	
	for (uint32_t I = 0; I < ArrayCount(Renderer->HistoryImages); I++)
	{
		DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->HistoryImages[I]);
	}
	
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->FinalImage);
	

	// Create images
	Renderer->AlbedoImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->AlbedoImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.DepthFormat, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	Renderer->NormalsImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->NormalsImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->MaterialImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->MaterialImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->LinearDepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->LinearDepthImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	for (uint32_t I = 0; I < ArrayCount(Renderer->VelocityImages); I++)
	{
		Renderer->VelocityImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->VelocityImages[I].Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	SDepthPyramidInfoResult DepthPyramidInfo = GetDepthPyramidInfo(Vulkan.InternalWidth, Vulkan.InternalHeight);
	Renderer->DepthPyramidImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DepthPyramidImage.Format, DepthPyramidInfo.Width, DepthPyramidInfo.Height, 0, DepthPyramidInfo.MipCount, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthPyramidMipCount = DepthPyramidInfo.MipCount;
	for (uint32_t I = 0; I < Renderer->DepthPyramidMipCount; I++)
	{
		Renderer->DepthPyramidMipViews[I] = CreateImageView(Vulkan.Device, Renderer->DepthPyramidImage.Image, Renderer->DepthPyramidImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Renderer->DiffuseLightImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DiffuseLightImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DiffuseImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DiffuseImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->DiffuseLightHistoryImages); I++)
	{
		Renderer->DiffuseLightHistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DiffuseLightHistoryImages[I].Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Renderer->SpecularLightImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->SpecularLightImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->CompositeImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->CompositeImage.Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->SpecularLightHistoryImages); I++)
	{
		Renderer->SpecularLightHistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->SpecularLightHistoryImages[I].Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Renderer->BloomImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->BloomImage.Format, Renderer->AlbedoImage.Width >> 1, Renderer->AlbedoImage.Height >> 1, 0, ArrayCount(Renderer->BloomMipViews), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->BloomMipViews); I++)
	{
		Renderer->BloomMipViews[I] = CreateImageView(Vulkan.Device, Renderer->BloomImage.Image, Renderer->BloomImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	for(uint32_t I = 0; I < ArrayCount(Renderer->HistoryImages); I++)
	{
		Renderer->HistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->HistoryImages[I].Format, Vulkan.InternalWidth, Vulkan.InternalHeight, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Renderer->FinalImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.SwapchainFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	// Clear images
	{
		VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkCheck(vkBeginCommandBuffer(Vulkan.CommandBuffer, &BeginInfo));

		VkImageMemoryBarrier TransferBarriers[] = 
		{
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->VelocityImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->VelocityImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->DiffuseLightHistoryImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->DiffuseLightHistoryImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->SpecularLightHistoryImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->SpecularLightHistoryImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->HistoryImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->HistoryImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
		};
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(TransferBarriers), TransferBarriers);

		VkClearColorValue ZeroColor = {};
		VkImageSubresourceRange ImageRange = {};
		ImageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageRange.levelCount = VK_REMAINING_MIP_LEVELS;
		ImageRange.layerCount = 1;
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->VelocityImages[0].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->VelocityImages[1].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->DiffuseLightHistoryImages[0].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->DiffuseLightHistoryImages[1].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->SpecularLightHistoryImages[0].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->SpecularLightHistoryImages[1].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->HistoryImages[0].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);
		vkCmdClearColorImage(Vulkan.CommandBuffer, Renderer->HistoryImages[1].Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ZeroColor, 1, &ImageRange);

		VkImageMemoryBarrier EndBarriers[] = 
		{
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->VelocityImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->VelocityImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->DiffuseLightHistoryImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->DiffuseLightHistoryImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->SpecularLightHistoryImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->SpecularLightHistoryImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->HistoryImages[0].Image, VK_IMAGE_ASPECT_COLOR_BIT),
			CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->HistoryImages[1].Image, VK_IMAGE_ASPECT_COLOR_BIT),
		};
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(EndBarriers), EndBarriers);

		VkCheck(vkEndCommandBuffer(Vulkan.CommandBuffer));

		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &Vulkan.CommandBuffer;
		VkCheck(vkQueueSubmit(Vulkan.GraphicsQueue, 1, &SubmitInfo, 0));

		VkCheck(vkDeviceWaitIdle(Vulkan.Device));
	}
	
	// Update render passes
	Renderer->CullingVoxComputePass.UpdateAfterResize(Vulkan, Renderer->MaxReductionSampler, Renderer->DepthPyramidImage);
	Renderer->GBufferVoxelRenderPass.UpdateAfterResize(Vulkan, Renderer->AlbedoImage, Renderer->NormalsImage, Renderer->MaterialImage, Renderer->VelocityImages, Renderer->LinearDepthImage, Renderer->DepthImage);
	Renderer->GBufferRenderPass.UpdateAfterResize(Vulkan, Renderer->AlbedoImage, Renderer->NormalsImage, Renderer->MaterialImage, Renderer->VelocityImages, Renderer->LinearDepthImage, Renderer->DepthImage);
	Renderer->DownscaleComputePass.UpdateAfterResize(Vulkan, Renderer->DescriptorPool, Renderer->LinearDepthImage, Renderer->DepthPyramidMipCount, Renderer->DepthPyramidMipViews, Renderer->PointEdgeSampler, Renderer->MaxReductionSampler);
	Renderer->DiffuseLightingPass.UpdateAfterResize(Vulkan, Renderer->PointEdgeSampler, Renderer->NormalsImage, Renderer->LinearDepthImage, Renderer->DiffuseLightHistoryImages, Renderer->DiffuseLightImage, Renderer->VelocityImages, Renderer->LinearEdgeSampler, Renderer->AlbedoImage, Renderer->DiffuseImage);
	Renderer->TransparentRenderPass.UpdateAfterResize(Vulkan, Renderer->DiffuseImage, Renderer->DepthImage);
	Renderer->SpecularLightingPass.UpdateAfterResize(Vulkan, Renderer->PointEdgeSampler, Renderer->NormalsImage, Renderer->LinearDepthImage, Renderer->LinearEdgeSampler, Renderer->MaterialImage, Renderer->DiffuseImage, Renderer->SpecularLightImage, Renderer->SpecularLightHistoryImages, Renderer->VelocityImages, Renderer->AlbedoImage, Renderer->CompositeImage);
	Renderer->FogRenderPass.UpdateAfterResize(Vulkan, Renderer->PointEdgeSampler, Renderer->LinearDepthImage, Renderer->DiffuseImage);
	Renderer->FirstPersonRenderPass.UpdateAfterResize(Vulkan, Renderer->CompositeImage, Renderer->VelocityImages);
	Renderer->ExposureRenderPass.UpdateAfterResize(Vulkan, Renderer->LinearEdgeSampler, Renderer->CompositeImage);
	Renderer->BloomRenderPass.UpdateAfterResize(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->BloomImage, Renderer->BloomMipViews, Renderer->LinearBorderZeroSampler);
	Renderer->TaaRenderPass.UpdateAfterResize(Vulkan, Renderer->LinearEdgeSampler, Renderer->CompositeImage, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->VelocityImages);
	Renderer->ToneMappingRenderPass.UpdateAfterResize(Vulkan, Renderer->PointEdgeSampler, Renderer->HistoryImages, Renderer->LinearEdgeSampler, Renderer->BloomImage, Renderer->FinalImage);
	Renderer->HudRenderPass.UpdateAfterResize(Vulkan, Renderer->FinalImage);
	Renderer->DebugRenderPass.UpdateAfterResize(Vulkan, Renderer->FinalImage);
}

void UpdateCameraRenderData(SRenderer* Renderer, const SCamera& Camera, uint32_t Width, uint32_t Height, uint32_t FrameID, uint32_t FrameInFlight)
{
	SCameraBuffer& CameraBufferData = Renderer->CameraBufferData;
	SBuffer& CameraBuffer = Renderer->CameraBuffers[FrameInFlight];

	float AspectRatio = float(Width) / float(Height);
	float NearHalfHeight = Camera.Near * tanf(0.5f*Radians(Camera.FoV));
	float NearHalfWidth = AspectRatio * NearHalfHeight;
	float FarHalfHeight = Camera.Far * tanf(0.5f*Radians(Camera.FoV));
	float FarHalfWidth = AspectRatio * FarHalfHeight;

	CameraBufferData.Pos = Vec4(Camera.Pos, -Camera.Near);

	CameraBufferData.PrevView = CameraBufferData.View;
	CameraBufferData.PrevProj = CameraBufferData.ProjUnjittered;

	CameraBufferData.View = LookAt(Camera.Pos, Camera.Pos + Camera.Dir, Camera.Up);

	vec2 CameraJitter = (2.0f * STaaRenderPass::GetJitter(FrameID)) / Vec2i(Width, Height);
	CameraBufferData.ProjUnjittered = Perspective(Camera.FoV, AspectRatio, Camera.Near, Camera.Far);
	CameraBufferData.Proj = CameraBufferData.ProjUnjittered;
	CameraBufferData.Proj.E[2 * 4 + 0] += CameraJitter.x;
	CameraBufferData.Proj.E[2 * 4 + 1] += CameraJitter.y;

	CameraBufferData.Viewport = Vec4(float(Width), float(Height), Camera.Near, Camera.Far);

	if (FrameID == 0)
	{
		CameraBufferData.PrevView = CameraBufferData.View;
		CameraBufferData.PrevProj = CameraBufferData.Proj;
	}

	memset(CameraBufferData.Frustums, 0, sizeof(CameraBufferData.Frustums));
	memset(CameraBufferData.FrustumCorners, 0, sizeof(CameraBufferData.FrustumCorners));
	{
		vec3 FrustumPoints[8] = {};
		FrustumPoints[0] = Camera.Pos + Camera.Near*Camera.Dir + NearHalfWidth*Camera.Right + NearHalfHeight*Camera.Up; // near right top
		FrustumPoints[1] = Camera.Pos + Camera.Near*Camera.Dir + NearHalfWidth*Camera.Right - NearHalfHeight*Camera.Up; // near right bot
		FrustumPoints[2] = Camera.Pos + Camera.Near*Camera.Dir - NearHalfWidth*Camera.Right + NearHalfHeight*Camera.Up; // near left top
		FrustumPoints[3] = Camera.Pos + Camera.Near*Camera.Dir - NearHalfWidth*Camera.Right - NearHalfHeight*Camera.Up; // near left bot
		FrustumPoints[4] = Camera.Pos + Camera.Far*Camera.Dir + FarHalfWidth*Camera.Right + FarHalfHeight*Camera.Up; // far right top
		FrustumPoints[5] = Camera.Pos + Camera.Far*Camera.Dir + FarHalfWidth*Camera.Right - FarHalfHeight*Camera.Up; // far right bot
		FrustumPoints[6] = Camera.Pos + Camera.Far*Camera.Dir - FarHalfWidth*Camera.Right + FarHalfHeight*Camera.Up; // far left top
		FrustumPoints[7] = Camera.Pos + Camera.Far*Camera.Dir - FarHalfWidth*Camera.Right - FarHalfHeight*Camera.Up; // far left bot

		vec3 FrustumPlaneNormals[6] = {};
		FrustumPlaneNormals[0] = Normalize(Cross(FrustumPoints[1] - FrustumPoints[0], FrustumPoints[2] - FrustumPoints[0])); // near
		FrustumPlaneNormals[1] = Normalize(Cross(FrustumPoints[6] - FrustumPoints[4], FrustumPoints[5] - FrustumPoints[4])); // far
		FrustumPlaneNormals[2] = Normalize(Cross(FrustumPoints[4] - FrustumPoints[0], FrustumPoints[1] - FrustumPoints[0])); // right
		FrustumPlaneNormals[3] = Normalize(Cross(FrustumPoints[3] - FrustumPoints[2], FrustumPoints[6] - FrustumPoints[2])); // left
		FrustumPlaneNormals[4] = Normalize(Cross(FrustumPoints[6] - FrustumPoints[2], FrustumPoints[0] - FrustumPoints[2])); // top
		FrustumPlaneNormals[5] = Normalize(Cross(FrustumPoints[5] - FrustumPoints[1], FrustumPoints[3] - FrustumPoints[1])); // bot

		CameraBufferData.Frustums[0] = Vec4(FrustumPlaneNormals[0], Dot(FrustumPlaneNormals[0], FrustumPoints[0])); // near
		CameraBufferData.Frustums[1] = Vec4(FrustumPlaneNormals[1], Dot(FrustumPlaneNormals[1], FrustumPoints[4])); // far
		CameraBufferData.Frustums[2] = Vec4(FrustumPlaneNormals[2], Dot(FrustumPlaneNormals[2], FrustumPoints[0])); // right
		CameraBufferData.Frustums[3] = Vec4(FrustumPlaneNormals[3], Dot(FrustumPlaneNormals[3], FrustumPoints[2])); // left
		CameraBufferData.Frustums[4] = Vec4(FrustumPlaneNormals[4], Dot(FrustumPlaneNormals[4], FrustumPoints[2])); // top
		CameraBufferData.Frustums[5] = Vec4(FrustumPlaneNormals[5], Dot(FrustumPlaneNormals[5], FrustumPoints[1])); // bot

		CameraBufferData.FrustumCorners[0] = Vec4(FrustumPoints[6] - Camera.Pos, 0.0f); // far left top
		CameraBufferData.FrustumCorners[1] = Vec4(FrustumPoints[7] - Camera.Pos, 0.0f); // far left bot
		CameraBufferData.FrustumCorners[2] = Vec4(FrustumPoints[4] - Camera.Pos, 0.0f); // far right top
		CameraBufferData.FrustumCorners[3] = Vec4(FrustumPoints[4] - Camera.Pos, 0.0f); // far right top
		CameraBufferData.FrustumCorners[4] = Vec4(FrustumPoints[7] - Camera.Pos, 0.0f); // far left bot
		CameraBufferData.FrustumCorners[5] = Vec4(FrustumPoints[5] - Camera.Pos, 0.0f); // far right bot
	}

	memcpy(CameraBuffer.Data, &CameraBufferData, sizeof(CameraBufferData));
}