struct SBloomRenderPass
{
public:
	static const uint32_t BloomMipsCount = 7;

public:
    static SBloomRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage& BloomImage, VkImageView* BloomMipViews, VkSampler LinearBorderZeroSampler);
    void Render(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer, const SImage& BloomImage, uint32_t FrameID);
	void UpdateAfterResize(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage& BloomImage, VkImageView* BloomMipViews, VkSampler LinearBorderZeroSampler);

private:
	// Bloom brightness stuff
    VkPipeline BrightnessPipeline;
    VkPipelineLayout BrightnessPipelineLayout;

    VkRenderPass BrightnessRenderPass;
    VkFramebuffer BrightnessFramebuffer;

    VkDescriptorSetLayout BrightnessDescrSetLayout;
    VkDescriptorSet BrightnessDescrSets[2];

    VkShaderModule BrightnessVShader;
    VkShaderModule BrightnessFShader;

    // Downscale stuff
	VkPipeline CompPipeline;
    VkPipelineLayout CompPipelineLayout;

    VkDescriptorSetLayout CompDescrSetLayout;
    VkDescriptorSet CompDescrSets[BloomMipsCount - 1];

    VkShaderModule CompShader;

	// Bloom stuff
	VkPipeline BloomPipeline;
    VkPipelineLayout BloomPipelineLayout;

    VkRenderPass BloomRenderPass;
    VkFramebuffer BloomFramebuffers[BloomMipsCount - 1];

    VkDescriptorSetLayout BloomDescrSetLayout;
    VkDescriptorSet BloomDescrSets[BloomMipsCount - 1];

    VkShaderModule BloomVShader;
    VkShaderModule BloomFShader;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);

    static VkRenderPass CreateBloomRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateBloomGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

struct SBloomDownscalePushConstants
{
	vec2 ImageSizeSrc;
    vec2 ImageSizeDst;
};

