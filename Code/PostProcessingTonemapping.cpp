struct SToneMappingRenderPass
{
public:
    static SToneMappingRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler PointEdgeSampler, const SImage* HistoryImages, const SImage* ExposureImages, VkSampler LinearEdgeSampler, const SImage& BloomImage, VkSampler NoiseSampler, const SImage& NoiseTexture, const SImage& FinalImage);
    void Render(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer, uint32_t FrameID, bool bMenuOpened, bool bVignette);
	void UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler PointEdgeSampler, const SImage* HistoryImages, VkSampler LinearEdgeSampler, const SImage& BloomImage, const SImage& FinalImage);

private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffer;

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[2];

    VkShaderModule VShader;
    VkShaderModule FShader;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

struct STonemappingRenderPassPushConstants
{
	uint32_t MenuOpened;
    uint32_t VignetteEnabled;
	uint32_t FrameNubmer;
};

SToneMappingRenderPass SToneMappingRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, VkSampler PointEdgeSampler, const SImage* HistoryImages, const SImage* ExposureImages, VkSampler LinearEdgeSampler, const SImage& BloomImage, VkSampler NoiseSampler, const SImage& NoiseTexture, const SImage& FinalImage)
{
    VkDescriptorSetLayoutBinding InputBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding ExposureBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding BloomBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding NoiseBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { InputBinding, ExposureBinding, BloomBinding, NoiseBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[2] = {};
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
    {
        DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, HistoryImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, ExposureImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, BloomImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NoiseSampler, NoiseTexture.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
        
    VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, FinalImage.Format);

    VkFramebuffer Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &FinalImage.View, 1, FinalImage.Width, FinalImage.Height);

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Fullscreen.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\Tonemapping.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(STonemappingRenderPassPushConstants), VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline Pipeline = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);

    SToneMappingRenderPass ToneMappingPass = {};
    ToneMappingPass.Pipeline = Pipeline;
    ToneMappingPass.PipelineLayout = PipelineLayout;
    ToneMappingPass.RenderPass = RenderPass;
    ToneMappingPass.Framebuffer = Framebuffer;
    ToneMappingPass.DescrSetLayout = DescrSetLayout;
    ToneMappingPass.VShader = VShader;
    ToneMappingPass.FShader = FShader;
    memcpy(ToneMappingPass.DescrSets, DescrSets, sizeof(ToneMappingPass.DescrSets));

    return ToneMappingPass;
}

void SToneMappingRenderPass::Render(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer, uint32_t FrameID, bool bMenuOpened, bool bVignette)
{
	BEGIN_GPU_PROFILER_BLOCK("TONEMAPPING", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffer;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.Width;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[FrameID % 2], 0, 0);

	STonemappingRenderPassPushConstants PushConstants = { bMenuOpened ? 1u : 0u, bVignette ? 1u : 0u, FrameID % 8 };
	vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(STonemappingRenderPassPushConstants), &PushConstants);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("TONEMAPPING", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SToneMappingRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler PointEdgeSampler, const SImage* HistoryImages, VkSampler LinearEdgeSampler, const SImage& BloomImage, const SImage& FinalImage)
{
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
    {
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, HistoryImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, BloomImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
        
    vkDestroyFramebuffer(Vulkan.Device, Framebuffer, 0);
    Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &FinalImage.View, 1, FinalImage.Width, FinalImage.Height);
}

VkRenderPass SToneMappingRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
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

VkPipeline SToneMappingRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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