struct SHUDRenderPass
{
    public:
    static SHUDRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* ProjectionBuffers, const SImage& FinalImage);
    void BeginRender(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer);
    void Render(const SVulkanContext& Vulkan, VkDescriptorSet TextureDescrSet, vec2 ScreenPosition, vec2 Scale);
	void RenderString(const SVulkanContext& Vulkan, const SFont* Font, vec2 ScreenPosition, vec2 FontScale, const char* String, vec4 Color, float BlendFactor = 1.0f);
	void RenderStringWithAppearance(const SVulkanContext& Vulkan, const SFont* Font, vec2 BaseLinePos, vec2 FontScale, const char* String, float AppearanceFactor);
	void EndRender(const SVulkanContext& Vulkan);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& FinalImage);
    
	VkDescriptorSetLayout GetTextureDescrSetLayout();
	VkFramebuffer GetFramebuffer();
    
    private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;
    
    VkRenderPass RenderPass;
    VkFramebuffer Framebuffer;
    
    VkDescriptorSetLayout TransformDescrSetLayout;
    VkDescriptorSet TransformDescrSets[FramesInFlight];
    
    VkDescriptorSetLayout TextureDescrSetLayout;
    
    VkShaderModule VShader;
    VkShaderModule FShader;
    
    private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

struct SHUDRenderPassPushConstants
{
	vec4 PositionScale;
	vec4 MaxMinTC;
    
    uint32_t FontRendering;
	float BlendFactor;
	float Padding0;
	float Padding1;

	vec4 FontColor;
};

SHUDRenderPass SHUDRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* ProjectionBuffers, const SImage& FinalImage)
{
	VkDescriptorSetLayoutBinding TransformBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayout TransformDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, 1, &TransformBinding);
    
	VkDescriptorSet TransformDescrSets[FramesInFlight] = {};
	for (uint32_t I = 0; I < ArrayCount(TransformDescrSets); I++)
	{
		TransformDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, TransformDescrSetLayout);
		UpdateDescriptorSetBuffer(Vulkan.Device, TransformDescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ProjectionBuffers[I], ProjectionBuffers[I].Size);
	}
    
	VkDescriptorSetLayoutBinding TextureBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayout TextureDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, 1, &TextureBinding);
    
	VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, FinalImage.Format);
    
    VkFramebuffer Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &FinalImage.View, 1, FinalImage.Width, FinalImage.Height);
    
    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\HUD.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\HUD.frag.spv");
    
	VkDescriptorSetLayout DescrSetLayouts[] = { TransformDescrSetLayout, TextureDescrSetLayout };
	VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, ArrayCount(DescrSetLayouts), DescrSetLayouts, sizeof(SHUDRenderPassPushConstants), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkPipeline Pipeline = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);
    
	SHUDRenderPass HUDRenderPass = {};
	HUDRenderPass.Pipeline = Pipeline;
    HUDRenderPass.PipelineLayout = PipelineLayout;
    HUDRenderPass.RenderPass = RenderPass;
    HUDRenderPass.Framebuffer = Framebuffer;
    HUDRenderPass.TransformDescrSetLayout = TransformDescrSetLayout;
    HUDRenderPass.TextureDescrSetLayout = TextureDescrSetLayout;
    HUDRenderPass.VShader = VShader;
    HUDRenderPass.FShader = FShader;
    memcpy(HUDRenderPass.TransformDescrSets, TransformDescrSets, sizeof(HUDRenderPass.TransformDescrSets));
    
	return HUDRenderPass;
}

void SHUDRenderPass::BeginRender(const SVulkanContext& Vulkan, const SBuffer& QuadVertexBuffer)
{
	BEGIN_GPU_PROFILER_BLOCK("HUD", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    
	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffer;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.Width;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
    
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &TransformDescrSets[Vulkan.FrameInFlight], 0, 0);
	
	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);
}

void SHUDRenderPass::Render(const SVulkanContext& Vulkan, VkDescriptorSet TextureDescrSet, vec2 ScreenPosition, vec2 Scale)
{
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 1, 1, &TextureDescrSet, 0, 0);
    
	SHUDRenderPassPushConstants PushConstants = { Vec4(ScreenPosition, Scale), Vec4(1.0f, 1.0f, 0.0f, 0.0f), 0 };

	vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SHUDRenderPassPushConstants), &PushConstants);
    
	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);
}

