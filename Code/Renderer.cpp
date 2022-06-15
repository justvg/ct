#include "Renderer.h"

void InitializeRenderer(SRenderer* Renderer, const SVulkanContext& Vulkan, const SGeometry* Geometry)
{
	for (uint32_t I = 0; I < ArrayCount(Renderer->StagingBuffers); I++)
	{
		Renderer->StagingBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(64), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	}
	Renderer->VertexBuffer = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(8), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->IndexBuffer = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(8), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->VoxelDrawBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SGameState::VoxelDraws), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->VoxelVisibilityBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SGameState::VoxelVisibilities), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->ParticleDrawBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SGameState::ParticleDraws), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer->IndirectBuffer = CreateBuffer(Vulkan.MemoryAllocator, Megabytes(64), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
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
	
	Renderer->HDRTargetImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, Vulkan.SampleCountMSAA);
	Renderer->DepthImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.DepthFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, Vulkan.SampleCountMSAA);
	Renderer->LinearDepthImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, Vulkan.SampleCountMSAA);
	Renderer->VelocityImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, Vulkan.SampleCountMSAA);
	
	Renderer->HDRTargetImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.DepthFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	Renderer->LinearDepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->VelocityImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	SDepthPyramidInfoResult DepthPyramidInfo = GetDepthPyramidInfo(Vulkan.Width, Vulkan.Height);
	Renderer->DepthPyramidImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, DepthPyramidInfo.Width, DepthPyramidInfo.Height, 0, DepthPyramidInfo.MipCount, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthPyramidMipCount = DepthPyramidInfo.MipCount;
	for (uint32_t I = 0; I < Renderer->DepthPyramidMipCount; I++)
	{
		Renderer->DepthPyramidMipViews[I] = CreateImageView(Vulkan.Device, Renderer->DepthPyramidImage.Image, Renderer->DepthPyramidImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
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
	
	Renderer->BloomImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Renderer->HDRTargetImage.Width >> 1, Renderer->HDRTargetImage.Height >> 1, 0, ArrayCount(Renderer->BloomMipViews), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->BloomMipViews); I++)
	{
		Renderer->BloomMipViews[I] = CreateImageView(Vulkan.Device, Renderer->BloomImage.Image, Renderer->BloomImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	for(uint32_t I = 0; I < ArrayCount(Renderer->HistoryImages); I++)
	{
		Renderer->HistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Renderer->FinalImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.SwapchainFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	for (uint32_t I = 0; I < FramesInFlight; I++)
	{
		Renderer->CameraBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SCameraBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Renderer->LightBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SLightBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Renderer->PointLightsBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, ArrayCount(SLevel::PointLights) * sizeof(SPointLight), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		Renderer->HUDProjectionBuffers[I] = CreateBuffer(Vulkan.MemoryAllocator, sizeof(SHUDProjectionBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
	Renderer->CullingVoxComputePass = SVoxelCullingComputePass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelDrawBuffer, Renderer->IndirectBuffer, Renderer->CountBuffer, Renderer->VoxelVisibilityBuffer, Renderer->MaxReductionSampler, Renderer->DepthPyramidImage);
	Renderer->ForwardVoxRenderPass = SForwardVoxelRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelDrawBuffer, Renderer->VoxelsBuffer, Renderer->PointRepeatSampler, Renderer->BlueNoiseTexture, Renderer->PointLightsBuffers, Renderer->LightBuffers, Renderer->HDRTargetImageMSAA, Renderer->LinearDepthImageMSAA, Renderer->VelocityImageMSAA, Renderer->DepthImageMSAA);
	Renderer->ForwardRenderPass = SForwardRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->VoxelsBuffer, Renderer->PointRepeatSampler, Renderer->BlueNoiseTexture, Renderer->PointLightsBuffers, Renderer->LightBuffers, Renderer->HDRTargetImageMSAA, Renderer->LinearDepthImageMSAA, Renderer->VelocityImageMSAA, Renderer->DepthImageMSAA);
	Renderer->ForwardPartRenderPass = SForwardParticleRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->ParticleDrawBuffer, Renderer->VoxelsBuffer, Renderer->PointRepeatSampler, Renderer->BlueNoiseTexture, Renderer->PointLightsBuffers, Renderer->LightBuffers, Renderer->HDRTargetImageMSAA, Renderer->LinearDepthImageMSAA, Renderer->VelocityImageMSAA, Renderer->DepthImageMSAA);
	Renderer->DownscaleComputePass = SDownscaleComputePass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearDepthImage, Renderer->DepthPyramidMipCount, Renderer->DepthPyramidMipViews, Renderer->PointEdgeSampler, Renderer->MaxReductionSampler);
	Renderer->ExposureRenderPass = SExposureRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->HDRTargetImage, Renderer->LinearEdgeSamplerMips, Renderer->BrightnessImage, Renderer->BrightnessMipViews, Renderer->PointEdgeSampler, Renderer->ExposureImages);
	Renderer->BloomRenderPass = SBloomRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->BloomImage, Renderer->BloomMipViews, Renderer->LinearBorderZeroSampler);
	Renderer->TaaRenderPass = STaaRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->HDRTargetImage, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->VelocityImage);
	Renderer->ToneMappingRenderPass = SToneMappingRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->PointEdgeSampler, Renderer->HistoryImages, Renderer->ExposureImages, Renderer->LinearEdgeSampler, Renderer->BloomImage, Renderer->FinalImage);
	Renderer->HudRenderPass = SHUDRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->HUDProjectionBuffers, Renderer->FinalImage);
	Renderer->DebugRenderPass = SDebugRenderPass::Create(Vulkan, Renderer->DescriptorPool, Renderer->CameraBuffers, Renderer->FinalImage);
	
	Renderer->AimTexture = LoadTexture(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->StagingBuffers[0], Vulkan.MemoryAllocator, "Textures\\aim_texture.bmp");
	Renderer->AimTextureDescrSet = CreateDescriptorSet(Vulkan.Device, Renderer->DescriptorPool, Renderer->HudRenderPass.GetTextureDescrSetLayout());
	UpdateDescriptorSetImage(Vulkan.Device, Renderer->AimTextureDescrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Renderer->LinearEdgeSampler, Renderer->AimTexture.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	Renderer->Font = LoadFont(Vulkan.Device, Renderer->DescriptorPool, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->StagingBuffers[0], Vulkan.MemoryAllocator, Renderer->HudRenderPass.GetTextureDescrSetLayout());


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

VkImage RenderScene(SGameState* GameState, SRenderer* Renderer, const SVulkanContext& Vulkan, SLevel* Level, uint32_t PointLightCount, uint32_t TotalParticleCount, uint32_t FrameID, STempMemoryArena* MemoryArena)
{
	// Rendering
	BEGIN_PROFILER_BLOCK("RENDERING");
    
	// Set viewport
	VkViewport Viewport = { 0.0f, float(Vulkan.Height), float(Vulkan.Width), -float(Vulkan.Height), 0.0f, 1.0f };
	VkRect2D Scissor = { {0, 0}, {Vulkan.Width, Vulkan.Height} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);
    
	BEGIN_GPU_PROFILER_BLOCK("RENDER", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	// Frustum cull voxels visible last frame
	Renderer->CullingVoxComputePass.Dispatch(Vulkan, Renderer->CountBuffer, Renderer->DepthPyramidImage, ArrayCount(GameState->VoxelDraws), FrameID, false);
    
	// Forward render voxels visibile last frame and frustum culled in this frame
	Renderer->ForwardVoxRenderPass.RenderEarly(Vulkan, Renderer->VertexBuffer, Renderer->IndexBuffer, Renderer->IndirectBuffer, Renderer->CountBuffer, ArrayCount(GameState->VoxelDraws), PointLightCount, Level->AmbientColor, FrameID);
    
	BEGIN_GPU_PROFILER_BLOCK("RESOLVE_LINEAR_DEPTH", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	VkImageMemoryBarrier LinearDepthResolveBarriers[] = 
	{
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->LinearDepthImageMSAA.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->LinearDepthImage.Image, VK_IMAGE_ASPECT_COLOR_BIT)
	};
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(LinearDepthResolveBarriers), LinearDepthResolveBarriers);
	
	VkImageResolve LinearDepthResolve = {};
	LinearDepthResolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	LinearDepthResolve.srcSubresource.layerCount = 1; 
    LinearDepthResolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    LinearDepthResolve.dstSubresource.layerCount = 1;
    LinearDepthResolve.extent = { Renderer->LinearDepthImage.Width, Renderer->LinearDepthImage.Height, 1 };
	vkCmdResolveImage(Vulkan.CommandBuffer, Renderer->LinearDepthImageMSAA.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->LinearDepthImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &LinearDepthResolve);
	
	END_GPU_PROFILER_BLOCK("RESOLVE_LINEAR_DEPTH", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	// Downscale linear depth buffer
	Renderer->DownscaleComputePass.Dispatch(Vulkan, Renderer->LinearDepthImage, Renderer->DepthPyramidImage, Renderer->DepthPyramidMipCount);
    
	// Frustum and occlusion cull all voxels
	Renderer->CullingVoxComputePass.Dispatch(Vulkan, Renderer->CountBuffer, Renderer->DepthPyramidImage, ArrayCount(GameState->VoxelDraws), FrameID, true);
    
	// Render voxels visible this frame that are not already rendered
	Renderer->ForwardVoxRenderPass.RenderLate(Vulkan, Renderer->VertexBuffer, Renderer->IndexBuffer, Renderer->IndirectBuffer, Renderer->CountBuffer, ArrayCount(GameState->VoxelDraws), PointLightCount, FrameID);
    
	// Forward render particles
	Renderer->ForwardPartRenderPass.Render(Vulkan, TotalParticleCount, Renderer->CubeVB, Renderer->CubeIB, PointLightCount, FrameID);

	// Forward render entities
	if (!GameState->bHideEntities)
	{
		Renderer->ForwardRenderPass.Render(Vulkan, Level->Entities, Level->EntityCount, GameState->Camera, GameState->Geometry, Renderer->VertexBuffer, Renderer->IndexBuffer, PointLightCount, FrameID, GameState->GameMode == GameMode_Game, MemoryArena);
	}
    
	BEGIN_GPU_PROFILER_BLOCK("RESOLVE_MSAA_TARGETS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	VkImageMemoryBarrier MSAAResolveBarriers[] = 
	{
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->HDRTargetImageMSAA.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->LinearDepthImageMSAA.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->VelocityImageMSAA.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->HDRTargetImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Renderer->VelocityImage.Image, VK_IMAGE_ASPECT_COLOR_BIT)
	};
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(MSAAResolveBarriers), MSAAResolveBarriers);
    
	VkImageResolve HDRTargetResolve = {};
	HDRTargetResolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	HDRTargetResolve.srcSubresource.layerCount = 1; 
    HDRTargetResolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    HDRTargetResolve.dstSubresource.layerCount = 1;
    HDRTargetResolve.extent = { Renderer->HDRTargetImage.Width, Renderer->HDRTargetImage.Height, 1 };
	vkCmdResolveImage(Vulkan.CommandBuffer, Renderer->HDRTargetImageMSAA.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->HDRTargetImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &HDRTargetResolve);
    
	vkCmdResolveImage(Vulkan.CommandBuffer, Renderer->LinearDepthImageMSAA.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->LinearDepthImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &LinearDepthResolve);
    
	VkImageResolve VelocityResolve = {};
	VelocityResolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VelocityResolve.srcSubresource.layerCount = 1; 
    VelocityResolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VelocityResolve.dstSubresource.layerCount = 1;
    VelocityResolve.extent = { Renderer->VelocityImage.Width, Renderer->VelocityImage.Height, 1 };
	vkCmdResolveImage(Vulkan.CommandBuffer, Renderer->VelocityImageMSAA.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->VelocityImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &VelocityResolve);
    
	VkImageMemoryBarrier HDRTargetReadBarrier = CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->HDRTargetImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &HDRTargetReadBarrier);
    
	END_GPU_PROFILER_BLOCK("RESOLVE_MSAA_TARGETS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	// Get scene exposure
	Renderer->ExposureRenderPass.Render(Vulkan, Renderer->QuadVB, Renderer->BrightnessImage, Renderer->ExposureImages, FrameID);
    
	// TAA
	VkImageMemoryBarrier VelocityReadBarrier = CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Renderer->VelocityImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &VelocityReadBarrier);
    
	Renderer->TaaRenderPass.Render(Vulkan, Renderer->HistoryImages, Renderer->QuadVB, FrameID);
    
	// Calculate bloom
	Renderer->BloomRenderPass.Render(Vulkan, Renderer->QuadVB, Renderer->BloomImage, FrameID);
    
	// Tone mapping
	Renderer->ToneMappingRenderPass.Render(Vulkan, Renderer->QuadVB, FrameID);
    
	// HUD
	SHUDProjectionBuffer HUDProjectionData = { Orthographic(-0.5f*Vulkan.Width, 0.5f*Vulkan.Width, -0.5f*Vulkan.Height, 0.5f*Vulkan.Height, -1.0f, 1.0f)  };
	memcpy(Renderer->HUDProjectionBuffers[Vulkan.FrameInFlight].Data, &HUDProjectionData, sizeof(SHUDProjectionBuffer));
    
	Renderer->HudRenderPass.BeginRender(Vulkan, Renderer->QuadVB);
	
	if (GameState->GameMode == GameMode_Game)
	{
		Renderer->HudRenderPass.Render(Vulkan, Renderer->AimTextureDescrSet, Vec2(0.0f, 0.0f), Vec2i(Renderer->AimTexture.Width, Renderer->AimTexture.Height));
	}

	for (uint32_t I = 0; I < GameState->TextsToRenderCount; I++)
	{
		const SText& Text = GameState->TextsToRender[I];

		vec2 TextSize = GetTextSize(Renderer->Font, Text.Scale, Text.String);
		vec2 ScreenPosition = Hadamard(Text.Pos, 0.5f * Vec2i(Vulkan.Width, Vulkan.Height)) - 0.5f * TextSize;
		if (Text.bAppearance)
		{
			Renderer->HudRenderPass.RenderStringWithAppearance(Vulkan, Renderer->Font, ScreenPosition, Text.Scale, Text.String, Clamp(Text.CurrentTime /  Text.TimeToAppear, 0.0f, 1.0f));
		}
		else
		{
			Renderer->HudRenderPass.RenderString(Vulkan, Renderer->Font, ScreenPosition, Text.Scale, Text.String);
		}
	}
	
	Renderer->HudRenderPass.EndRender(Vulkan);
    
	Renderer->DebugRenderPass.Render(Vulkan);
    
#ifndef ENGINE_RELEASE
	// DearImgui
	RenderDearImgui(GameState, &Vulkan, Renderer->HudRenderPass.GetFramebuffer());
#endif
    
	VkImageMemoryBarrier FinalRenderBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Renderer->FinalImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &FinalRenderBarrier);
    
	END_GPU_PROFILER_BLOCK("RENDER", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	END_PROFILER_BLOCK("RENDERING");
    
	return Renderer->FinalImage.Image;
}

