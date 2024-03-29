struct STransparentRenderPass
{
public:
    static STransparentRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SImage& DiffuseImage, const SImage& DepthImage);
    void Render(const SVulkanContext& Vulkan, SEntity* Entities, uint32_t EntityCount, const SGeometry& Geometry, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, uint32_t FrameID, float Time);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& DiffuseImage, const SImage& DepthImage);

private:
	VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffer;

	VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;

	VkPipeline PipelinePortal;
	VkShaderModule FShaderPortal;

	VkPipeline PipelineDoor;
	VkShaderModule FShaderDoor;

	VkPipeline PipelineFireball;
	VkShaderModule FShaderFireball;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat);
	static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

STransparentRenderPass STransparentRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SImage& DiffuseImage, const SImage& DepthImage)
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

	VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, DiffuseImage.Format, Vulkan.DepthFormat);

	VkImageView FramebufferAttachments[] = { DiffuseImage.View, DepthImage.View };
    VkFramebuffer Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), DiffuseImage.Width, DiffuseImage.Height);

	VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Mesh.vert.spv");
	VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\MeshTransparent.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SMeshRenderPushConstants), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkPipeline Pipeline = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);

	VkShaderModule FShaderPortal = LoadShader(Vulkan.Device, "Shaders\\Portal.frag.spv");
	VkPipeline PipelinePortal = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShaderPortal);

	VkShaderModule FShaderDoor = LoadShader(Vulkan.Device, "Shaders\\Door.frag.spv");
	VkPipeline PipelineDoor = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShaderDoor);

	VkShaderModule FShaderFireball = LoadShader(Vulkan.Device, "Shaders\\Fireball.frag.spv");
	VkPipeline PipelineFireball = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShaderFireball);;

	STransparentRenderPass TransparentRenderPass = {};
	TransparentRenderPass.Pipeline = Pipeline;
    TransparentRenderPass.PipelineLayout = PipelineLayout;
    TransparentRenderPass.RenderPass = RenderPass;
    TransparentRenderPass.Framebuffer = Framebuffer;
	TransparentRenderPass.DescrSetLayout = DescrSetLayout;
    TransparentRenderPass.VShader = VShader;
    TransparentRenderPass.FShader = FShader;
	TransparentRenderPass.PipelinePortal = PipelinePortal;
	TransparentRenderPass.FShaderPortal = FShaderPortal;
	TransparentRenderPass.PipelineDoor = PipelineDoor;
	TransparentRenderPass.FShaderDoor = FShaderDoor;
	TransparentRenderPass.PipelineFireball = PipelineFireball;
	TransparentRenderPass.FShaderFireball = FShaderFireball;
    memcpy(TransparentRenderPass.DescrSets, DescrSets, sizeof(TransparentRenderPass.DescrSets));

	return TransparentRenderPass;
}

void STransparentRenderPass::Render(const SVulkanContext& Vulkan, SEntity* Entities, uint32_t EntityCount, const SGeometry& Geometry, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, uint32_t FrameID, float Time)
{
	BEGIN_GPU_PROFILER_BLOCK("RENDER_TRANSPARENT", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffer;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	SMeshRenderPushConstants PushConstants = {};
	PushConstants.Time = Time - 10.000f * Floor(Time / 10000.0f);

	EMeshMaterial LastMeshMaterial = MeshMaterial_Default;
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		const SEntity& Entity = Entities[I];
		Assert(Entity.Alpha < 1.0f);

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
			case Entity_Gates:
			{
				if (LastMeshMaterial != MeshMaterial_Portal)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelinePortal);
					vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

					LastMeshMaterial = MeshMaterial_Portal;
				}

				if (Entity.bFinishedLevel)
				{
					PushConstants.Color.rgb = Vec3(0.25f, 1.0f, 0.45f);
				}
				else if (Entity.bClosedGates)
				{
					PushConstants.Color.rgb = Vec3(1.0f, 0.3f, 0.3f);
				}
			} break;

			case Entity_Door:
			{
				if (LastMeshMaterial != MeshMaterial_Door)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineDoor);
					vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

					LastMeshMaterial = MeshMaterial_Door;
				}

				PushConstants.ShaderValue0 = 0.1f;
			} break;

			case Entity_Fireball:
			{
				if (LastMeshMaterial != MeshMaterial_Fireball)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineFireball);
					vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

					LastMeshMaterial = MeshMaterial_Fireball;
				}
			} break;

			default:
			{
				if (LastMeshMaterial != MeshMaterial_TranspDefault)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
					vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

					LastMeshMaterial = MeshMaterial_TranspDefault;
				}
			}
		}

		const SMesh& Mesh = Geometry.Meshes[Entity.MeshIndex];
		vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SMeshRenderPushConstants), &PushConstants);
		vkCmdDrawIndexed(Vulkan.CommandBuffer, Mesh.IndexCount, 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);
	}

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("RENDER_TRANSPARENT", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void STransparentRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& DiffuseImage, const SImage& DepthImage)
{
	vkDestroyFramebuffer(Vulkan.Device, Framebuffer, 0);

	VkImageView FramebufferAttachments[] = { DiffuseImage.View, DepthImage.View };
    Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), DiffuseImage.Width, DiffuseImage.Height);
}

VkRenderPass STransparentRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat)
{
	VkAttachmentDescription Attachments[2] = {};

	Attachments[0].format = ColorFormat;
	Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[1].format = DepthFormat;
	Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	Attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference DepthAttachment = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachment;
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

VkPipeline STransparentRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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
	RasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
	RasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	RasterizationStateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo MultisampleStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	MultisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateInfo.depthTestEnable = VK_TRUE;
	DepthStencilStateInfo.depthWriteEnable = VK_FALSE;
	DepthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	DepthStencilStateInfo.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorAttachmentState = {};
	ColorAttachmentState.blendEnable = VK_TRUE;
    ColorAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColorAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ColorAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    ColorAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorAttachmentState.alphaBlendOp = VK_BLEND_OP_MIN;
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