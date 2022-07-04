struct SExposureRenderPass
{
	static const uint32_t BrightnessImageWidth = 256;
	static const uint32_t BrightnessImageHeight = 256;
	static const uint32_t BrightnessImageMipCount = 9;

public:
    static SExposureRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage& DiffuseImage, VkSampler LinearEdgeSamplerMips, const SImage& BrightnessImage, const VkImageView* BrightnessImageMipViews, VkSampler PointEdgeSampler, const SImage* ExposureImages);
    void Render(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer, const SImage& BrightnessImage, const SImage* ExposureImages, uint32_t FrameID);
	void UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler LinearEdgeSampler, const SImage& DiffuseImage);

private:
	// Brightness stuff
    VkPipeline BrightnessPipeline;
    VkPipelineLayout BrightnessPipelineLayout;

    VkRenderPass BrightnessRenderPass;
    VkFramebuffer BrightnessFramebuffer;

    VkDescriptorSetLayout BrightnessDescrSetLayout;
    VkDescriptorSet BrightnessDescrSet;

    VkShaderModule BrightnessVShader;
    VkShaderModule BrightnessFShader;

	// Downscale stuff
	VkPipeline CompPipeline;
    VkPipelineLayout CompPipelineLayout;

    VkDescriptorSetLayout CompDescrSetLayout;
    VkDescriptorSet CompDescrSets[BrightnessImageMipCount];

    VkShaderModule CompShader;

	// Exposure stuff
	VkPipeline ExposurePipeline;
    VkPipelineLayout ExposurePipelineLayout;

    VkRenderPass ExposureRenderPass;
    VkFramebuffer ExposureFramebuffers[2];

    VkDescriptorSetLayout ExposureDescrSetLayout;
    VkDescriptorSet ExposureDescrSets[2];

    VkShaderModule ExposureVShader;
    VkShaderModule ExposureFShader;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

struct SMipgenPushConstants
{
	vec2 ImageSize;
};

