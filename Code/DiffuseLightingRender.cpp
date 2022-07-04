struct SDiffuseLightingPass
{
public:
    static SDiffuseLightingPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SBuffer& VoxelsBuffer, VkSampler NoiseSampler, const SImage& NoiseTexture, const SBuffer* PointLightsBuffers, const SBuffer* LightBuffers, VkSampler PointEdgeSampler, const SImage& NormalsImage, const SImage& LinearDepthImage, const SImage* VelocityImages, VkSampler LinearEdgeSampler, const SImage& AlbedoImage, const SImage* DiffuseLightHistoryImages, const SImage& DiffuseLightImage, const SImage& DiffuseImage);
    void Render(const SVulkanContext& Vulkan, const SImage& DiffuseLightImage, const SImage* DiffuseLightHistoryImages, const SBuffer& QuadVertexBuffer, EAOQuality AOQuality, uint32_t PointLightCount, uint32_t FrameID, bool bSwapchainChanged);
	void UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler PointEdgeSampler, const SImage& NormalsImage, const SImage& LinearDepthImage, const SImage* DiffuseLightHistoryImages, const SImage& DiffuseLightImage, const SImage* VelocityImages, VkSampler LinearEdgeSampler, const SImage& AlbedoImage, const SImage& DiffuseImage);

private:
    VkPipeline Pipeline[AOQuality_Count];
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffer;

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;


	VkPipeline DenoisePipeline;
    VkPipelineLayout DenoisePipelineLayout;

	VkRenderPass DenoiseRenderPass;
	VkFramebuffer DenoiseFramebuffers[2];

	VkDescriptorSetLayout DenoiseDescrSetLayout;
    VkDescriptorSet DenoiseDescrSets[FramesInFlight];

    VkShaderModule DenoiseFShader;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
	static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, const int32_t SampleCountAO);

    static VkRenderPass CreateDenoiseRenderPass(VkDevice Device, VkFormat AmbientFormat, VkFormat DiffuseFormat);
	static VkPipeline CreateDenoiseGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
};

struct SDiffuseLightingPushConstants
{
    uint32_t FrameNumber;
	uint32_t PointLightCount;
};

