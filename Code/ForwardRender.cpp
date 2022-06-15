struct SForwardRenderPass
{
public:
    static SForwardRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SBuffer& VoxelsBuffer, VkSampler NoiseSampler, const SImage& NoiseTexture, const SBuffer* PointLightsBuffers, const SBuffer* LightBuffers, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage);
    void Render(const SVulkanContext& Vulkan, SEntity* Entities, uint32_t EntityCount, const SCamera& Camera, const SGeometry& Geometry, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, uint32_t PointLightCount, uint32_t FrameID, bool bGameMode, STempMemoryArena* MemoryArena);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage);

private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffer;

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;
	
	VkPipeline PipelineTransp;
    VkRenderPass RenderPassTransp;
    VkFramebuffer FramebufferTransp;
    VkShaderModule FShaderTransp;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount);
	static VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount);

    static VkRenderPass CreateRenderPassTransp(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount);
	static VkPipeline CreateGraphicsPipelineTransp(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount);
};

struct SForwardRenderPushConstants
{
	vec4 Position; // w - unused
	vec4 Scale; // w - unused
	quat Orientation;
	vec4 Color; // w - unused
	vec4 Offset; // w contains point lights count

	vec4 PrevPosition; // w - unused
	quat PrevOrientation;

    uint32_t FrameNumber;
	uint32_t FPWeaponDepthTest;
};

SForwardRenderPass SForwardRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, 
                                              const SBuffer* CameraBuffers, const SBuffer& VoxelsBuffer, VkSampler NoiseSampler, const SImage& NoiseTexture, const SBuffer* PointLightsBuffers, const SBuffer* LightBuffers,
                                              const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage)
{
    VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding VoxelsBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding NoiseBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding LightBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding PointLightsBinding = CreateDescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding, VoxelsBinding, NoiseBinding, LightBinding, PointLightsBinding  };
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
	}

    VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, HDRTargetImage.Format, Vulkan.DepthFormat, Vulkan.SampleCountMSAA);

	VkImageView FramebufferAttachments[] = { HDRTargetImage.View, LinearDepthImage.View, VelocityImage.View, DepthImage.View };
    VkFramebuffer Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), Vulkan.Width, Vulkan.Height);

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Mesh.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\Mesh.frag.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SForwardRenderPushConstants), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipeline Pipeline = CreateGraphicsPipeline(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader, Vulkan.SampleCountMSAA);

    VkRenderPass RenderPassTransp = CreateRenderPassTransp(Vulkan.Device, HDRTargetImage.Format, Vulkan.DepthFormat, Vulkan.SampleCountMSAA);

	VkImageView FramebufferAttachmentsTransp[] = { HDRTargetImage.View, DepthImage.View };
    VkFramebuffer FramebufferTransp = CreateFramebuffer(Vulkan.Device, RenderPassTransp, FramebufferAttachmentsTransp, ArrayCount(FramebufferAttachmentsTransp), Vulkan.Width, Vulkan.Height);

	VkShaderModule FShaderTransp = LoadShader(Vulkan.Device, "Shaders\\MeshTransparent.frag.spv");
	VkPipeline PipelineTransp = CreateGraphicsPipelineTransp(Vulkan.Device, RenderPassTransp, PipelineLayout, VShader, FShaderTransp, Vulkan.SampleCountMSAA);

    SForwardRenderPass ForwardRenderPass = {};
    ForwardRenderPass.Pipeline = Pipeline;
    ForwardRenderPass.PipelineLayout = PipelineLayout;
    ForwardRenderPass.RenderPass = RenderPass;
    ForwardRenderPass.Framebuffer = Framebuffer;
    ForwardRenderPass.DescrSetLayout = DescrSetLayout;
    ForwardRenderPass.VShader = VShader;
    ForwardRenderPass.FShader = FShader;
    ForwardRenderPass.FShaderTransp = FShaderTransp;
	ForwardRenderPass.PipelineTransp = PipelineTransp;
    ForwardRenderPass.RenderPassTransp = RenderPassTransp;
    ForwardRenderPass.FramebufferTransp = FramebufferTransp;
    memcpy(ForwardRenderPass.DescrSets, DescrSets, sizeof(ForwardRenderPass.DescrSets));

    return ForwardRenderPass;
}

