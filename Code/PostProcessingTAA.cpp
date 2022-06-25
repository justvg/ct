struct STaaRenderPass
{
public:
    static STaaRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage& HDRTargetImage, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage* VelocityImages);
    void Render(const SVulkanContext& Vulkan, const SImage* HistoryImages, const SBuffer& QuadVertexBuffer, uint32_t FrameID, bool bSwapchainChanged);
	void UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler LinearEdgeSampler, const SImage& HDRTargetImage, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage* VelocityImages);

	static vec2 GetJitter(uint32_t FrameID);

private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffers[2];

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[2];

    VkShaderModule VShader;
    VkShaderModule FShader;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

STaaRenderPass STaaRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler LinearEdgeSampler, const SImage& HDRTargetImage, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage* VelocityImages)
{
    VkDescriptorSetLayoutBinding CurrBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding HistoryBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding VelocityBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding HistoryVelocityBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CurrBinding, HistoryBinding, VelocityBinding, HistoryVelocityBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[2] = {};
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
    {
        DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, HDRTargetImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, HistoryImages[(I + 1) % ArrayCount(DescrSets)].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[(I + 1) % ArrayCount(DescrSets)].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
        
    VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, HistoryImages[0].Format);

    VkFramebuffer Framebuffers[2] = {};
    for (uint32_t I = 0; I < ArrayCount(Framebuffers); I++)
    {
        Framebuffers[I] = CreateFramebuffer(Vulkan.Device, RenderPass, &HistoryImages[I].View, 1, Vulkan.Width, Vulkan.Height);
    }

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Fullscreen.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\TAA.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout);
    VkPipeline Pipeline = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);

    STaaRenderPass TaaPass = {};
    TaaPass.Pipeline = Pipeline;
    TaaPass.PipelineLayout = PipelineLayout;
    TaaPass.RenderPass = RenderPass;
    TaaPass.DescrSetLayout = DescrSetLayout;
    TaaPass.VShader = VShader;
    TaaPass.FShader = FShader;
    memcpy(TaaPass.Framebuffers, Framebuffers, sizeof(TaaPass.Framebuffers));
    memcpy(TaaPass.DescrSets, DescrSets, sizeof(TaaPass.DescrSets));

    return TaaPass;
}

void STaaRenderPass::Render(const SVulkanContext& Vulkan, const SImage* HistoryImages, const SBuffer& QuadVertexBuffer, uint32_t FrameID, bool bSwapchainChanged)
{
	BEGIN_GPU_PROFILER_BLOCK("TAA", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	const uint32_t TargetIndex = FrameID % 2;
    const uint32_t PrevIndex = (TargetIndex + 1) % 2;

	if ((FrameID == 0) || bSwapchainChanged)
	{
		VkImageMemoryBarrier TAABeginBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, HistoryImages[PrevIndex].Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &TAABeginBarrier);
	}

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffers[TargetIndex];
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.Width;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[TargetIndex], 0, 0);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	VkImageMemoryBarrier TaaEndBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, HistoryImages[TargetIndex].Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &TaaEndBarrier);

	END_GPU_PROFILER_BLOCK("TAA", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void STaaRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler LinearEdgeSampler, const SImage& HDRTargetImage, const SImage* HistoryImages, VkSampler PointEdgeSampler, const SImage* VelocityImages)
{
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
    {
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, HDRTargetImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, HistoryImages[(I + 1) % ArrayCount(DescrSets)].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[(I + 1) % ArrayCount(DescrSets)].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
        
    for (uint32_t I = 0; I < ArrayCount(Framebuffers); I++)
    {
		vkDestroyFramebuffer(Vulkan.Device, Framebuffers[I], 0);
        Framebuffers[I] = CreateFramebuffer(Vulkan.Device, RenderPass, &HistoryImages[I].View, 1, Vulkan.Width, Vulkan.Height);
    }
}

vec2 STaaRenderPass::GetJitter(uint32_t FrameID)
{
	static const vec2 vSSAA2x[2] =
	{
		Vec2(-0.25f, 0.25f),
		Vec2(0.25f, -0.25f)
	};

	static const vec2 vSSAA4x[4] =
	{
		Vec2(-0.25f, 0.25f),
		Vec2(-0.25f, -0.25f),
		Vec2(0.25f, 0.25f),
		Vec2(0.25f, -0.25f)
	};

	vec2 Jitter = vSSAA4x[FrameID % ArrayCount(vSSAA4x)];
	return Jitter;
}

VkRenderPass STaaRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
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

	VkRenderPass RenderPass = 0;
	VkCheck(vkCreateRenderPass(Device, &CreateInfo, 0, &RenderPass));
	Assert(RenderPass);

	return RenderPass;	
}

VkPipeline STaaRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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