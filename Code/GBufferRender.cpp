struct SGBufferRenderPass
{
public:
    static SGBufferRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage);
    void Render(const SVulkanContext& Vulkan, SEntity* Entities, uint32_t EntityCount, const SGeometry& Geometry, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, uint32_t FrameID, bool bGameMode, float Time);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage);

private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffers[2];

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;
	
private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat, VkFormat NormalsFormat, VkFormat MaterialFormat, VkFormat VelocityFormat, VkFormat LinearDepthFormat, VkFormat DepthFormat);
	static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

struct SMeshRenderPushConstants
{
	vec4 Position; // w - unused
	vec4 Scale; // w - unused
	quat Orientation;
	vec4 Color; // w - unused
	vec4 Offset; // w - unused

	vec4 PrevPosition; // w - unused
	quat PrevOrientation;

	uint32_t FirstPersonDepthTest;

	float Time;
	float ShaderValue0; // NOTE(georgii): Currently used for material parameters. Like MaxComponentNoise in door shader
};

enum EMeshMaterial
{
	MeshMaterial_Default,
	MeshMaterial_TranspDefault,

	MeshMaterial_Portal,
	MeshMaterial_Door,
	MeshMaterial_Fireball
};

SGBufferRenderPass SGBufferRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, 
                                              const SBuffer* CameraBuffers,
                                              const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage)
{
    VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[FramesInFlight] = {};
	for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
	{
		DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
		UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
	}

    VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, AlbedoImage.Format, NormalsImage.Format, MaterialImage.Format, VelocityImages[0].Format, LinearDepthImage.Format, Vulkan.DepthFormat);

    VkFramebuffer Framebuffers[2] = {};
	for (uint32_t I = 0; I < ArrayCount(Framebuffers); I++)
	{
		VkImageView FramebufferAttachments[] = { AlbedoImage.View, NormalsImage.View, MaterialImage.View, VelocityImages[I].View, LinearDepthImage.View, DepthImage.View };
		Framebuffers[I] = CreateFramebuffer(Vulkan.Device, RenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), AlbedoImage.Width, AlbedoImage.Height);
	}

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Mesh.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\Mesh.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SMeshRenderPushConstants), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline Pipeline = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);

    SGBufferRenderPass GBufferRenderPass = {};
	GBufferRenderPass.Pipeline = Pipeline;
    GBufferRenderPass.PipelineLayout = PipelineLayout;
    GBufferRenderPass.RenderPass = RenderPass;
    GBufferRenderPass.DescrSetLayout = DescrSetLayout;
    GBufferRenderPass.VShader = VShader;
    GBufferRenderPass.FShader = FShader;
    memcpy(GBufferRenderPass.Framebuffers, Framebuffers, sizeof(GBufferRenderPass.Framebuffers));
    memcpy(GBufferRenderPass.DescrSets, DescrSets, sizeof(GBufferRenderPass.DescrSets));

    return GBufferRenderPass;
}

void SGBufferRenderPass::Render(const SVulkanContext& Vulkan, SEntity* Entities, uint32_t EntityCount, const SGeometry& Geometry, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, uint32_t FrameID, bool bGameMode, float Time)
{
	BEGIN_GPU_PROFILER_BLOCK("RENDER_ENTITIES", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffers[FrameID % 2];
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	SMeshRenderPushConstants PushConstants = {};
	PushConstants.Time = Time - 10.000f * Floor(Time / 10000.0f);

	for (uint32_t I = 0; I < EntityCount; I++)
	{
		const SEntity& Entity = Entities[I];
		Assert(Entity.Alpha == 1.0f);

		PushConstants.Position = Vec4(Entity.Pos, 0.0f);
		PushConstants.Scale = Vec4(Entity.Scale * Entity.Dim, 0.0f);
		PushConstants.Orientation = EulerToQuat(Entity.Orientation.xyz);
		PushConstants.Color = Vec4(Entity.Color, Entity.Alpha);
		PushConstants.Offset = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
		PushConstants.PrevPosition.xyz = Entity.PrevPos;
		PushConstants.PrevOrientation = EulerToQuat(Entity.PrevOrientation.xyz);
		PushConstants.FirstPersonDepthTest = false;

		switch (Entity.Type)
		{
			case Entity_Hero:
			{
				if (bGameMode)
				{
					continue;
				}
				else
				{
					PushConstants.Scale = Vec4(0.3f, 0.3f, 0.3f, 0.3f);
					PushConstants.Color = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
				}
			} break;

			case Entity_Torch:
			{
				PushConstants.Color.rgb *= 200.0f;
			} break;

			case Entity_Wire:
			{
				if (Entity.bActive)
				{
					PushConstants.Color.rgb *= Max(Entity.ColorScale, 1.0f);
				}
			} break;

			case Entity_Fireball:
			{
				PushConstants.Color.rgb *= 200.0f;
			} break;

			case Entity_MessageToggler:
			{
				if (bGameMode)
				{
					continue;
				}
				else
				{
					PushConstants.Scale = Vec4(0.3f, 0.3f, 0.3f, 0.3f);
					PushConstants.Color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			} break;

			case Entity_Checkpoint:
			{
				if (bGameMode)
				{
					continue;
				}
				else
				{
					PushConstants.Scale = Vec4(0.3f, 0.3f, 0.3f, 0.3f);
					PushConstants.Color = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
				}
			} break;
		}

		const SMesh& Mesh = Geometry.Meshes[Entity.MeshIndex];
		vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SMeshRenderPushConstants), &PushConstants);
		vkCmdDrawIndexed(Vulkan.CommandBuffer, Mesh.IndexCount, 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);
	}

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("RENDER_ENTITIES", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SGBufferRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage)
{
	for (uint32_t I = 0; I < ArrayCount(Framebuffers); I++)
	{
		vkDestroyFramebuffer(Vulkan.Device, Framebuffers[I], 0);
	}
	
	for (uint32_t I = 0; I < ArrayCount(Framebuffers); I++)
	{
		VkImageView FramebufferAttachments[] = { AlbedoImage.View, NormalsImage.View, MaterialImage.View, VelocityImages[I].View, LinearDepthImage.View, DepthImage.View };
		Framebuffers[I] = CreateFramebuffer(Vulkan.Device, RenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), AlbedoImage.Width, AlbedoImage.Height);
	}
}

