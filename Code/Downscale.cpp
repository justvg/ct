struct SDownscaleComputePass
{
public:
    static SDownscaleComputePass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SImage& LinearDepthImage, uint32_t DepthPyramidMipCount, const VkImageView* DepthPyramidMipViews, VkSampler PointSampler, VkSampler MaxReductionSampler);
    void Dispatch(const SVulkanContext& Vulkan, const SImage& LinearDepthImage, const SImage& DepthPyramidImage, uint32_t MipCount);
    void UpdateAfterResize(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SImage& LinearDepthImage, uint32_t DepthPyramidMipCount, const VkImageView* DepthPyramidMipViews, VkSampler PointSampler, VkSampler MaxReductionSampler);

private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkDescriptorSetLayout DescrSetLayout;

	uint32_t DescrSetCount;
    VkDescriptorSet DescrSets[16];

    VkShaderModule Shader;
};

struct SDownscalePushConstants
{
    vec2 SrcImageSize;
    vec2 DstImageSize;
    uint32_t MinMaxSampler;
};

SDownscaleComputePass SDownscaleComputePass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SImage& LinearDepthImage, uint32_t DepthPyramidMipCount, const VkImageView* DepthPyramidMipViews, VkSampler PointSampler, VkSampler MaxReductionSampler)
{
    VkDescriptorSetLayoutBinding OutBinging = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding InBinging = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { OutBinging, InBinging };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[ArrayCount(SDownscaleComputePass::DescrSets)] = {};
    for (uint32_t I = 0; I < DepthPyramidMipCount; I++)
    {
        DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);

        VkImageView SrcView = (I == 0) ? LinearDepthImage.View : DepthPyramidMipViews[I - 1];
        VkSampler SrcSampler = (I == 0) ? PointSampler : MaxReductionSampler;
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MaxReductionSampler, DepthPyramidMipViews[I], VK_IMAGE_LAYOUT_GENERAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SrcSampler, SrcView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);            
    }

    VkShaderModule Shader = LoadShader(Vulkan.Device, "Shaders\\Downscale.comp.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SDownscalePushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipeline Pipeline = CreateComputePipeline(Vulkan.Device, PipelineLayout, Shader);

    SDownscaleComputePass DownscalePass = {};
    DownscalePass.Pipeline = Pipeline;
    DownscalePass.PipelineLayout = PipelineLayout;
    DownscalePass.DescrSetLayout = DescrSetLayout;
    DownscalePass.Shader = Shader;
    DownscalePass.DescrSetCount = DepthPyramidMipCount;
    memcpy(DownscalePass.DescrSets, DescrSets, sizeof(DownscalePass.DescrSets));

    return DownscalePass;
}

void SDownscaleComputePass::Dispatch(const SVulkanContext& Vulkan, const SImage& LinearDepthImage, const SImage& DepthPyramidImage, uint32_t MipCount)
{
	BEGIN_GPU_PROFILER_BLOCK("DOWNSCALE_DEPTH", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

    VkImageMemoryBarrier DownscaleDepthBarriers[] =
	{
		CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, LinearDepthImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
		CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, DepthPyramidImage.Image, VK_IMAGE_ASPECT_COLOR_BIT),
	};
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(DownscaleDepthBarriers), DownscaleDepthBarriers);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
	for (uint32_t I = 0; I < MipCount; I++)
	{
		vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout, 0, 1, &DescrSets[I], 0, 0);

		vec2 SrcImageSize = (I == 0) ? Vec2i(LinearDepthImage.Width, LinearDepthImage.Height) : Vec2i(Max(DepthPyramidImage.Width >> (I - 1), 1u), Max(DepthPyramidImage.Height >> (I - 1), 1u));
		vec2 DstImageSize = Vec2i(Max(DepthPyramidImage.Width >> I, 1u), Max(DepthPyramidImage.Height >> I, 1u));
        SDownscalePushConstants PushConstants = { SrcImageSize, DstImageSize, (I == 0) || Vulkan.bMinMaxSampler ? 1u : 0u };
		vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SDownscalePushConstants), &PushConstants);

		vkCmdDispatch(Vulkan.CommandBuffer, ((uint32_t)DstImageSize.x + 31) / 32, ((uint32_t)DstImageSize.y + 31) / 32, 1);

		VkImageMemoryBarrier MipDownscaleDepthBarrier = CreateImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DepthPyramidImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, I, 1);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &MipDownscaleDepthBarrier);
	}

    VkImageMemoryBarrier LinearDepthBarrier = CreateImageMemoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, LinearDepthImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &LinearDepthBarrier);

	END_GPU_PROFILER_BLOCK("DOWNSCALE_DEPTH", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SDownscaleComputePass::UpdateAfterResize(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SImage& LinearDepthImage, uint32_t DepthPyramidMipCount, const VkImageView* DepthPyramidMipViews, VkSampler PointSampler, VkSampler MaxReductionSampler)
{
	if (DescrSetCount < DepthPyramidMipCount)
	{
		for (uint32_t I = DescrSetCount; I < DepthPyramidMipCount; I++)
		{
			DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
		}

		DescrSetCount = DepthPyramidMipCount;
	}

    for (uint32_t I = 0; I < DepthPyramidMipCount; I++)
    {
        VkImageView SrcView = (I == 0) ? LinearDepthImage.View : DepthPyramidMipViews[I - 1];
        VkSampler SrcSampler = (I == 0) ? PointSampler : MaxReductionSampler;
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MaxReductionSampler, DepthPyramidMipViews[I], VK_IMAGE_LAYOUT_GENERAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SrcSampler, SrcView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);            
    }
}