SDiffuseLightingPass SDiffuseLightingPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool,
                                                  const SBuffer* CameraBuffers, const SBuffer& VoxelsBuffer, VkSampler NoiseSampler, const SImage& NoiseTexture, const SBuffer* PointLightsBuffers, const SBuffer* LightBuffers, VkSampler PointEdgeSampler, const SImage& NormalsImage, const SImage& LinearDepthImage, const SImage* VelocityImages, 
                                                  VkSampler LinearEdgeSampler, const SImage& AlbedoImage, const SImage* DiffuseLightHistoryImages, const SImage& DiffuseLightImage, const SImage& DiffuseImage)
{
    VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding VoxelsBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding NoiseBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding LightBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding PointLightsBinding = CreateDescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding NormalsBinding = CreateDescriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding LinearDepthBinding = CreateDescriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding, VoxelsBinding, NoiseBinding, LightBinding, PointLightsBinding, NormalsBinding, LinearDepthBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[FramesInFlight] = {};
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
	{
		DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VoxelsBuffer, VoxelsBuffer.Size);
    	UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NoiseSampler, NoiseTexture.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LightBuffers[I], LightBuffers[I].Size);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, PointLightsBuffers[I], PointLightsBuffers[I].Size);
    	UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, NormalsImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, LinearDepthImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

    VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, DiffuseLightImage.Format);
    VkFramebuffer Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &DiffuseLightImage.View, 1, DiffuseLightImage.Width, DiffuseLightImage.Height);
    
    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\FullscreenLighting.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\DiffuseLighting.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SDiffuseLightingPushConstants), VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipeline Pipeline[AOQuality_Count] = {};
	for (uint32_t I = 0; I < AOQuality_Count; I++)
	{
		const int32_t SampleCountAO = 1 << I;
		Pipeline[I] = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader, SampleCountAO);
	}


	VkDescriptorSetLayoutBinding DenoiseCameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DenoiseNoiseBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DenoiseCurrentBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DenoiseHistoryBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DenoiseVelocityBinding = CreateDescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DenoiseHistoryVelocityBinding = CreateDescriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding DenoiseAlbedoBinding = CreateDescriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DenoiseDescrSetLayoutBindings[] = { DenoiseCameraBinding, DenoiseNoiseBinding, DenoiseCurrentBinding, DenoiseHistoryBinding, DenoiseVelocityBinding, DenoiseHistoryVelocityBinding, DenoiseAlbedoBinding };
    VkDescriptorSetLayout DenoiseDescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DenoiseDescrSetLayoutBindings), DenoiseDescrSetLayoutBindings);

    VkDescriptorSet DenoiseDescrSets[FramesInFlight] = {};
    for (uint32_t I = 0; I < ArrayCount(DenoiseDescrSets); I++)
	{
		DenoiseDescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DenoiseDescrSetLayout);
    	UpdateDescriptorSetBuffer(Vulkan.Device, DenoiseDescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NoiseSampler, NoiseTexture.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, DiffuseLightImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, DiffuseLightHistoryImages[(I + 1) % ArrayCount(DenoiseDescrSets)].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[(I + 1) % 2].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, AlbedoImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	VkRenderPass DenoiseRenderPass = CreateDenoiseRenderPass(Vulkan.Device, DiffuseLightHistoryImages[0].Format, DiffuseImage.Format);
	VkFramebuffer DenoiseFramebuffers[2] = {};
	for (uint32_t I = 0; I < ArrayCount(DenoiseFramebuffers); I++)
	{
		VkImageView FramebufferAttachments[] = { DiffuseLightHistoryImages[I].View, DiffuseImage.View };
		DenoiseFramebuffers[I] = CreateFramebuffer(Vulkan.Device, DenoiseRenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), DiffuseLightHistoryImages[I].Width, DiffuseLightHistoryImages[I].Height);
	}

    VkShaderModule DenoiseFShader = LoadShader(Vulkan.Device, "Shaders\\DiffuseLightingDenoise.frag.spv");

    VkPipelineLayout DenoisePipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DenoiseDescrSetLayout, sizeof(SDiffuseLightingPushConstants), VK_SHADER_STAGE_FRAGMENT_BIT);
	VkPipeline DenoisePipeline = CreateDenoiseGraphicsPipeline(Vulkan.Device, DenoiseRenderPass, DenoisePipelineLayout, VShader, DenoiseFShader);

    SDiffuseLightingPass DiffuseLightingPass = {};
	memcpy(DiffuseLightingPass.Pipeline, Pipeline, sizeof(DiffuseLightingPass.Pipeline));
    DiffuseLightingPass.PipelineLayout = PipelineLayout;
    DiffuseLightingPass.RenderPass = RenderPass;
	DiffuseLightingPass.Framebuffer = Framebuffer;
    DiffuseLightingPass.DescrSetLayout = DescrSetLayout;
    DiffuseLightingPass.VShader = VShader;
    DiffuseLightingPass.FShader = FShader;

	DiffuseLightingPass.DenoisePipeline = DenoisePipeline;
    DiffuseLightingPass.DenoisePipelineLayout = DenoisePipelineLayout;
	DiffuseLightingPass.DenoiseRenderPass = DenoiseRenderPass;
	DiffuseLightingPass.DenoiseDescrSetLayout = DenoiseDescrSetLayout;
    DiffuseLightingPass.DenoiseFShader = DenoiseFShader;

	DiffuseLightingPass.DenoiseRenderPass = DenoiseRenderPass;
    memcpy(DiffuseLightingPass.DescrSets, DescrSets, sizeof(DiffuseLightingPass.DescrSets));
	memcpy(DiffuseLightingPass.DenoiseFramebuffers, DenoiseFramebuffers, sizeof(DiffuseLightingPass.DenoiseFramebuffers));
	memcpy(DiffuseLightingPass.DenoiseDescrSets, DenoiseDescrSets, sizeof(DiffuseLightingPass.DenoiseDescrSets));

    return DiffuseLightingPass;
}