VkRenderPass SGBufferRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat, VkFormat NormalsFormat, VkFormat MaterialFormat, VkFormat VelocityFormat, VkFormat LinearDepthFormat, VkFormat DepthFormat)
{
	VkAttachmentDescription Attachments[6] = {};

	Attachments[0].format = ColorFormat;
	Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[1].format = NormalsFormat;
	Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[2].format = MaterialFormat;
	Attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	Attachments[3].format = VelocityFormat;
	Attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[3].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	Attachments[4].format = LinearDepthFormat;
	Attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[4].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[5].format = DepthFormat;
	Attachments[5].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[5].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[5].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[5].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	Attachments[5].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference NormalsAttachment = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference MaterialAttachment = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference VelocityAttachment = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference LinearDepthAttachment = { 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference DepthAttachment = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkAttachmentReference ColorAttachments[] = { ColorAttachment, NormalsAttachment, MaterialAttachment, VelocityAttachment, LinearDepthAttachment };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = ArrayCount(ColorAttachments);
	Subpass.pColorAttachments = ColorAttachments;
	Subpass.pDepthStencilAttachment = &DepthAttachment;

	VkRenderPassCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	CreateInfo.attachmentCount = ArrayCount(Attachments);
	CreateInfo.pAttachments = Attachments;
	CreateInfo.subpassCount = 1;
	CreateInfo.pSubpasses = &Subpass;

	VkRenderPass RenderPass = 0;
	VkCheck(vkCreateRenderPass(Device, &CreateInfo, 0, &RenderPass));
	Assert(RenderPass);

	return RenderPass;
}

VkPipeline SGBufferRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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
	BindingDescr.stride = sizeof(SVertex);
	BindingDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription AttributeDescrs[2] = {};
	AttributeDescrs[0].location = 0;
	AttributeDescrs[0].binding = 0;
	AttributeDescrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	AttributeDescrs[0].offset = 0;
	AttributeDescrs[1].location = 1;
	AttributeDescrs[1].binding = 0;
	AttributeDescrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	AttributeDescrs[1].offset = OffsetOf(SVertex, Normal);

	VkPipelineVertexInputStateCreateInfo VertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VertexInputInfo.vertexBindingDescriptionCount = 1;
	VertexInputInfo.pVertexBindingDescriptions = &BindingDescr;
	VertexInputInfo.vertexAttributeDescriptionCount = ArrayCount(AttributeDescrs);
	VertexInputInfo.pVertexAttributeDescriptions = AttributeDescrs;

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
	DepthStencilStateInfo.depthTestEnable = VK_TRUE;
	DepthStencilStateInfo.depthWriteEnable = VK_TRUE;
	DepthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	DepthStencilStateInfo.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorAttachmentStates[5] = {};
	ColorAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[3].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[4].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendStateInfo.attachmentCount = ArrayCount(ColorAttachmentStates);
	ColorBlendStateInfo.pAttachments = ColorAttachmentStates;

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