SExposureRenderPass SExposureRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage& DiffuseImage, VkSampler LinearEdgeSamplerMips, const SImage& BrightnessImage, const VkImageView* BrightnessImageMipViews, VkSampler PointEdgeSampler, const SImage* ExposureImages)
{
	// Brightness stuff
    VkDescriptorSetLayoutBinding InputBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding BrightDescrSetLayoutBindings[] = { InputBinding };
    VkDescriptorSetLayout BrightnessDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(BrightDescrSetLayoutBindings), BrightDescrSetLayoutBindings);

    VkDescriptorSet BrightnessDescrSet = CreateDescriptorSet(Vulkan.Device, DescrPool, BrightnessDescrSetLayout);
    UpdateDescriptorSetImage(Vulkan.Device, BrightnessDescrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, DiffuseImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkRenderPass BrightnessRenderPass = CreateRenderPass(Vulkan.Device, BrightnessImage.Format);

    VkFramebuffer BrightnessFramebuffer = CreateFramebuffer(Vulkan.Device, BrightnessRenderPass, &BrightnessImageMipViews[0], 1, BrightnessImage.Width, BrightnessImage.Height);

    VkShaderModule BrightnessVShader = LoadShader(Vulkan.Device, "Shaders\\Fullscreen.vert.spv");
    VkShaderModule BrightnessFShader = LoadShader(Vulkan.Device, "Shaders\\Brightness.frag.spv");

    VkPipelineLayout BrightnessPipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &BrightnessDescrSetLayout);
    VkPipeline BrightnessPipeline = CreateGraphicsPipeline(Vulkan.Device, BrightnessRenderPass, BrightnessPipelineLayout, BrightnessVShader, BrightnessFShader);

	// Downscale stuff
	VkDescriptorSetLayoutBinding OutBinging = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding InBinging = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

    VkDescriptorSetLayoutBinding CompDescrSetLayoutBindings[] = { OutBinging, InBinging };
    VkDescriptorSetLayout CompDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(CompDescrSetLayoutBindings), CompDescrSetLayoutBindings);

    VkDescriptorSet CompDescrSets[ArrayCount(SExposureRenderPass::CompDescrSets)] = {};
    for (uint32_t I = 0; I < ArrayCount(CompDescrSets) - 1; I++)
    {
        CompDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, CompDescrSetLayout);

        UpdateDescriptorSetImage(Vulkan.Device, CompDescrSets[I], 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LinearEdgeSampler, BrightnessImageMipViews[I + 1], VK_IMAGE_LAYOUT_GENERAL);
        UpdateDescriptorSetImage(Vulkan.Device, CompDescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, BrightnessImageMipViews[I], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);            
    }

    VkShaderModule CompShader = LoadShader(Vulkan.Device, "Shaders\\MipGeneration.comp.spv");

    VkPipelineLayout CompPipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &CompDescrSetLayout, sizeof(SMipgenPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipeline CompPipeline = CreateComputePipeline(Vulkan.Device, CompPipelineLayout, CompShader);

	// Exposure stuff
    VkDescriptorSetLayoutBinding InputBrightnessBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding OldExposureBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding ExposureDescrSetLayoutBindings[] = { InputBrightnessBinding, OldExposureBinding };
    VkDescriptorSetLayout ExposureDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(ExposureDescrSetLayoutBindings), ExposureDescrSetLayoutBindings);

    VkDescriptorSet ExposureDescrSets[2] = {};
	for (uint32_t I = 0; I < ArrayCount(ExposureDescrSets); I++)
	{
		ExposureDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, ExposureDescrSetLayout);

    	UpdateDescriptorSetImage(Vulkan.Device, ExposureDescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSamplerMips, BrightnessImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, ExposureDescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, ExposureImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

    VkRenderPass ExposureRenderPass = CreateRenderPass(Vulkan.Device, ExposureImages[0].Format);

    VkFramebuffer ExposureFramebuffers[2] = {};
	for (uint32_t I = 0; I < ArrayCount(ExposureFramebuffers); I++)
	{
		ExposureFramebuffers[I] = CreateFramebuffer(Vulkan.Device, ExposureRenderPass, &ExposureImages[I].View, 1, ExposureImages[I].Width, ExposureImages[I].Height);
	}

    VkShaderModule ExposureVShader = LoadShader(Vulkan.Device, "Shaders\\Fullscreen.vert.spv");
    VkShaderModule ExposureFShader = LoadShader(Vulkan.Device, "Shaders\\Exposure.frag.spv");

    VkPipelineLayout ExposurePipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &ExposureDescrSetLayout);
    VkPipeline ExposurePipeline = CreateGraphicsPipeline(Vulkan.Device, ExposureRenderPass, ExposurePipelineLayout, ExposureVShader, ExposureFShader);

    SExposureRenderPass ExposurePass = {};
    ExposurePass.BrightnessPipeline = BrightnessPipeline;
    ExposurePass.BrightnessPipelineLayout = BrightnessPipelineLayout;
    ExposurePass.BrightnessRenderPass = BrightnessRenderPass;
    ExposurePass.BrightnessFramebuffer = BrightnessFramebuffer;
    ExposurePass.BrightnessDescrSetLayout = BrightnessDescrSetLayout;
    ExposurePass.BrightnessDescrSet = BrightnessDescrSet;
    ExposurePass.BrightnessVShader = BrightnessVShader;
    ExposurePass.BrightnessFShader = BrightnessFShader;
	ExposurePass.CompPipeline = CompPipeline;
    ExposurePass.CompPipelineLayout = CompPipelineLayout;
    ExposurePass.CompDescrSetLayout = CompDescrSetLayout;
    ExposurePass.CompShader = CompShader;
    memcpy(ExposurePass.CompDescrSets, CompDescrSets, sizeof(ExposurePass.CompDescrSets));
	ExposurePass.ExposurePipeline = ExposurePipeline;
    ExposurePass.ExposurePipelineLayout = ExposurePipelineLayout;
    ExposurePass.ExposureRenderPass = ExposureRenderPass;
    ExposurePass.ExposureDescrSetLayout = ExposureDescrSetLayout;
    ExposurePass.ExposureVShader = ExposureVShader;
    ExposurePass.ExposureFShader = ExposureFShader;
    memcpy(ExposurePass.ExposureFramebuffers, ExposureFramebuffers, sizeof(ExposurePass.ExposureFramebuffers));
    memcpy(ExposurePass.ExposureDescrSets, ExposureDescrSets, sizeof(ExposurePass.ExposureDescrSets));

    return ExposurePass;
}