void SDiffuseLightingPass::Render(const SVulkanContext& Vulkan, const SImage& DiffuseLightImage, const SImage* DiffuseLightHistoryImages, const SBuffer& QuadVertexBuffer, EAOQuality AOQuality, uint32_t PointLightCount, uint32_t FrameID, bool bSwapchainChanged)
{
	const uint32_t TargetIndex = FrameID % 2;
    const uint32_t PrevIndex = (TargetIndex + 1) % 2;

    BEGIN_GPU_PROFILER_BLOCK("DIFFUSE_LIGHTING", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffer;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline[AOQuality]);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[FrameID % 2], 0, 0);

	SDiffuseLightingPushConstants PushConstants = { FrameID % 8, PointLightCount };
	vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SDiffuseLightingPushConstants), &PushConstants);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &QuadVertexBuffer.Buffer, &Offset);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	VkImageMemoryBarrier DiffuseReadBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DiffuseLightImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &DiffuseReadBarrier);

	if ((FrameID == 0) || bSwapchainChanged)
	{
		VkImageMemoryBarrier DiffuseReadBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DiffuseLightHistoryImages[PrevIndex].Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &DiffuseReadBarrier);
	}

	END_GPU_PROFILER_BLOCK("DIFFUSE_LIGHTING", Vulkan.CommandBuffer, Vulkan.FrameInFlight);


	BEGIN_GPU_PROFILER_BLOCK("DIFFUSE_LIGHTING_DENOISE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
	
	VkRenderPassBeginInfo DenoiseRenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	DenoiseRenderPassBeginInfo.renderPass = DenoiseRenderPass;
	DenoiseRenderPassBeginInfo.framebuffer = DenoiseFramebuffers[TargetIndex];
	DenoiseRenderPassBeginInfo.renderArea.extent.width = Vulkan.InternalWidth;
	DenoiseRenderPassBeginInfo.renderArea.extent.height = Vulkan.InternalHeight;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &DenoiseRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DenoisePipeline);

	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DenoisePipelineLayout, 0, 1, &DenoiseDescrSets[FrameID % 2], 0, 0);
	vkCmdPushConstants(Vulkan.CommandBuffer, DenoisePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SDiffuseLightingPushConstants), &PushConstants);

	vkCmdDraw(Vulkan.CommandBuffer, 6, 1, 0, 0);
	
	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	VkImageMemoryBarrier DenoiseEndBarrier = CreateImageMemoryBarrier(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DiffuseLightHistoryImages[TargetIndex].Image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &DenoiseEndBarrier);

	END_GPU_PROFILER_BLOCK("DIFFUSE_LIGHTING_DENOISE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SDiffuseLightingPass::UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler PointEdgeSampler, const SImage& NormalsImage, const SImage& LinearDepthImage, const SImage* DiffuseLightHistoryImages, const SImage& DiffuseLightImage, const SImage* VelocityImages, VkSampler LinearEdgeSampler, const SImage& AlbedoImage, const SImage& DiffuseImage)
{
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
	{
    	UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, NormalsImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, LinearDepthImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	vkDestroyFramebuffer(Vulkan.Device, Framebuffer, 0);
	Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &DiffuseLightImage.View, 1, DiffuseLightImage.Width, DiffuseLightImage.Height);


	for (uint32_t I = 0; I < ArrayCount(DenoiseFramebuffers); I++)
	{
		vkDestroyFramebuffer(Vulkan.Device, DenoiseFramebuffers[I], 0);
		VkImageView FramebufferAttachments[] = { DiffuseLightHistoryImages[I].View, DiffuseImage.View };
		DenoiseFramebuffers[I] = CreateFramebuffer(Vulkan.Device, DenoiseRenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), DiffuseLightHistoryImages[I].Width, DiffuseLightHistoryImages[I].Height);
	}

	for (uint32_t I = 0; I < ArrayCount(DenoiseDescrSets); I++)
	{
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, DiffuseLightImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LinearEdgeSampler, DiffuseLightHistoryImages[(I + 1) % ArrayCount(DenoiseDescrSets)].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[I].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, VelocityImages[(I + 1) % 2].View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    	UpdateDescriptorSetImage(Vulkan.Device, DenoiseDescrSets[I], 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointEdgeSampler, AlbedoImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

VkRenderPass SDiffuseLightingPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
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

	VkAttachmentReference AmbientAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &AmbientAttachment;

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

VkRenderPass SDiffuseLightingPass::CreateDenoiseRenderPass(VkDevice Device, VkFormat AmbientFormat, VkFormat DiffuseFormat)
{
	VkAttachmentDescription Attachments[2] = {};

	Attachments[0].format = AmbientFormat;
	Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[1].format = DiffuseFormat;
	Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	Attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference AmbientAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference DiffuseAttachment = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentReference ColorAttachments[] = { AmbientAttachment, DiffuseAttachment };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = ArrayCount(ColorAttachments);
	Subpass.pColorAttachments = ColorAttachments;

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

VkPipeline SDiffuseLightingPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, const int32_t SampleCountAO)
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

VkPipeline SDiffuseLightingPass::CreateDenoiseGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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

	VkPipelineColorBlendAttachmentState ColorAttachmentStates[2] = {};
	ColorAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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