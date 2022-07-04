struct SGBufferVoxelRenderPass
{
public:
    static SGBufferVoxelRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SBuffer& DrawBuffer, const SBuffer& VoxelsBuffer, const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage);
    void RenderEarly(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, vec3 AmbientColor, uint32_t FrameID, bool bDraw);
    void RenderLate(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, uint32_t FrameID, bool bDraw);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage);

private:
    VkPipeline PipelineEarly, PipelineLate;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPassEarly, RenderPassLate;
    VkFramebuffer FramebuffersEarly[2], FramebuffersLate[2];

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;

private:
    static VkRenderPass CreateRenderPassEarly(VkDevice Device, VkFormat ColorFormat, VkFormat NormalsFormat, VkFormat MaterialFormat, VkFormat VelocityFormat, VkFormat LinearDepthFormat, VkFormat DepthFormat);
	static VkRenderPass CreateRenderPassLate(VkDevice Device, VkFormat ColorFormat, VkFormat NormalsFormat, VkFormat MaterialFormat, VkFormat VelocityFormat, VkFormat LinearDepthFormat, VkFormat DepthFormat);
	static VkPipeline CreateGraphicsPipelineEarly(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
	static VkPipeline CreateGraphicsPipelineLate(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

SGBufferVoxelRenderPass SGBufferVoxelRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, 
                                              			const SBuffer* CameraBuffers, const SBuffer& DrawBuffer, const SBuffer& VoxelsBuffer,
                                              			const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage)
{
    VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DrawsBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayoutBinding VoxelsBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding, DrawsBinding, VoxelsBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[FramesInFlight] = {};
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
	{
		DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DrawBuffer, DrawBuffer.Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VoxelsBuffer, VoxelsBuffer.Size);
	}

    VkRenderPass RenderPassEarly = CreateRenderPassEarly(Vulkan.Device, AlbedoImage.Format, NormalsImage.Format, MaterialImage.Format, VelocityImages[0].Format, LinearDepthImage.Format, Vulkan.DepthFormat);
    VkRenderPass RenderPassLate = CreateRenderPassLate(Vulkan.Device, AlbedoImage.Format, NormalsImage.Format, MaterialImage.Format, VelocityImages[0].Format, LinearDepthImage.Format, Vulkan.DepthFormat);

    VkFramebuffer FramebuffersEarly[2] = {};
    VkFramebuffer FramebuffersLate[2] = {};
	for (uint32_t I = 0; I < 2; I++)
	{
		VkImageView FramebufferAttachments[] = { AlbedoImage.View, NormalsImage.View, MaterialImage.View, VelocityImages[I].View, LinearDepthImage.View, DepthImage.View };
		FramebuffersEarly[I] = CreateFramebuffer(Vulkan.Device, RenderPassLate, FramebufferAttachments, ArrayCount(FramebufferAttachments), AlbedoImage.Width, AlbedoImage.Height);
		FramebuffersLate[I] = CreateFramebuffer(Vulkan.Device, RenderPassEarly, FramebufferAttachments, ArrayCount(FramebufferAttachments), AlbedoImage.Width, AlbedoImage.Height);
	}

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Voxel.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\Voxel.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout);
	VkPipeline PipelineEarly = CreateGraphicsPipelineEarly(Vulkan.Device, RenderPassEarly, PipelineLayout, VShader, FShader);
	VkPipeline PipelineLate = CreateGraphicsPipelineLate(Vulkan.Device, RenderPassLate, PipelineLayout, VShader, FShader);

    SGBufferVoxelRenderPass GBufferVoxelRenderPass = {};
	GBufferVoxelRenderPass.PipelineEarly = PipelineEarly;
	GBufferVoxelRenderPass.PipelineLate = PipelineLate;
    GBufferVoxelRenderPass.PipelineLayout = PipelineLayout;
    GBufferVoxelRenderPass.RenderPassEarly = RenderPassEarly;
    GBufferVoxelRenderPass.RenderPassLate = RenderPassLate;
    GBufferVoxelRenderPass.DescrSetLayout = DescrSetLayout;
    GBufferVoxelRenderPass.VShader = VShader;
    GBufferVoxelRenderPass.FShader = FShader;
    memcpy(GBufferVoxelRenderPass.FramebuffersEarly, FramebuffersEarly, sizeof(GBufferVoxelRenderPass.FramebuffersEarly));
    memcpy(GBufferVoxelRenderPass.FramebuffersLate, FramebuffersLate, sizeof(GBufferVoxelRenderPass.FramebuffersLate));
	memcpy(GBufferVoxelRenderPass.DescrSets, DescrSets, sizeof(GBufferVoxelRenderPass.DescrSets));

    return GBufferVoxelRenderPass;
}