void SExposureRenderPass::Render(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer, const SImage& BrightnessImage, const SImage* ExposureImages, uint32_t FrameID)
{
	BEGIN_GPU_PROFILER_BLOCK("EXPOSURE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Brightness stuff
	BEGIN_GPU_PROFILER_BLOCK("BRIGHTNESS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkViewport Viewport = { 0.0f, float(BrightnessImage.Height), float(BrightnessImage.Width), -float(BrightnessImage.Height), 0.0f, 1.0f };
	VkRect2D Scissor = { {0, 0}, {BrightnessImage.Width, BrightnessImage.Height} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

    VkRenderPassBeginInfo BrightRenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BrightRenderPassBeginInfo.renderPass = BrightnessRenderPass;
	BrightRenderPassBeginInfo.framebuffer = BrightnessFramebuffer;
	BrightRenderPassBeginInfo.renderArea.extent.width = BrightnessImage.Width;
	BrightRenderPassBeginInfo.renderArea.extent.height = BrightnessImage.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &BrightRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BrightnessPipeline);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, BrightnessPipelineLayout, 0, 1, &BrightnessDescrSet, 0, 0);

    VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(Vulkan.CommandBuffer);
	
	END_GPU_PROFILER_BLOCK("BRIGHTNESS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Downscale stuff
	BEGIN_GPU_PROFILER_BLOCK("DOWNSCALE_BRIGHTNESS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CompPipeline);

	VkImageMemoryBarrier DownscaleStartBarrier[] = 
	{
		CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, BrightnessImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1),
		CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, BrightnessImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, 1, BrightnessImageMipCount - 1)
	};
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(DownscaleStartBarrier), DownscaleStartBarrier);
	
	for (uint32_t I = 0; I < BrightnessImageMipCount - 1; I++)
	{
		vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, CompPipelineLayout, 0, 1, &CompDescrSets[I], 0, 0);

		vec2 ImageSize = Vec2i(Max(BrightnessImage.Width >> (I + 1), 1u), Max(BrightnessImage.Height >> (I + 1), 1u));
        SMipgenPushConstants PushConstants = { ImageSize };
		vkCmdPushConstants(Vulkan.CommandBuffer, CompPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SMipgenPushConstants), &PushConstants);

		vkCmdDispatch(Vulkan.CommandBuffer, ((uint32_t)ImageSize.x + 31) / 32, ((uint32_t)ImageSize.y + 31) / 32, 1);

		VkImageMemoryBarrier DownscaleBarrier = CreateImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, BrightnessImage.Image, VK_IMAGE_ASPECT_COLOR_BIT, I + 1, 1);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &DownscaleBarrier);
	}

	END_GPU_PROFILER_BLOCK("DOWNSCALE_BRIGHTNESS", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Exposure stuff
	BEGIN_GPU_PROFILER_BLOCK("CALCULATE_EXPOSURE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	uint32_t TargetIndex = FrameID % 2;
	uint32_t PrevIndex = 1 - TargetIndex;

	Viewport = { 0.0f, float(ExposureImages[TargetIndex].Height), float(ExposureImages[TargetIndex].Width), -float(ExposureImages[TargetIndex].Height), 0.0f, 1.0f };
	Scissor = { {0, 0}, {ExposureImages[TargetIndex].Width, ExposureImages[TargetIndex].Height} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

	if (FrameID == 0)
	{
		VkImageMemoryBarrier ExposureBeginBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ExposureImages[PrevIndex].Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &ExposureBeginBarrier);
	}

    VkRenderPassBeginInfo ExposureRenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	ExposureRenderPassBeginInfo.renderPass = ExposureRenderPass;
	ExposureRenderPassBeginInfo.framebuffer = ExposureFramebuffers[TargetIndex];
	ExposureRenderPassBeginInfo.renderArea.extent.width = ExposureImages[TargetIndex].Width;
	ExposureRenderPassBeginInfo.renderArea.extent.height = ExposureImages[TargetIndex].Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &ExposureRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ExposurePipeline);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ExposurePipelineLayout, 0, 1, &ExposureDescrSets[PrevIndex], 0, 0);

	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	VkImageMemoryBarrier ExposureEndBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ExposureImages[TargetIndex].Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &ExposureEndBarrier);

	END_GPU_PROFILER_BLOCK("CALCULATE_EXPOSURE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	// Set viewport back to fullres
	Viewport = { 0.0f, float(Vulkan.InternalHeight), float(Vulkan.InternalWidth), -float(Vulkan.InternalHeight), 0.0f, 1.0f };
	Scissor = { {0, 0}, {Vulkan.InternalWidth, Vulkan.InternalHeight} };
	vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

	END_GPU_PROFILER_BLOCK("EXPOSURE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SExposureRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler LinearEdgeSampler, const SImage& DiffuseImage)
{
    UpdateDescriptorSetImage(Vulkan.Device, BrightnessDescrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, DiffuseImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

VkRenderPass SExposureRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
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

VkPipeline SExposureRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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