void SForwardRenderPass::Render(const SVulkanContext& Vulkan, SEntity* Entities, uint32_t EntityCount, const SCamera& Camera, const SGeometry& Geometry, const SBuffer& VertexBuffer, const SBuffer& IndexBuffer, uint32_t PointLightCount, uint32_t FrameID, bool bGameMode, STempMemoryArena* MemoryArena)
{
	BEGIN_GPU_PROFILER_BLOCK("RENDER_ENTITIES", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	SEntity* RenderEntities = (SEntity*) PushMemory(MemoryArena->Arena, sizeof(SEntity) * EntityCount);
	memcpy(RenderEntities, Entities, sizeof(SEntity) * EntityCount);

	for (uint32_t I = 0; I < EntityCount; I++)
	{
		RenderEntities[I].DistanceToCam = Length(RenderEntities[I].Pos - Camera.Pos); 
	}

	// Bubble sort 
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		bool bSwapped  = false;
		for (uint32_t J = 0; J < EntityCount - I - 1; J++)
		{
			if ((RenderEntities[J].Alpha == 1.0f) && (RenderEntities[J + 1].Alpha == 1.0f))
			{
				if (RenderEntities[J].DistanceToCam > RenderEntities[J + 1].DistanceToCam)
				{
					SEntity Temp = RenderEntities[J];
					RenderEntities[J] = RenderEntities[J + 1];
					RenderEntities[J + 1] = Temp;

					bSwapped = true;
				}
			} 
			else if ((RenderEntities[J].Alpha != 1.0f) && (RenderEntities[J + 1].Alpha != 1.0f))
			{
				if (RenderEntities[J].DistanceToCam < RenderEntities[J + 1].DistanceToCam)
				{
					SEntity Temp = RenderEntities[J];
					RenderEntities[J] = RenderEntities[J + 1];
					RenderEntities[J + 1] = Temp;

					bSwapped = true;
				}
			}
			else if (RenderEntities[J].Alpha != 1.0f)
			{
				SEntity Temp = RenderEntities[J];
				RenderEntities[J] = RenderEntities[J + 1];
				RenderEntities[J + 1] = Temp;

				bSwapped = true;
			}
		}

		if (!bSwapped)
        {
			break;
        }
	}

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffer;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.Width;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	VkDeviceSize Offset = 0;
	vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);
	vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	bool bFirstTranspEncountered = false;
	SForwardRenderPushConstants PushConstants = {};
	PushConstants.FrameNumber = FrameID % 8;
	for (uint32_t I = 0; I < EntityCount; I++)
	{
		const SEntity& Entity = RenderEntities[I];

		if ((Entity.Alpha < 1.0f) && !bFirstTranspEncountered)
		{
			bFirstTranspEncountered = true;
			
			vkCmdEndRenderPass(Vulkan.CommandBuffer);

			VkRenderPassBeginInfo RenderPassTranspBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			RenderPassTranspBeginInfo.renderPass = RenderPassTransp;
			RenderPassTranspBeginInfo.framebuffer = FramebufferTransp;
			RenderPassTranspBeginInfo.renderArea.extent.width = Vulkan.Width;
			RenderPassTranspBeginInfo.renderArea.extent.height = Vulkan.Height;
			vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassTranspBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineTransp);
			vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);
		}

		PushConstants.Position = Vec4(Entity.Pos, 0.0f);
		PushConstants.Scale = Vec4(Entity.Scale * Entity.Dim, 0.0f);
		PushConstants.Orientation = EulerToQuat(Entity.Orientation.xyz);
		PushConstants.Color = Vec4(Entity.Color, Entity.Alpha);
		PushConstants.Offset = Vec4(0.0f, 0.0f, 0.0f, float(PointLightCount));
		PushConstants.PrevPosition = Vec4(Entity.PrevPos, 0.0f);
		PushConstants.PrevOrientation = EulerToQuat(Entity.PrevOrientation.xyz);
		PushConstants.FPWeaponDepthTest = true;

		switch (Entity.Type)
		{
			case Entity_Hero:
			{
				if (bGameMode)
				{
					// NOTE(georgii): These hacks are here so a player can always see the lamp.
					float CameraPitch = Clamp(Camera.Pitch, -89.0f, 50.0f);
					float CameraHead = Camera.Head;

					vec3 CameraDir;
					CameraDir.x = Cos(Radians(CameraPitch)) * Sin(Radians(CameraHead));
					CameraDir.y = Sin(Radians(CameraPitch));
					CameraDir.z = Cos(Radians(CameraPitch)) * Cos(Radians(CameraHead));
					CameraDir = Normalize(CameraDir);
					
					vec3 CameraRight = Normalize(Cross(CameraDir, Vec3(0.0f, 1.0f, 0.0f)));
					vec3 CameraUp = Cross(CameraRight, CameraDir);


					float CameraPrevPitch = Clamp(Camera.PrevPitch, -89.0f, 50.0f);
					float CameraPrevHead = Camera.PrevHead;
					
					vec3 CameraPrevDir;
					CameraPrevDir.x = Cos(Radians(CameraPrevPitch)) * Sin(Radians(CameraPrevHead));
					CameraPrevDir.y = Sin(Radians(CameraPrevPitch));
					CameraPrevDir.z = Cos(Radians(CameraPrevPitch)) * Cos(Radians(CameraPrevHead));
					CameraPrevDir = Normalize(CameraPrevDir);
					
					vec3 CameraPrevRight = Normalize(Cross(CameraPrevDir, Vec3(0.0f, 1.0f, 0.0f)));
					vec3 CameraPrevUp = Cross(CameraPrevRight, CameraPrevDir);

					PushConstants.Position.xyz += (Entity.LampOffset.x * Entity.Dim.x * CameraRight) + (Entity.LampOffset.z * CameraDir) + (Entity.LampOffset.y * CameraUp);
					PushConstants.Orientation = Quat(Vec3(0.0f, 1.0f, 0.0f), Camera.Head + Entity.LampRotationOffset.y) * Quat(Vec3(1.0f, 0.0f, 0.0f), -Camera.Pitch - Entity.LampRotationOffset.x);
					PushConstants.PrevPosition.xyz += (Entity.PrevLampOffset.x * Entity.Dim.x * CameraPrevRight) + (Entity.PrevLampOffset.z * CameraPrevDir) + (Entity.PrevLampOffset.y * CameraPrevUp);
					PushConstants.PrevOrientation = Quat(Vec3(0.0f, 1.0f, 0.0f), Camera.PrevHead + Entity.LampRotationOffset.y) * Quat(Vec3(1.0f, 0.0f, 0.0f), -Camera.PrevPitch - Entity.LampRotationOffset.x);
					PushConstants.Color.rgb *= 110.0f;
					PushConstants.FPWeaponDepthTest = false;
				}
				PushConstants.Scale = Vec4(Vec3(Entity.Scale), 0.0f);
			} break;

			case Entity_Torch:
			{
				PushConstants.Color.rgb *= 150.0f;
			} break;

			case Entity_Fireball:
			{
				PushConstants.Color.rgb *= 150.0f;
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
		vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SForwardRenderPushConstants), &PushConstants);
		vkCmdDrawIndexed(Vulkan.CommandBuffer, Mesh.IndexCount, 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);

		if (Entity.Type == Entity_Door)
		{
			PushConstants.Offset.xyz = Vec3(-0.3f * Entity.Scale * Entity.Dim.x, 0.0f, 0.0f);
			PushConstants.Scale = Hadamard(PushConstants.Scale, Vec4(0.15f, 0.15f, 1.03f, 0.0f));
			PushConstants.Color.rgb = IsEqual(Entity.CurrentColor, Entity.TargetColor) ? 150.0f * Entity.TargetColor : Entity.TargetColor;
			vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SForwardRenderPushConstants), &PushConstants);
			vkCmdDrawIndexed(Vulkan.CommandBuffer, Mesh.IndexCount, 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);

			PushConstants.Offset.xyz = Vec3(0.3f * Entity.Scale * Entity.Dim.x, 0.0f, 0.0f);
			PushConstants.Color.rgb = IsEqual(Entity.CurrentColor, Entity.TargetColor) ? 150.0f * Entity.CurrentColor : Entity.CurrentColor;
			vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SForwardRenderPushConstants), &PushConstants);
			vkCmdDrawIndexed(Vulkan.CommandBuffer, Mesh.IndexCount, 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);
		}
	}

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("RENDER_ENTITIES", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
}

void SForwardRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& HDRTargetImage, const SImage& LinearDepthImage, const SImage& VelocityImage, const SImage& DepthImage)
{
	vkDestroyFramebuffer(Vulkan.Device, Framebuffer, 0);
	vkDestroyFramebuffer(Vulkan.Device, FramebufferTransp, 0);
	
	VkImageView FramebufferAttachments[] = { HDRTargetImage.View, LinearDepthImage.View, VelocityImage.View, DepthImage.View };
    Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, FramebufferAttachments, ArrayCount(FramebufferAttachments), Vulkan.Width, Vulkan.Height);

	VkImageView FramebufferAttachmentsTransp[] = { HDRTargetImage.View, DepthImage.View };
    FramebufferTransp = CreateFramebuffer(Vulkan.Device, RenderPassTransp, FramebufferAttachmentsTransp, ArrayCount(FramebufferAttachmentsTransp), Vulkan.Width, Vulkan.Height);
}

VkRenderPass SForwardRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount)
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
	Attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

VkPipeline SForwardRenderPass::CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount)
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

VkRenderPass SForwardRenderPass::CreateRenderPassTransp(VkDevice Device, VkFormat ColorFormat, VkFormat DepthFormat, VkSampleCountFlagBits SampleCount)
{
	VkAttachmentDescription Attachments[2] = {};

	Attachments[0].format = ColorFormat;
	Attachments[0].samples = SampleCount;
	Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Attachments[1].format = DepthFormat;
	Attachments[1].samples = SampleCount;
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

VkPipeline SForwardRenderPass::CreateGraphicsPipelineTransp(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS, VkSampleCountFlagBits SampleCount)
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
	MultisampleStateInfo.rasterizationSamples = SampleCount;

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