void RendererSwapchainResized(SRenderer* Renderer, const SVulkanContext& Vulkan)
{
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->HDRTargetImageMSAA);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DepthImageMSAA);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->LinearDepthImageMSAA);		
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->VelocityImageMSAA);		
	
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->HDRTargetImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DepthImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->LinearDepthImage);
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->VelocityImage);
	
	for (uint32_t I = 0; I < Renderer->DepthPyramidMipCount; I++)
	{
		vkDestroyImageView(Vulkan.Device, Renderer->DepthPyramidMipViews[I], 0);
	}
	DestroyImage(Vulkan.Device, Vulkan.MemoryAllocator, Renderer->DepthPyramidImage);
	
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
	
	Renderer->HDRTargetImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, Vulkan.SampleCountMSAA);
	Renderer->DepthImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.DepthFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, Vulkan.SampleCountMSAA);
	Renderer->LinearDepthImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, Vulkan.SampleCountMSAA);
	Renderer->VelocityImageMSAA = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, Vulkan.SampleCountMSAA);
	
	Renderer->HDRTargetImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.DepthFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	Renderer->LinearDepthImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->VelocityImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	SDepthPyramidInfoResult DepthPyramidInfo = GetDepthPyramidInfo(Vulkan.Width, Vulkan.Height);
	Renderer->DepthPyramidImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R32_SFLOAT, DepthPyramidInfo.Width, DepthPyramidInfo.Height, 0, DepthPyramidInfo.MipCount, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	Renderer->DepthPyramidMipCount = DepthPyramidInfo.MipCount;
	for (uint32_t I = 0; I < Renderer->DepthPyramidMipCount; I++)
	{
		Renderer->DepthPyramidMipViews[I] = CreateImageView(Vulkan.Device, Renderer->DepthPyramidImage.Image, Renderer->DepthPyramidImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Renderer->BloomImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Renderer->HDRTargetImage.Width >> 1, Renderer->HDRTargetImage.Height >> 1, 0, ArrayCount(Renderer->BloomMipViews), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32_t I = 0; I < ArrayCount(Renderer->BloomMipViews); I++)
	{
		Renderer->BloomMipViews[I] = CreateImageView(Vulkan.Device, Renderer->BloomImage.Image, Renderer->BloomImage.Format, I, 1, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	for(uint32_t I = 0; I < ArrayCount(Renderer->HistoryImages); I++)
	{
		Renderer->HistoryImages[I] = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, VK_FORMAT_R16G16B16A16_SFLOAT, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	
	Renderer->FinalImage = CreateImage(Vulkan.Device, Vulkan.MemoryAllocator, Vulkan.SwapchainFormat, Vulkan.Width, Vulkan.Height, 0, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	
	Renderer->CullingVoxComputePass.UpdateAfterResize(Vulkan, Renderer->MaxReductionSampler, Renderer->DepthPyramidImage);
	Renderer->ForwardVoxRenderPass.UpdateAfterResize(Vulkan, Renderer->HDRTargetImageMSAA, Renderer->LinearDepthImageMSAA, Renderer->VelocityImageMSAA, Renderer->DepthImageMSAA);
	Renderer->ForwardRenderPass.UpdateAfterResize(Vulkan, Renderer->HDRTargetImageMSAA, Renderer->LinearDepthImageMSAA, Renderer->VelocityImageMSAA, Renderer->DepthImageMSAA);
	Renderer->ForwardPartRenderPass.UpdateAfterResize(Vulkan, Renderer->HDRTargetImageMSAA, Renderer->LinearDepthImageMSAA, Renderer->VelocityImageMSAA, Renderer->DepthImageMSAA);
	Renderer->DownscaleComputePass.UpdateAfterResize(Vulkan, Renderer->DescriptorPool, Renderer->LinearDepthImage, Renderer->DepthPyramidMipCount, Renderer->DepthPyramidMipViews, Renderer->PointEdgeSampler, Renderer->MaxReductionSampler);
	Renderer->ExposureRenderPass.UpdateAfterResize(Vulkan, Renderer->LinearEdgeSampler, Renderer->HDRTargetImage);
	Renderer->BloomRenderPass.UpdateAfterResize(Vulkan, Renderer->DescriptorPool, Renderer->LinearEdgeSampler, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->BloomImage, Renderer->BloomMipViews, Renderer->LinearBorderZeroSampler);
	Renderer->TaaRenderPass.UpdateAfterResize(Vulkan, Renderer->LinearEdgeSampler, Renderer->HDRTargetImage, Renderer->HistoryImages, Renderer->PointEdgeSampler, Renderer->VelocityImage);
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
	CameraBufferData.PrevProj = CameraBufferData.Proj;
	CameraBufferData.PrevProj.E[2 * 4 + 0] = 0.0f;
	CameraBufferData.PrevProj.E[2 * 4 + 1] = 0.0f;

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
	}

	memcpy(CameraBuffer.Data, &CameraBufferData, sizeof(CameraBufferData));
}