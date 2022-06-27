struct SForwardVoxelRenderPass
{
public:
    static SForwardVoxelRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SBuffer& DrawBuffer, const SBuffer& VoxelsBuffer, VkSampler NoiseSampler, const SImage& NoiseTexture, const SBuffer* PointLightsBuffers, const SBuffer* LightBuffers, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage);
    void RenderEarly(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, uint32_t PointLightCount, vec3 AmbientColor, EAOQuality AOQuality, uint32_t FrameID);
    void RenderLate(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, uint32_t PointLightCount, EAOQuality AOQuality, uint32_t FrameID);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage);
	void HandleSampleMSAAChange(const SVulkanContext& Vulkan, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage);

private:
    VkPipeline PipelineEarly[AOQuality_Count], PipelineLate[AOQuality_Count];
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPassEarly, RenderPassLate;
    VkFramebuffer FramebufferEarly, FramebufferLate;

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;

private:
    static VkRenderPass CreateRenderPassEarly(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount);
	static VkRenderPass CreateRenderPassLate(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount);
	static VkPipeline CreateGraphicsPipelineEarly(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount, const int32_t SampleCountAO);
	static VkPipeline CreateGraphicsPipelineLate(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount, const int32_t SampleCountAO);
};

struct SForwardRenderVoxPushConstants
{
    uint32_t FrameNumber;
	uint32_t PointLightCount;
};

SForwardVoxelRenderPass SForwardVoxelRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, 
                                              			const SBuffer* CameraBuffers, const SBuffer& DrawBuffer, const SBuffer& VoxelsBuffer, VkSampler NoiseSampler, const SImage& NoiseTexture, const SBuffer* PointLightsBuffers, const SBuffer* LightBuffers,
                                              			const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage)
{
    VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DrawsBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayoutBinding VoxelsBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding NoiseBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding LightBinding = CreateDescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding PointLightsBinding = CreateDescriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding, DrawsBinding, VoxelsBinding, NoiseBinding, LightBinding, PointLightsBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[FramesInFlight] = {};
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
	{
		DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DrawBuffer, DrawBuffer.Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VoxelsBuffer, VoxelsBuffer.Size);
    	UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NoiseSampler, NoiseTexture.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LightBuffers[I], LightBuffers[I].Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, PointLightsBuffers[I], PointLightsBuffers[I].Size);
	}

    VkRenderPass RenderPassEarly = CreateRenderPassEarly(Vulkan.Device, HDRTargetImage.Format, Vulkan.DepthFormat, Vulkan.SampleCountMSAA);
    VkRenderPass RenderPassLate = CreateRenderPassLate(Vulkan.Device, HDRTargetImage.Format, Vulkan.DepthFormat, Vulkan.SampleCountMSAA);

	VkImageView FramebufferAttachments[] = { HDRTargetImage.View, LinearDepthImage.View, VelocityImage.View, DepthImage.View };
    VkFramebuffer FramebufferEarly = CreateFramebuffer(Vulkan.Device, RenderPassEarly, FramebufferAttachments, ArrayCount(FramebufferAttachments), HDRTargetImage.Width, HDRTargetImage.Height);
    VkFramebuffer FramebufferLate = CreateFramebuffer(Vulkan.Device, RenderPassLate, FramebufferAttachments, ArrayCount(FramebufferAttachments), HDRTargetImage.Width, HDRTargetImage.Height);

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Voxel.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\Voxel.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SForwardRenderVoxPushConstants), VK_SHADER_STAGE_FRAGMENT_BIT);

    SForwardVoxelRenderPass ForwardRenderPass = {};
	for (uint32_t I = 0; I < AOQuality_Count; I++)
	{
		const int32_t SampleCountAO = 1 << I;
		ForwardRenderPass.PipelineEarly[I] = CreateGraphicsPipelineEarly(Vulkan.Device, RenderPassEarly, PipelineLayout, VShader, FShader, Vulkan.SampleCountMSAA, SampleCountAO);
		ForwardRenderPass.PipelineLate[I] = CreateGraphicsPipelineLate(Vulkan.Device, RenderPassLate, PipelineLayout, VShader, FShader, Vulkan.SampleCountMSAA, SampleCountAO);
	}

    ForwardRenderPass.PipelineLayout = PipelineLayout;
    ForwardRenderPass.RenderPassEarly = RenderPassEarly;
    ForwardRenderPass.RenderPassLate = RenderPassLate;
    ForwardRenderPass.FramebufferEarly = FramebufferEarly;
    ForwardRenderPass.FramebufferLate = FramebufferLate;
    ForwardRenderPass.DescrSetLayout = DescrSetLayout;
    ForwardRenderPass.VShader = VShader;
    ForwardRenderPass.FShader = FShader;
	memcpy(ForwardRenderPass.DescrSets, DescrSets, sizeof(ForwardRenderPass.DescrSets));

    return ForwardRenderPass;
}