SBloomRenderPass SBloomRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage& BloomImage, VkImageView* BloomMipViews, VkSampler LinearBorderZeroSampler)
{
	// Bloom brightness stuff
    VkDescriptorSetLayoutBinding InputBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding BrightDescrSetLayoutBindings[] = { InputBinding };
    VkDescriptorSetLayout BrightnessDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(BrightDescrSetLayoutBindings), BrightDescrSetLayoutBindings);

    VkDescriptorSet BrightnessDescrSets[2] = {};
    for (uint32_t I = 0; I < ArrayCount(BrightnessDescrSets); I++)
    {
        BrightnessDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, BrightnessDescrSetLayout);
        
        UpdateDescriptorSetImage(Vulkan.Device, BrightnessDescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, HistoryImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkRenderPass BrightnessRenderPass = CreateRenderPass(Vulkan.Device, BloomImage.Format);

    VkFramebuffer BrightnessFramebuffer = CreateFramebuffer(Vulkan.Device, BrightnessRenderPass, &BloomMipViews[0], 1, BloomImage.Width, BloomImage.Height);

    VkShaderModule BrightnessVShader = LoadShader(Vulkan.Device, "Shaders\\Fullscreen.vert.spv");
    VkShaderModule BrightnessFShader = LoadShader(Vulkan.Device, "Shaders\\BloomBrightness.frag.spv");

    VkPipelineLayout BrightnessPipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &BrightnessDescrSetLayout);
    VkPipeline BrightnessPipeline = CreateGraphicsPipeline(Vulkan.Device, BrightnessRenderPass, BrightnessPipelineLayout, BrightnessVShader, BrightnessFShader);

    // Downscale stuff
    VkDescriptorSetLayoutBinding OutBinging = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding InBinging = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

    VkDescriptorSetLayoutBinding CompDescrSetLayoutBindings[] = { OutBinging, InBinging };
    VkDescriptorSetLayout CompDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(CompDescrSetLayoutBindings), CompDescrSetLayoutBindings);

    VkDescriptorSet CompDescrSets[ArrayCount(SBloomRenderPass::CompDescrSets)] = {};
    for (uint32_t I = 0; I < ArrayCount(CompDescrSets); I++)
    {
        CompDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, CompDescrSetLayout);

        UpdateDescriptorSetImage(Vulkan.Device, CompDescrSets[I], 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, PointEdgeSampler, BloomMipViews[I + 1], VK_IMAGE_LAYOUT_GENERAL);
        UpdateDescriptorSetImage(Vulkan.Device, CompDescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearBorderZeroSampler, BloomMipViews[I], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkShaderModule CompShader = LoadShader(Vulkan.Device, "Shaders\\BloomDownscale.comp.spv");

    VkPipelineLayout CompPipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &CompDescrSetLayout, sizeof(SBloomDownscalePushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipeline CompPipeline = CreateComputePipeline(Vulkan.Device, CompPipelineLayout, CompShader);

	// Bloom stuff
    VkDescriptorSetLayoutBinding BloomInputBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding BloomDescrSetLayoutBindings[] = { BloomInputBinding };
    VkDescriptorSetLayout BloomDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(BloomDescrSetLayoutBindings), BloomDescrSetLayoutBindings);

    VkDescriptorSet BloomDescrSets[ArrayCount(SBloomRenderPass::BloomDescrSets)] = {};
    for (uint32_t I = 0; I < ArrayCount(BloomDescrSets); I++)
    {
        BloomDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, BloomDescrSetLayout);
        
        UpdateDescriptorSetImage(Vulkan.Device, BloomDescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearBorderZeroSampler, BloomMipViews[ArrayCount(BloomDescrSets) - I], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkRenderPass BloomRenderPass = CreateBloomRenderPass(Vulkan.Device, BloomImage.Format);

    VkFramebuffer BloomFramebuffers[ArrayCount(SBloomRenderPass::BloomFramebuffers)] = {};
	for (uint32_t I = 0; I < ArrayCount(BloomFramebuffers); I++)
	{
		uint32_t MipLevel = ArrayCount(BloomFramebuffers) - I - 1;
		BloomFramebuffers[I] = CreateFramebuffer(Vulkan.Device, BloomRenderPass, &BloomMipViews[MipLevel], 1, Max(BloomImage.Width >> MipLevel, 1), Max(BloomImage.Height >> MipLevel, 1));
	}

    VkShaderModule BloomVShader = LoadShader(Vulkan.Device, "Shaders\\Fullscreen.vert.spv");
    VkShaderModule BloomFShader = LoadShader(Vulkan.Device, "Shaders\\BloomUpscale.frag.spv");

    VkPipelineLayout BloomPipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &BloomDescrSetLayout, sizeof(SBloomDownscalePushConstants), VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline BloomPipeline = CreateBloomGraphicsPipeline(Vulkan.Device, BloomRenderPass, BloomPipelineLayout, BloomVShader, BloomFShader);

    SBloomRenderPass BloomPass = {};
    BloomPass.BrightnessPipeline = BrightnessPipeline;
    BloomPass.BrightnessPipelineLayout = BrightnessPipelineLayout;
    BloomPass.BrightnessRenderPass = BrightnessRenderPass;
    BloomPass.BrightnessFramebuffer = BrightnessFramebuffer;
    BloomPass.BrightnessDescrSetLayout = BrightnessDescrSetLayout;
    BloomPass.BrightnessVShader = BrightnessVShader;
    BloomPass.BrightnessFShader = BrightnessFShader;
    memcpy(BloomPass.BrightnessDescrSets, BrightnessDescrSets, sizeof(BloomPass.BrightnessDescrSets));

    BloomPass.CompPipeline = CompPipeline;
    BloomPass.CompPipelineLayout = CompPipelineLayout;
    BloomPass.CompDescrSetLayout = CompDescrSetLayout;
    BloomPass.CompShader = CompShader;
    memcpy(BloomPass.CompDescrSets, CompDescrSets, sizeof(BloomPass.CompDescrSets));
	
	BloomPass.BloomPipeline = BloomPipeline;
    BloomPass.BloomPipelineLayout = BloomPipelineLayout;
    BloomPass.BloomRenderPass = BloomRenderPass;
    BloomPass.BloomDescrSetLayout = BloomDescrSetLayout;
    BloomPass.BloomVShader = BloomVShader;
    BloomPass.BloomFShader = BloomFShader;
    memcpy(BloomPass.BloomDescrSets, BloomDescrSets, sizeof(BloomPass.BloomDescrSets));
    memcpy(BloomPass.BloomFramebuffers, BloomFramebuffers, sizeof(BloomPass.BloomFramebuffers));

    return BloomPass;
}

void SBloomRenderPass::Render(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer, const SImage& BloomImage, uint32_t FrameID)
{
	BEGIN_GPU_PROFILER_BLOCK("BLOOM", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Bloom brightness stuff
	BEGIN_GPU_PROFILER_BLOCK("BLOOM_BRIGHTNESS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkViewport Viewport = { 0.0f, float(BloomImage.Height), float(BloomImage.Width), -float(BloomImage.Height), 0.0f, 1.0f };
	VkRect2D Scissor = { {0, 0}, {BloomImage.Width, BloomImage.Height} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

    VkRenderPassBeginInfo BrightRenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BrightRenderPassBeginInfo.renderPass = BrightnessRenderPass;
	BrightRenderPassBeginInfo.framebuffer = BrightnessFramebuffer;
	BrightRenderPassBeginInfo.renderArea.extent.width = BloomImage.Width;
	BrightRenderPassBeginInfo.renderArea.extent.height = BloomImage.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &BrightRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BrightnessPipeline);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BrightnessPipelineLayout, 0, 1, &BrightnessDescrSets[FrameID % 2], 0, 0);

    VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("BLOOM_BRIGHTNESS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Downscale stuff
	BEGIN_GPU_PROFILER_BLOCK("DOWNSCALE_BLOOM", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CompPipeline);

	VkImageMemoryBarrier DownscaleStartBarrier[] = 
	{
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, BloomImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1),
		CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, BloomImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, 1, BloomMipsCount - 1),
	};
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(DownscaleStartBarrier), DownscaleStartBarrier);
	
	for (uint32_t I = 0; I < ArrayCount(CompDescrSets); I++)
	{
		vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CompPipelineLayout, 0, 1, &CompDescrSets[I], 0, 0);

		vec2 ImageSizeSrc = Vec2i(Max(BloomImage.Width >> I, 1u), Max(BloomImage.Height >> I, 1u));
		vec2 ImageSizeDst = Vec2i(Max(BloomImage.Width >> (I + 1), 1u), Max(BloomImage.Height >> (I + 1), 1u));
        SBloomDownscalePushConstants PushConstants = { ImageSizeSrc, ImageSizeDst };
		vkCmdPushConstants(Vulkan.CommandBuffer, CompPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SBloomDownscalePushConstants), &PushConstants);

		vkCmdDispatch(Vulkan.CommandBuffer, ((uint32_t)ImageSizeDst.x + 31) / 32, ((uint32_t)ImageSizeDst.y + 31) / 32, 1);

		VkImageMemoryBarrier DownscaleBarrier = CreateImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, BloomImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, I + 1, 1);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &DownscaleBarrier);
	}

	END_GPU_PROFILER_BLOCK("DOWNSCALE_BLOOM", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Bloom stuff
	BEGIN_GPU_PROFILER_BLOCK("CALCULATE_BLOOM", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPipeline);

	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	for (uint32_t I = 0; I < ArrayCount(BloomDescrSets); I++)
	{
		uint32_t MipSourceLevel = ArrayCount(BloomDescrSets) - I;
		uint32_t SourceWidth = Max(BloomImage.Width >> MipSourceLevel, 1);
		uint32_t SourceHeight = Max(BloomImage.Height >> MipSourceLevel, 1);		

		uint32_t MipTargetLevel = ArrayCount(BloomDescrSets) - I - 1;
		uint32_t TargetWidth = Max(BloomImage.Width >> MipTargetLevel, 1);
		uint32_t TargetHeight = Max(BloomImage.Height >> MipTargetLevel, 1);

		Viewport = { 0.0f, float(TargetHeight), float(TargetWidth), -float(TargetHeight), 0.0f, 1.0f };
		Scissor = { {0, 0}, {TargetWidth, TargetHeight} };
		vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
		vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

		VkRenderPassBeginInfo BloomRenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		BloomRenderPassBeginInfo.renderPass = BloomRenderPass;
		BloomRenderPassBeginInfo.framebuffer = BloomFramebuffers[I];
		BloomRenderPassBeginInfo.renderArea.extent.width = TargetWidth;
		BloomRenderPassBeginInfo.renderArea.extent.height = TargetHeight;
		vkCmdBeginRenderPass(Vulkan.CommandBuffer, &BloomRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		SBloomDownscalePushConstants PushConstants = { Vec2i(SourceWidth, SourceHeight), Vec2i(TargetWidth, TargetHeight) };
		vkCmdPushConstants(Vulkan.CommandBuffer, BloomPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SBloomDownscalePushConstants), &PushConstants);

		vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPipelineLayout, 0, 1, &BloomDescrSets[I], 0, 0);
		vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

		vkCmdEndRenderPass(Vulkan.CommandBuffer);

		VkImageMemoryBarrier UpscaleBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, BloomImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, MipTargetLevel, 1);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &UpscaleBarrier);
	}

	END_GPU_PROFILER_BLOCK("CALCULATE_BLOOM", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Set viewport back to fullres
	Viewport = { 0.0f, float(Vulkan.Height), float(Vulkan.Width), -float(Vulkan.Height), 0.0f, 1.0f };
	Scissor = { {0, 0}, {Vulkan.Width, Vulkan.Height} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

	END_GPU_PROFILER_BLOCK("BLOOM", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SBloomRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage& BloomImage, VkImageView* BloomMipViews, VkSampler LinearBorderZeroSampler)
{
	// Bloom brightness stuff
    for (uint32_t I = 0; I < ArrayCount(BrightnessDescrSets); I++)
    {
        UpdateDescriptorSetImage(Vulkan.Device, BrightnessDescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, HistoryImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    vkDestroyFramebuffer(Vulkan.Device, BrightnessFramebuffer, 0);
    BrightnessFramebuffer = CreateFramebuffer(Vulkan.Device, BrightnessRenderPass, &BloomMipViews[0], 1, BloomImage.Width, BloomImage.Height);

	// Downscale stuff
    for (uint32_t I = 0; I < ArrayCount(CompDescrSets); I++)
    {
		UpdateDescriptorSetImage(Vulkan.Device, CompDescrSets[I], 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, PointEdgeSampler, BloomMipViews[I + 1], VK_IMAGE_LAYOUT_GENERAL);
        UpdateDescriptorSetImage(Vulkan.Device, CompDescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearBorderZeroSampler, BloomMipViews[I], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

	// Bloom stuff
	for (uint32_t I = 0; I < ArrayCount(BloomDescrSets); I++)
    {
        UpdateDescriptorSetImage(Vulkan.Device, BloomDescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearBorderZeroSampler, BloomMipViews[ArrayCount(BloomDescrSets) - I], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

	for (uint32_t I = 0; I < ArrayCount(BloomFramebuffers); I++)
	{
	    vkDestroyFramebuffer(Vulkan.Device, BloomFramebuffers[I], 0);

		uint32_t MipLevel = ArrayCount(BloomFramebuffers) - I - 1;
		BloomFramebuffers[I] = CreateFramebuffer(Vulkan.Device, BloomRenderPass, &BloomMipViews[MipLevel], 1, Max(BloomImage.Width >> MipLevel, 1), Max(BloomImage.Height >> MipLevel, 1));
	}
}

VkRenderPass SBloomRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
{
	VkAttachmentDescription Attachment = {};
	Attachment.format = ColorFormat;
	Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachment;

	VkRenderPassCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	CreateInfo.attachmentCount = 1;
	CreateInfo.pAttachments = &Attachment;
	CreateInfo.subpassCount = 1;
	CreateInfo.pSubpasses = &Subpass;

	VkRenderPass BrightnessRenderPass = 0;
	VkCheck(vkCreateRenderPass(Device, &CreateInfo, 0, &BrightnessRenderPass));
	Assert(BrightnessRenderPass);

	return BrightnessRenderPass;	
}

VkPipeline SBloomRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
{
    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
	ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ShaderStages[0].module = VS;
	ShaderStages[0].pName = "main";
	ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ShaderStages[1].module = FS;
	ShaderStages[1].pName = "main";

	VkVertexInputBindingDescription BindingDescr = {};
	BindingDescr.binding = 0;
	BindingDescr.stride = sizeof(vec2);
	BindingDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription AttributeDescr = { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 };

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VertexInputInfo.vertexBindingDescriptionCount = 1;
	VertexInputInfo.pVertexBindingDescriptions = &BindingDescr;
	VertexInputInfo.vertexAttributeDescriptionCount = 1;
	VertexInputInfo.pVertexAttributeDescriptions = &AttributeDescr;

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo ViewportStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportStateInfo.viewportCount = 1;
	ViewportStateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo RasterizationStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	RasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	RasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	RasterizationStateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo MultisampleStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	MultisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	VkPipelineColorBlendAttachmentState ColorAttachmentState = {};
	ColorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendStateInfo.attachmentCount = 1;
	ColorBlendStateInfo.pAttachments = &ColorAttachmentState;

	VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo DynamicStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	DynamicStateInfo.dynamicStateCount = ArrayCount(DynamicStates);
	DynamicStateInfo.pDynamicStates = DynamicStates;

	VkGraphicsPipelineCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	CreateInfo.stageCount = ArrayCount(ShaderStages);
	CreateInfo.pStages = ShaderStages;
	CreateInfo.pVertexInputState = &VertexInputInfo;
	CreateInfo.pInputAssemblyState = &InputAssemblyInfo;
	CreateInfo.pViewportState = &ViewportStateInfo;
	CreateInfo.pRasterizationState = &RasterizationStateInfo;
	CreateInfo.pMultisampleState = &MultisampleStateInfo;
	CreateInfo.pDepthStencilState = &DepthStencilStateInfo;
	CreateInfo.pColorBlendState = &ColorBlendStateInfo;
	CreateInfo.pDynamicState = &DynamicStateInfo;
	CreateInfo.layout = PipelineLayout;
	CreateInfo.renderPass = RenderPass;

	VkPipeline GraphicsPipeline = 0;
	VkCheck(vkCreateGraphicsPipelines(Device, 0, 1, &CreateInfo, 0, &GraphicsPipeline));
	Assert(GraphicsPipeline);

	return GraphicsPipeline;
}

VkRenderPass SBloomRenderPass::CreateBloomRenderPass(VkDevice Device, VkFormat ColorFormat)
{
	VkAttachmentDescription Attachment = {};
	Attachment.format = ColorFormat;
	Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	Attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachment;

	VkRenderPassCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	CreateInfo.attachmentCount = 1;
	CreateInfo.pAttachments = &Attachment;
	CreateInfo.subpassCount = 1;
	CreateInfo.pSubpasses = &Subpass;

	VkRenderPass BrightnessRenderPass = 0;
	VkCheck(vkCreateRenderPass(Device, &CreateInfo, 0, &BrightnessRenderPass));
	Assert(BrightnessRenderPass);

	return BrightnessRenderPass;	
}

VkPipeline SBloomRenderPass::CreateBloomGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
{
    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
	ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ShaderStages[0].module = VS;
	ShaderStages[0].pName = "main";
	ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ShaderStages[1].module = FS;
	ShaderStages[1].pName = "main";

	VkVertexInputBindingDescription BindingDescr = {};
	BindingDescr.binding = 0;
	BindingDescr.stride = sizeof(vec2);
	BindingDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription AttributeDescr = { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 };

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VertexInputInfo.vertexBindingDescriptionCount = 1;
	VertexInputInfo.pVertexBindingDescriptions = &BindingDescr;
	VertexInputInfo.vertexAttributeDescriptionCount = 1;
	VertexInputInfo.pVertexAttributeDescriptions = &AttributeDescr;

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo ViewportStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportStateInfo.viewportCount = 1;
	ViewportStateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo RasterizationStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	RasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	RasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	RasterizationStateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo MultisampleStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	MultisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	VkPipelineColorBlendAttachmentState ColorAttachmentState = {};
	ColorAttachmentState.blendEnable = VK_TRUE;
    ColorAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    ColorAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ColorAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	ColorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendStateInfo.attachmentCount = 1;
	ColorBlendStateInfo.pAttachments = &ColorAttachmentState;

	VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo DynamicStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	DynamicStateInfo.dynamicStateCount = ArrayCount(DynamicStates);
	DynamicStateInfo.pDynamicStates = DynamicStates;

	VkGraphicsPipelineCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	CreateInfo.stageCount = ArrayCount(ShaderStages);
	CreateInfo.pStages = ShaderStages;
	CreateInfo.pVertexInputState = &VertexInputInfo;
	CreateInfo.pInputAssemblyState = &InputAssemblyInfo;
	CreateInfo.pViewportState = &ViewportStateInfo;
	CreateInfo.pRasterizationState = &RasterizationStateInfo;
	CreateInfo.pMultisampleState = &MultisampleStateInfo;
	CreateInfo.pDepthStencilState = &DepthStencilStateInfo;
	CreateInfo.pColorBlendState = &ColorBlendStateInfo;
	CreateInfo.pDynamicState = &DynamicStateInfo;
	CreateInfo.layout = PipelineLayout;
	CreateInfo.renderPass = RenderPass;

	VkPipeline GraphicsPipeline = 0;
	VkCheck(vkCreateGraphicsPipelines(Device, 0, 1, &CreateInfo, 0, &GraphicsPipeline));
	Assert(GraphicsPipeline);

	return GraphicsPipeline;
}