void SHUDRenderPass::RenderString(const SVulkanContext& Vulkan, const SFont* Font, vec2 BaseLinePos, vec2 FontScale, const char* String, vec4 Color, float BlendFactor)
{
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 1, 1, &Font->BitmapFontDescrSet, 0, 0);
    
    const uint32_t Length = StringLength(String);
    
	for (const char *C = String; *C; C++)
	{
		Assert(*C < ArrayCount(Font->Glyphs));
		const SGlyph& Glyph = Font->Glyphs[*C];
        
		if (*C != ' ')
		{
			vec2 ScreenPosition = BaseLinePos;
			ScreenPosition.x += FontScale.x * (Glyph.OffsetX + 0.5f * Glyph.Width);
			ScreenPosition.y += FontScale.y * (Glyph.OffsetY - 0.5f * Glyph.Height);
            
			SHUDRenderPassPushConstants PushConstants = {};
			PushConstants.PositionScale = Vec4(ScreenPosition, FontScale.x * Glyph.Width, FontScale.y * Glyph.Height);
			PushConstants.MaxMinTC = Vec4(Glyph.UVs.z, 1.0f - Glyph.UVs.y, Glyph.UVs.x, 1.0f - Glyph.UVs.w);
			PushConstants.FontRendering = 1;
			PushConstants.BlendFactor = BlendFactor;
			PushConstants.FontColor = Color;

			vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SHUDRenderPassPushConstants), &PushConstants);
            
			vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);
		}
        
		BaseLinePos.x += FontScale.x * Glyph.Advance;
	}
}

void SHUDRenderPass::RenderStringWithAppearance(const SVulkanContext& Vulkan, const SFont* Font, vec2 BaseLinePos, vec2 FontScale, const char* String, float AppearanceFactor)
{
    vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 1, 1, &Font->BitmapFontDescrSet, 0, 0);
    
    const uint32_t Length = StringLength(String);
	for (uint32_t I = 0; I < Length; I++)
    {
		const char C = String[I];
        Assert(C < ArrayCount(Font->Glyphs));
        const SGlyph& Glyph = Font->Glyphs[C];

		float BlendFactor = Clamp(Length * AppearanceFactor - I, 0.0f, 1.0f);
	
		if (BlendFactor > 0.0f)
		{
			if (C != ' ')
			{
				vec2 ScreenPosition = BaseLinePos;
				ScreenPosition.x += FontScale.x * (Glyph.OffsetX + 0.5f * Glyph.Width);
				ScreenPosition.y += FontScale.y * (Glyph.OffsetY - 0.5f * Glyph.Height);
				
				SHUDRenderPassPushConstants PushConstants = {};
				PushConstants.PositionScale = Vec4(ScreenPosition, FontScale.x * Glyph.Width, FontScale.y * Glyph.Height);
				PushConstants.MaxMinTC = Vec4(Glyph.UVs.z, 1.0f - Glyph.UVs.y, Glyph.UVs.x, 1.0f - Glyph.UVs.w);
				PushConstants.FontRendering = 1;
				PushConstants.BlendFactor = BlendFactor;
				PushConstants.FontColor = Vec4(1.0f);

				vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SHUDRenderPassPushConstants), &PushConstants);
				
				vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);
			}
		}
        
        BaseLinePos.x += FontScale.x * Glyph.Advance;
    }
}

void SHUDRenderPass::EndRender(const SVulkanContext& Vulkan)
{
	vkCmdEndRenderPass(Vulkan.CommandBuffer);
    
	END_GPU_PROFILER_BLOCK("HUD", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SHUDRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& FinalImage)
{
	vkDestroyFramebuffer(Vulkan.Device, Framebuffer, 0);
	Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &FinalImage.View, 1, FinalImage.Width, FinalImage.Height);
}

VkDescriptorSetLayout SHUDRenderPass::GetTextureDescrSetLayout()
{
	return TextureDescrSetLayout;
}

VkFramebuffer SHUDRenderPass::GetFramebuffer()
{
	return Framebuffer;
}

VkRenderPass SHUDRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
{
	VkAttachmentDescription Attachment = {};
	Attachment.format = ColorFormat;
	Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

VkPipeline SHUDRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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
    ColorAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColorAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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