void SForwardVoxelRenderPass::RenderEarly(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, uint32_t PointLightCount, vec3 AmbientColor, EAOQuality AOQuality, uint32_t FrameID)
{
	BEGIN_GPU_PROFILER_BLOCK("VOXEL_RENDER_EARLY", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkClearValue ClearColorValue = { AmbientColor.x, AmbientColor.y, AmbientColor.z };
	VkClearValue ClearLinearDepthValue = { 1.0f };
	VkClearValue ClearVelocityValue = { 0.0f, 0.0f };
	VkClearValue ClearDepthValue = { 1.0f, 0.0f };
	VkClearValue ClearValues[] = { ClearColorValue, ClearLinearDepthValue, ClearVelocityValue, ClearDepthValue };
	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPassEarly;
	RenderPassBeginInfo.framebuffer = FramebufferEarly;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	RenderPassBeginInfo.clearValueCount = ArrayCount(ClearValues);
	RenderPassBeginInfo.pClearValues = ClearValues;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineEarly[AOQuality]);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	SForwardRenderVoxPushConstants PushConstants = { FrameID % 8, PointLightCount };
	vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SForwardRenderVoxPushConstants), &PushConstants);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
				
	Vulkan.vkCmdDrawIndexedIndirectCount(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, CountBuffer.Buffer, 0, ObjectsCount, sizeof(VkDrawIndexedIndirectCommand));

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("VOXEL_RENDER_EARLY", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SForwardVoxelRenderPass::RenderLate(const SVulkanContext& Vulkan, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, uint32_t ObjectsCount, uint32_t PointLightCount, EAOQuality AOQuality, uint32_t FrameID)
{
	BEGIN_GPU_PROFILER_BLOCK("VOXEL_RENDER_LATE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPassLate;
	RenderPassBeginInfo.framebuffer = FramebufferLate;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLate[AOQuality]);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	SForwardRenderVoxPushConstants PushConstants = { FrameID % 8, PointLightCount };
	vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SForwardRenderVoxPushConstants), &PushConstants);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
				
	Vulkan.vkCmdDrawIndexedIndirectCount(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, CountBuffer.Buffer, 0, ObjectsCount, sizeof(VkDrawIndexedIndirectCommand));

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("VOXEL_RENDER_LATE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SForwardVoxelRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage)
{
	vkDestroyFramebuffer(Vulkan.Device, FramebufferEarly, 0);
	vkDestroyFramebuffer(Vulkan.Device, FramebufferLate, 0);
	
	VkImageView FramebufferAttachments[] = { HDRTargetImage.View, LinearDepthImage.View, VelocityImage.View, DepthImage.View };
    FramebufferEarly = CreateFramebuffer(Vulkan.Device, RenderPassEarly, FramebufferAttachments, ArrayCount(FramebufferAttachments), HDRTargetImage.Width, HDRTargetImage.Height);
    FramebufferLate = CreateFramebuffer(Vulkan.Device, RenderPassLate, FramebufferAttachments, ArrayCount(FramebufferAttachments), HDRTargetImage.Width, HDRTargetImage.Height);
}

void SForwardVoxelRenderPass::HandleSampleMSAAChange(const SVulkanContext& Vulkan, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage)
{
	for (uint32_t I = 0; I < AOQuality_Count; I++)
	{
		vkDestroyPipeline(Vulkan.Device, PipelineEarly[I], 0);
		vkDestroyPipeline(Vulkan.Device, PipelineLate[I], 0);
	}

	vkDestroyFramebuffer(Vulkan.Device, FramebufferEarly, 0);
	vkDestroyFramebuffer(Vulkan.Device, FramebufferLate, 0);

	vkDestroyRenderPass(Vulkan.Device, RenderPassEarly, 0);
	vkDestroyRenderPass(Vulkan.Device, RenderPassLate, 0);

	RenderPassEarly = CreateRenderPassEarly(Vulkan.Device, HDRTargetImage.Format, Vulkan.DepthFormat, Vulkan.SampleCountMSAA);
    RenderPassLate = CreateRenderPassLate(Vulkan.Device, HDRTargetImage.Format, Vulkan.DepthFormat, Vulkan.SampleCountMSAA);

	VkImageView FramebufferAttachments[] = { HDRTargetImage.View, LinearDepthImage.View, VelocityImage.View, DepthImage.View };
    FramebufferEarly = CreateFramebuffer(Vulkan.Device, RenderPassEarly, FramebufferAttachments, ArrayCount(FramebufferAttachments), HDRTargetImage.Width, HDRTargetImage.Height);
    FramebufferLate = CreateFramebuffer(Vulkan.Device, RenderPassLate, FramebufferAttachments, ArrayCount(FramebufferAttachments), HDRTargetImage.Width, HDRTargetImage.Height);

	for (uint32_t I = 0; I < AOQuality_Count; I++)
	{
		const int32_t SampleCountAO = 1 << I;
		PipelineEarly[I] = CreateGraphicsPipelineEarly(Vulkan.Device, RenderPassEarly, PipelineLayout, VShader, FShader, Vulkan.SampleCountMSAA, SampleCountAO);
		PipelineLate[I] = CreateGraphicsPipelineLate(Vulkan.Device, RenderPassLate, PipelineLayout, VShader, FShader, Vulkan.SampleCountMSAA, SampleCountAO);
	}
}

VkRenderPass SForwardVoxelRenderPass::CreateRenderPassEarly(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount)
{
	VkAttachmentDescription Attachments[4] = {};

	Attachments[0].format = ColorFormat;
	Attachments[0].samples = SampleCount;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	Attachments[1].format = VK_FORMAT_R32_SFLOAT;
	Attachments[1].samples = SampleCount;
	Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[2].format = VK_FORMAT_R16G16_SFLOAT;
	Attachments[2].samples = SampleCount;
	Attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[3].format = DepthFormat;
	Attachments[3].samples = SampleCount;
	Attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	Attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference LinearDepthAttachment = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference VelocityAttachment = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference DepthAttachment = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkAttachmentReference ColorAttachments[] = { ColorAttachment, LinearDepthAttachment, VelocityAttachment };

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

VkRenderPass SForwardVoxelRenderPass::CreateRenderPassLate(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount)
{
	VkAttachmentDescription Attachments[4] = {};

	Attachments[0].format = ColorFormat;
	Attachments[0].samples = SampleCount;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[1].format = VK_FORMAT_R32_SFLOAT;
	Attachments[1].samples = SampleCount;
	Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	Attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	Attachments[2].format = VK_FORMAT_R16G16_SFLOAT;
	Attachments[2].samples = SampleCount;
	Attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[3].format = DepthFormat;
	Attachments[3].samples = SampleCount;
	Attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[3].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	Attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference LinearDepthAttachment = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference VelocityAttachment = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference DepthAttachment = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkAttachmentReference ColorAttachments[] = { ColorAttachment, LinearDepthAttachment, VelocityAttachment };

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

VkPipeline SForwardVoxelRenderPass::CreateGraphicsPipelineEarly(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount, const int32_t SampleCountAO)
{
	VkSpecializationMapEntry SpecializationEntry = { 0, 0, sizeof(int32_t) };
	VkSpecializationInfo SpecializationInfo = { 1, &SpecializationEntry, sizeof(int32_t), &SampleCountAO };

	VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
	ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ShaderStages[0].module = VS;
	ShaderStages[0].pName = "main";
	ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ShaderStages[1].module = FS;
	ShaderStages[1].pName = "main";
	ShaderStages[1].pSpecializationInfo = &SpecializationInfo;

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
	MultisampleStateInfo.rasterizationSamples = SampleCount;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateInfo.depthTestEnable = VK_TRUE;
	DepthStencilStateInfo.depthWriteEnable = VK_TRUE;
	DepthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	DepthStencilStateInfo.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorAttachmentStates[3] = {};
	ColorAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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

VkPipeline SForwardVoxelRenderPass::CreateGraphicsPipelineLate(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount, const int32_t SampleCountAO)
{
	VkSpecializationMapEntry SpecializationEntry = { 0, 0, sizeof(int32_t) };
	VkSpecializationInfo SpecializationInfo = { 1, &SpecializationEntry, sizeof(int32_t), &SampleCountAO };

	VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
	ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ShaderStages[0].module = VS;
	ShaderStages[0].pName = "main";
	ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ShaderStages[1].module = FS;
	ShaderStages[1].pName = "main";
	ShaderStages[1].pSpecializationInfo = &SpecializationInfo;

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
	MultisampleStateInfo.rasterizationSamples = SampleCount;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateInfo.depthTestEnable = VK_TRUE;
	DepthStencilStateInfo.depthWriteEnable = VK_TRUE;
	DepthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	DepthStencilStateInfo.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorAttachmentStates[3] = {};
	ColorAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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