void SGBufferVoxelRenderPass::RenderEarly(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, vec3 AmbientColor, uint32_t FrameID, bool bDraw)
{
	BEGIN_GPU_PROFILER_BLOCK("VOXEL_RENDER_EARLY", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkClearValue ClearColorValue = { AmbientColor.x, AmbientColor.y, AmbientColor.z };
	VkClearValue ClearNormalsValue = { 0.0f, 0.0f, 0.0f, 0.0f };
	VkClearValue ClearMaterialValue = { 0.0f, 0.0f };
	VkClearValue ClearVelocityValue = { 0.0f, 0.0f };
	VkClearValue ClearLinearDepthValue = { 1.0f };
	VkClearValue ClearDepthValue = { 1.0f, 0.0f };
	VkClearValue ClearValues[] = { ClearColorValue, ClearNormalsValue, ClearMaterialValue, ClearVelocityValue, ClearLinearDepthValue, ClearDepthValue };
	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPassEarly;
	RenderPassBeginInfo.framebuffer = FramebuffersEarly[FrameID % 2];
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	RenderPassBeginInfo.clearValueCount = ArrayCount(ClearValues);
	RenderPassBeginInfo.pClearValues = ClearValues;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineEarly);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	if (bDraw)
	{
		if (Vulkan.vkCmdDrawIndexedIndirectCount)
		{
			Vulkan.vkCmdDrawIndexedIndirectCount(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, CountBuffer.Buffer, 0, ObjectsCount, sizeof(VkDrawIndexedIndirectCommand));
		}
		else
		{
			vkCmdDrawIndexedIndirect(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, ObjectsCount, sizeof(VkDrawIndexedIndirectCommand));
		}
	}

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("VOXEL_RENDER_EARLY", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SGBufferVoxelRenderPass::RenderLate(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, uint32_t FrameID, bool bDraw)
{
	BEGIN_GPU_PROFILER_BLOCK("VOXEL_RENDER_LATE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPassLate;
	RenderPassBeginInfo.framebuffer = FramebuffersLate[FrameID % 2];
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLate);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	if (bDraw)
	{
		if (Vulkan.vkCmdDrawIndexedIndirectCount)
		{
			Vulkan.vkCmdDrawIndexedIndirectCount(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, CountBuffer.Buffer, 0, ObjectsCount, sizeof(VkDrawIndexedIndirectCommand));
		}		
		else
		{
			vkCmdDrawIndexedIndirect(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, ObjectsCount, sizeof(VkDrawIndexedIndirectCommand));
		}
	}

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("VOXEL_RENDER_LATE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SGBufferVoxelRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& AlbedoImage, const SImage& NormalsImage, const SImage& MaterialImage, const SImage* VelocityImages, const SImage& LinearDepthImage, const SImage& DepthImage)
{
	for (uint32_t I = 0; I < ArrayCount(FramebuffersEarly); I++)
	{
		vkDestroyFramebuffer(Vulkan.Device, FramebuffersEarly[I], 0);
		vkDestroyFramebuffer(Vulkan.Device, FramebuffersLate[I], 0);
	}
	
    for (uint32_t I = 0; I < 2; I++)
	{
		VkImageView FramebufferAttachments[] = { AlbedoImage.View, NormalsImage.View, MaterialImage.View, VelocityImages[I].View, LinearDepthImage.View, DepthImage.View };
		FramebuffersEarly[I] = CreateFramebuffer(Vulkan.Device, RenderPassLate, FramebufferAttachments, ArrayCount(FramebufferAttachments), AlbedoImage.Width, AlbedoImage.Height);
		FramebuffersLate[I] = CreateFramebuffer(Vulkan.Device, RenderPassEarly, FramebufferAttachments, ArrayCount(FramebufferAttachments), AlbedoImage.Width, AlbedoImage.Height);
	}
}

VkRenderPass SGBufferVoxelRenderPass::CreateRenderPassEarly(VkDevice Device, VkFormat ColorFormat, VkFormat NormalsFormat, VkFormat MaterialFormat, VkFormat VelocityFormat, VkFormat LinearDepthFormat, VkFormat DepthFormat)
{
	VkAttachmentDescription Attachments[6] = {};

	Attachments[0].format = ColorFormat;
	Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	Attachments[1].format = NormalsFormat;
	Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[2].format = MaterialFormat;
	Attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[3].format = VelocityFormat;
	Attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[4].format = LinearDepthFormat;
	Attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[5].format = DepthFormat;
	Attachments[5].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[5].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[5].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[5].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

VkRenderPass SGBufferVoxelRenderPass::CreateRenderPassLate(VkDevice Device, VkFormat ColorFormat, VkFormat NormalsFormat, VkFormat MaterialFormat, VkFormat VelocityFormat, VkFormat LinearDepthFormat, VkFormat DepthFormat)
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
	Attachments[4].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

VkPipeline SGBufferVoxelRenderPass::CreateGraphicsPipelineEarly(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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

VkPipeline SGBufferVoxelRenderPass::CreateGraphicsPipelineLate(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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