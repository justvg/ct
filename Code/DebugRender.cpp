enum EDebugDrawType
{
	DebugDraw_Point,
	DebugDraw_Box,
	DebugDraw_Sphere,
	DebugDraw_Circle,
	DebugDraw_Arrow,
};

struct SDebugDraw
{
	EDebugDrawType Type;
	
	union 
	{
		// Point
		struct SPoint
		{
			vec3 Pos;
			float Size;
			vec3 Color;
		};
		SPoint Point;

		// Box
		struct SBox
		{
			vec3 Center;
			vec3 Extent;
			vec3 Color;
			vec4 Rotation;
			vec3 Offset;
			bool bSolid;
		};
		SBox Box;

		// Sphere
		struct SSphere
		{
			vec3 Center;
			vec3 Dim;
			vec3 Color;
		};
		SSphere Sphere;

		// Circle
		struct SCircle
		{
			vec3 Center;
			float Radius;
			vec3 Color;
		};
		SCircle Circle;

		// Arrow
		struct SArrow
		{
			vec3 Pos;
			vec3 Size;
			vec4 Rotation;
			vec3 Color;
		};
		SArrow Arrow;
	};
};

struct SDebugRenderPass
{
public:
    static SDebugRenderPass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SImage&FinalImage);
    void Render(const SVulkanContext& Vulkan);
	void UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& FinalImage);

	void DrawDebugPoint(vec3 Pos, float Size, vec3 Color);
	void DrawDebugBox(vec3 Center, vec3 Extent, vec3 Color, vec4 Rotation = Vec4(0.0f), vec3 Offset = Vec3(0.0f), bool bSolid = false);
	void DrawDebugSphere(vec3 Center, float Radius, vec3 Color);
	void DrawDebugSphere(vec3 Center, vec3 Dim, vec3 Color);
	void DrawDebugCircle(vec3 Center, float Radius, vec3 Color);
	void DrawDebugArrow(vec3 Pos, vec3 Size, vec3 Color, vec4 Rotation = Vec4(0.0f));

	static Rect GetArrowAABB() { return ArrowAABB; }

	static uint32_t GetCircleIndexCount() { return CircleIndexCount; }
	static uint32_t* GetCircleIndices() { return CircleIndices; }
	static SVertex* GetCircleVertices() { return CircleVertices; }

private:
#ifndef ENGINE_RELEASE
    VkPipeline PipelineWireframe;
    VkPipeline PipelineSolid;
    VkPipelineLayout PipelineLayout;

    VkRenderPass RenderPass;
    VkFramebuffer Framebuffer;

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule VShader;
    VkShaderModule FShader;

	const static uint32_t DebugDrawMaxCount = LevelDimZ*LevelDimY*LevelDimX + 128;
	uint32_t DebugDrawCount;
	static SDebugDraw* DebugDraws; 
#endif

	static uint32_t BoxIndexCount;
	static SBuffer BoxVertexBuffer;
	static SBuffer BoxIndexBuffer;

	static uint32_t SphereIndexCount;
	static SBuffer SphereVertexBuffer;
	static SBuffer SphereIndexBuffer;

	const static uint32_t CircleSectorCount = 64;
	const static uint32_t CircleVertexCount = 2 * CircleSectorCount;
	const static uint32_t CircleIndexCount = 6 * CircleSectorCount;
	static SVertex CircleVertices[CircleVertexCount];
	static uint32_t CircleIndices[CircleIndexCount];

	static SBuffer CircleVertexBuffer;
	static SBuffer CircleIndexBuffer;

	static uint32_t ArrowVertexCount;
	static SBuffer ArrowVertexBuffer;
	static Rect ArrowAABB;

private:
    static VkRenderPass CreateRenderPass(VkDevice Device, VkFormat ColorFormat);
    static VkPipeline CreateGraphicsPipelineWireframe(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);
    static VkPipeline CreateGraphicsPipelineSolid(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS);

	static void InitializeBoxMesh(const SVulkanContext& Vulkan);
	static void InitializeSphereMesh(const SVulkanContext& Vulkan);
	static void InitializeCircleMesh(const SVulkanContext& Vulkan);
	static void InitializeArrowMesh(const SVulkanContext& Vulkan);
};

struct SDebugRenderPassPushConstants
{
	mat4 Model;
	vec4 Color;
};

#ifndef ENGINE_RELEASE
SDebugDraw* SDebugRenderPass::DebugDraws; 
#endif

uint32_t SDebugRenderPass::BoxIndexCount;
SBuffer SDebugRenderPass::BoxVertexBuffer;
SBuffer SDebugRenderPass::BoxIndexBuffer;

uint32_t SDebugRenderPass::SphereIndexCount;
SBuffer SDebugRenderPass::SphereVertexBuffer;
SBuffer SDebugRenderPass::SphereIndexBuffer;

SVertex SDebugRenderPass::CircleVertices[CircleVertexCount];
uint32_t SDebugRenderPass::CircleIndices[CircleIndexCount];
SBuffer SDebugRenderPass::CircleVertexBuffer;
SBuffer SDebugRenderPass::CircleIndexBuffer;

uint32_t SDebugRenderPass::ArrowVertexCount;
SBuffer SDebugRenderPass::ArrowVertexBuffer;
Rect SDebugRenderPass::ArrowAABB;

SDebugRenderPass SDebugRenderPass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SImage&FinalImage)
{
#ifndef ENGINE_RELEASE
	VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding };
	VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

	VkDescriptorSet DescrSets[FramesInFlight] = {};
	for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
	{
		DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
		UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
	}

	VkRenderPass RenderPass = CreateRenderPass(Vulkan.Device, FinalImage.Format);

    VkFramebuffer Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &FinalImage.View, 1, FinalImage.Width, FinalImage.Height);

    VkShaderModule VShader = LoadShader(Vulkan.Device, "Shaders\\Debug.vert.spv");
    VkShaderModule FShader = LoadShader(Vulkan.Device, "Shaders\\Debug.frag.spv");

	VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SDebugRenderPassPushConstants), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkPipeline PipelineWireframe = CreateGraphicsPipelineWireframe(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);
	VkPipeline PipelineSolid = CreateGraphicsPipelineSolid(Vulkan.Device, RenderPass, PipelineLayout, VShader, FShader);

	InitializeBoxMesh(Vulkan);
	InitializeSphereMesh(Vulkan);
	InitializeCircleMesh(Vulkan);
	InitializeArrowMesh(Vulkan);

	SDebugRenderPass DebugRenderPass = {};
	DebugRenderPass.PipelineWireframe = PipelineWireframe;
	DebugRenderPass.PipelineSolid = PipelineSolid;
    DebugRenderPass.PipelineLayout = PipelineLayout;
    DebugRenderPass.RenderPass = RenderPass;
	DebugRenderPass.Framebuffer = Framebuffer;
    DebugRenderPass.DescrSetLayout = DescrSetLayout;
    DebugRenderPass.VShader = VShader;
    DebugRenderPass.FShader = FShader;
    memcpy(DebugRenderPass.DescrSets, DescrSets, sizeof(DebugRenderPass.DescrSets));

	DebugDraws = (SDebugDraw*) malloc(DebugDrawMaxCount * sizeof(SDebugDraw));
	Assert(DebugDraws);
#else
	SDebugRenderPass DebugRenderPass = {};
#endif

	return DebugRenderPass;
}

void SDebugRenderPass::Render(const SVulkanContext& Vulkan)
{
#ifndef ENGINE_RELEASE
	BEGIN_GPU_PROFILER_BLOCK("DEBUG_RENDER", Vulkan.CommandBuffer, Vulkan.FrameInFlight);

	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = Framebuffer;
	RenderPassBeginInfo.renderArea.extent.width = Vulkan.Width;
	RenderPassBeginInfo.renderArea.extent.height = Vulkan.Height;
	vkCmdBeginRenderPass(Vulkan.CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineWireframe);
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);
	
	vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineSolid);
	vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

	bool bWireframePipelineBound = false;
	for (uint32_t I = 0; I < DebugDrawCount; I++)
	{
		SDebugDraw& DebugDraw = DebugDraws[I];

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;
		SBuffer VertexBuffer = {}, IndexBuffer = {};
		mat4 Model = {};
		vec4 Color = {};
		switch (DebugDraw.Type)
		{
			case DebugDraw_Point:
			{
				IndexCount = SphereIndexCount;
				VertexBuffer = SphereVertexBuffer;
				IndexBuffer = SphereIndexBuffer;
				Model = Translation(DebugDraw.Point.Pos) * Scaling(DebugDraw.Point.Size);
				Color = Vec4(DebugDraw.Point.Color, 1.0f);

				if (!bWireframePipelineBound)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineWireframe);
					bWireframePipelineBound = true;
				}
			} break;

			case DebugDraw_Box:
			{
				IndexCount = BoxIndexCount;
				VertexBuffer = BoxVertexBuffer;
				IndexBuffer = BoxIndexBuffer;
				Model = Translation(DebugDraw.Box.Center) * Rotation(DebugDraw.Box.Rotation.w, Normalize(DebugDraw.Box.Rotation.xyz)) * Translation(DebugDraw.Box.Offset) * Scaling(DebugDraw.Box.Extent);
				Color = Vec4(DebugDraw.Box.Color, 1.0f);

				if (DebugDraw.Box.bSolid)
				{
					if (bWireframePipelineBound)
					{
						vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineSolid);
						bWireframePipelineBound = false;
					}
				}
				else
				{
					if (!bWireframePipelineBound)
					{
						vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineWireframe);
						bWireframePipelineBound = true;
					}
				}
			} break;

			case DebugDraw_Sphere:
			{
				IndexCount = SphereIndexCount;
				VertexBuffer = SphereVertexBuffer;
				IndexBuffer = SphereIndexBuffer;
				Model = Translation(DebugDraw.Sphere.Center) * Scaling(DebugDraw.Sphere.Dim);
				Color = Vec4(DebugDraw.Sphere.Color, 1.0f);

				if (!bWireframePipelineBound)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineWireframe);
					bWireframePipelineBound = true;
				}
			} break;

			case DebugDraw_Circle:
			{
				IndexCount = CircleIndexCount;
				VertexBuffer = CircleVertexBuffer;
				IndexBuffer = CircleIndexBuffer;
				Model = Translation(DebugDraw.Circle.Center) * Scaling(Vec3(DebugDraw.Circle.Radius, 1.0f, DebugDraw.Circle.Radius));
				Color = Vec4(DebugDraw.Circle.Color, 1.0f);

				if (bWireframePipelineBound)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineSolid);
					bWireframePipelineBound = false;
				}
			} break;
			
			case DebugDraw_Arrow:
			{
				VertexCount = ArrowVertexCount;
				VertexBuffer = ArrowVertexBuffer;
				Model = Translation(DebugDraw.Arrow.Pos) * Rotation(DebugDraw.Arrow.Rotation.w, Normalize(DebugDraw.Arrow.Rotation.xyz)) * Scaling(Vec3(1.0f, 1.0f, 1.0f));
				Color = Vec4(DebugDraw.Arrow.Color, 1.0f);

				if (bWireframePipelineBound)
				{
					vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineSolid);
					bWireframePipelineBound = false;
				}
			} break;
		}

		SDebugRenderPassPushConstants PushConstants = { Model, Color };
		vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SDebugRenderPassPushConstants), &PushConstants);

		VkDeviceSize Offset = 0;
		vkCmdBindVertexBuffers(Vulkan.CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Offset);

		if (IndexCount > 0)
		{
			vkCmdBindIndexBuffer(Vulkan.CommandBuffer, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(Vulkan.CommandBuffer, IndexCount, 1, 0, 0, 0);
		}
		else
		{
			Assert(VertexCount > 0);
			vkCmdDraw(Vulkan.CommandBuffer, VertexCount, 1, 0, 0);
		}
	}
	DebugDrawCount = 0;

	vkCmdEndRenderPass(Vulkan.CommandBuffer);

	END_GPU_PROFILER_BLOCK("DEBUG_RENDER", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
#endif
}

void SDebugRenderPass::UpdateAfterResize(const SVulkanContext& Vulkan, const SImage& FinalImage)
{
#ifndef ENGINE_RELEASE
	vkDestroyFramebuffer(Vulkan.Device, Framebuffer, 0);
	Framebuffer = CreateFramebuffer(Vulkan.Device, RenderPass, &FinalImage.View, 1, FinalImage.Width, FinalImage.Height);
#endif
}

void SDebugRenderPass::DrawDebugPoint(vec3 Pos, float Size, vec3 Color)
{
#ifndef ENGINE_RELEASE
	Assert(DebugDrawCount < DebugDrawMaxCount);

	SDebugDraw DebugDraw;
	DebugDraw.Type = DebugDraw_Point;
	DebugDraw.Point.Pos = Pos;
	DebugDraw.Point.Size = Size;
	DebugDraw.Point.Color = Color;

	DebugDraws[DebugDrawCount++] = DebugDraw;
#endif
}

void SDebugRenderPass::DrawDebugBox(vec3 Center, vec3 Extent, vec3 Color, vec4 Rotation, vec3 Offset, bool bSolid)
{
#ifndef ENGINE_RELEASE
	Assert(DebugDrawCount < DebugDrawMaxCount);
	SDebugDraw DebugDraw;
	DebugDraw.Type = DebugDraw_Box;
	DebugDraw.Box.Center = Center;
	DebugDraw.Box.Extent = Extent;
	DebugDraw.Box.Color = Color;
	DebugDraw.Box.Rotation = Rotation;
	DebugDraw.Box.Offset = Offset;
	DebugDraw.Box.bSolid = bSolid;

	DebugDraws[DebugDrawCount++] = DebugDraw;
#endif
}

void SDebugRenderPass::DrawDebugSphere(vec3 Center, float Radius, vec3 Color)
{
#ifndef ENGINE_RELEASE
	Assert(DebugDrawCount < DebugDrawMaxCount);
	SDebugDraw DebugDraw;
	DebugDraw.Type = DebugDraw_Sphere;
	DebugDraw.Sphere.Center = Center;
	DebugDraw.Sphere.Dim = Vec3(Radius);
	DebugDraw.Sphere.Color = Color;

	DebugDraws[DebugDrawCount++] = DebugDraw;
#endif
}

void SDebugRenderPass::DrawDebugSphere(vec3 Center, vec3 Dim, vec3 Color)
{
#ifndef ENGINE_RELEASE
	Assert(DebugDrawCount < DebugDrawMaxCount);
	SDebugDraw DebugDraw;
	DebugDraw.Type = DebugDraw_Sphere;
	DebugDraw.Sphere.Center = Center;
	DebugDraw.Sphere.Dim = 0.5f * Dim;
	DebugDraw.Sphere.Color = Color;

	DebugDraws[DebugDrawCount++] = DebugDraw;
#endif
}

void SDebugRenderPass::DrawDebugCircle(vec3 Center, float Radius, vec3 Color)
{
#ifndef ENGINE_RELEASE
	Assert(DebugDrawCount < DebugDrawMaxCount);
	SDebugDraw DebugDraw;
	DebugDraw.Type = DebugDraw_Circle;
	DebugDraw.Circle.Center = Center;
	DebugDraw.Circle.Radius = Radius;
	DebugDraw.Circle.Color = Color;

	DebugDraws[DebugDrawCount++] = DebugDraw;
#endif
}

void SDebugRenderPass::DrawDebugArrow(vec3 Pos, vec3 Size, vec3 Color, vec4 Rotation)
{
#ifndef ENGINE_RELEASE
	Assert(DebugDrawCount < DebugDrawMaxCount);
	SDebugDraw DebugDraw;
	DebugDraw.Type = DebugDraw_Arrow;
	DebugDraw.Arrow.Pos = Pos;
	DebugDraw.Arrow.Size = Size;
	DebugDraw.Arrow.Color = Color;
	DebugDraw.Arrow.Rotation = Rotation;

	DebugDraws[DebugDrawCount++] = DebugDraw;
#endif
}

VkRenderPass SDebugRenderPass::CreateRenderPass(VkDevice Device, VkFormat ColorFormat)
{
#ifndef ENGINE_RELEASE
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
#endif
	return 0;
}

VkPipeline SDebugRenderPass::CreateGraphicsPipelineWireframe(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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
	RasterizationStateInfo.polygonMode = VK_POLYGON_MODE_LINE;
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

VkPipeline SDebugRenderPass::CreateGraphicsPipelineSolid(VkDevice Device, VkRenderPass RenderPass, VkPipelineLayout PipelineLayout, VkShaderModule VS, VkShaderModule FS)
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

void SDebugRenderPass::InitializeBoxMesh(const SVulkanContext& Vulkan)
{
	SVertex Vertices[] = 
	{
		{ Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(0.5f, 0.5f, -0.5f),   Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(0.5f, -0.5f, -0.5f),  Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(-0.5f, 0.5f, -0.5f),  Vec3(0.0f, 0.0f, -1.0f) },

		{ Vec3(-0.5f, -0.5f, 0.5f),  Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.5f, -0.5f, 0.5f),   Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.5f, 0.5f, 0.5f),    Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(-0.5f, 0.5f, 0.5f),   Vec3(0.0f, 0.0f, 1.0f) },

		{ Vec3(-0.5f, 0.5f, 0.5f),   Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(-0.5f, 0.5f, -0.5f),  Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(-0.5f, -0.5f, -0.5f), Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(-0.5f, -0.5f, 0.5f),  Vec3(-1.0f, 0.0f, 0.0f) },

		{ Vec3(0.5f, 0.5f, 0.5f),    Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, -0.5f),  Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(0.5f, 0.5f, -0.5f),   Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, 0.5f),   Vec3(1.0f, 0.0f, 0.0f) },

		{ Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, -0.5f),  Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(0.5f, -0.5f, 0.5f),   Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(-0.5f, -0.5f, 0.5f),  Vec3(0.0f, -1.0f, 0.0f) },

		{ Vec3(-0.5f, 0.5f, -0.5f),  Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(0.5f, 0.5f, 0.5f),    Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(0.5f, 0.5f, -0.5f),   Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(-0.5f, 0.5f, 0.5f),   Vec3(0.0f, 1.0f, 0.0f) },
	};

	uint32_t Indices[] = 
	{
		0, 1, 2, 1, 0, 3, 4, 5, 6, 6, 7, 4, 8, 9, 10, 10, 11, 8, 12, 13, 14, 13, 12, 15, 16, 17, 18, 18, 19, 16, 20, 21, 22, 21, 20, 23
	};

	BoxIndexCount = ArrayCount(Indices);
	BoxVertexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(Vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	BoxIndexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(Indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(BoxVertexBuffer.Data, Vertices, sizeof(Vertices));
	memcpy(BoxIndexBuffer.Data, Indices, sizeof(Indices));
}

void SDebugRenderPass::InitializeSphereMesh(const SVulkanContext& Vulkan)
{
	const uint32_t StackCount = 16;
	const uint32_t SectorCount = 16;

	const uint32_t VertexCount = (StackCount - 1) * SectorCount + 2;
	const uint32_t IndexCount = 2 * 3 * SectorCount + (StackCount - 2) * SectorCount * 6;
	SVertex Vertices[VertexCount];
	uint32_t Indices[IndexCount];

	uint32_t VertexIndex = 0;
	Vertices[VertexIndex].Pos = Vec3(0.0f, 1.0f, 0.0f);
	Vertices[VertexIndex++].Normal = Vec3(0.0f, 1.0f, 0.0f);
	for (uint32_t Stack = 1; Stack < StackCount; Stack++)
	{
		const float Phi = 180.0f * (float(Stack) / StackCount);
		for (uint32_t Sector = 0; Sector < SectorCount; Sector++)
		{
			const float Theta = 360.0f * (float(Sector) / SectorCount);

			float X = Sin(Radians(Phi)) * Sin(Radians(Theta));
			float Y = Cos(Radians(Phi));
			float Z = -Sin(Radians(Phi)) * Cos(Radians(Theta));
			Vertices[VertexIndex].Pos = Vec3(X, Y, Z);
			Vertices[VertexIndex++].Normal = Normalize(Vertices[VertexIndex].Pos);
		}
	}
	Vertices[VertexIndex].Pos = Vec3(0.0f, -1.0f, 0.0f);
	Vertices[VertexIndex++].Normal = Vec3(0.0f, -1.0f, 0.0f);

	uint32_t IndexIndex = 0;
	for (uint32_t I = 0; I < SectorCount; I++)
	{
		Indices[IndexIndex++] = 0;
		Indices[IndexIndex++] = ((I + 2) == (SectorCount + 1)) ? (I + 2) % SectorCount : I + 2;
		Indices[IndexIndex++] = I + 1;
	}

	for (uint32_t Stack = 1; Stack < StackCount - 1; Stack++)
	{
		for (uint32_t Sector = 0; Sector < SectorCount; Sector++)
		{
			uint32_t A = (Stack - 1) * SectorCount + 1 + Sector;
			uint32_t B = (A % SectorCount + SectorCount * (Stack - 1)) + 1;
			uint32_t C = Stack * SectorCount + 1 + Sector;
			uint32_t D = (C  % SectorCount + SectorCount * Stack) + 1;

			Indices[IndexIndex++] = A;
			Indices[IndexIndex++] = B;
			Indices[IndexIndex++] = D;
			Indices[IndexIndex++] = A;
			Indices[IndexIndex++] = D;
			Indices[IndexIndex++] = C;
		}
	}

	for (uint32_t I = VertexCount - SectorCount - 1; I < VertexCount - 1; I++)
	{
		Indices[IndexIndex++] = I;
		Indices[IndexIndex++] = ((I + 1) == (VertexCount - 1)) ? VertexCount - SectorCount - 1 : I + 1;
		Indices[IndexIndex++] = VertexCount - 1;
	}

	SphereIndexCount = IndexCount;
	SphereVertexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(Vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	SphereIndexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(Indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(SphereVertexBuffer.Data, Vertices, sizeof(Vertices));
	memcpy(SphereIndexBuffer.Data, Indices, sizeof(Indices));
}

void SDebugRenderPass::InitializeCircleMesh(const SVulkanContext& Vulkan)
{
	uint32_t VertexIndex = 0;
	for (uint32_t Stack = 0; Stack < 2; Stack++)
	{
		const float Phi = 5.0f * Stack + 87.5f;
		for (uint32_t Sector = 0; Sector < CircleSectorCount; Sector++)
		{
			const float Theta = 360.0f * (float(Sector) / CircleSectorCount);

			float X = Sin(Radians(Phi)) * Sin(Radians(Theta));
			float Y = Cos(Radians(Phi));
			float Z = -Sin(Radians(Phi)) * Cos(Radians(Theta));
			CircleVertices[VertexIndex].Pos = Vec3(X, Y, Z);
			CircleVertices[VertexIndex++].Normal = Normalize(CircleVertices[VertexIndex].Pos);
		}
	}

	uint32_t IndexIndex = 0;
	for (uint32_t Sector = 0; Sector < CircleSectorCount; Sector++)
	{
		uint32_t A = Sector;
		uint32_t B = (Sector + 1) % CircleSectorCount;
		uint32_t C = A + CircleSectorCount;
		uint32_t D = B + CircleSectorCount;

		CircleIndices[IndexIndex++] = A;
		CircleIndices[IndexIndex++] = B;
		CircleIndices[IndexIndex++] = D;
		CircleIndices[IndexIndex++] = A;
		CircleIndices[IndexIndex++] = D;
		CircleIndices[IndexIndex++] = C;		
	}
	
	CircleVertexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(CircleVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	CircleIndexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(CircleIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(CircleVertexBuffer.Data, CircleVertices, sizeof(CircleVertices));
	memcpy(CircleIndexBuffer.Data, CircleIndices, sizeof(CircleIndices));
}

void SDebugRenderPass::InitializeArrowMesh(const SVulkanContext& Vulkan)
{
	uint32_t Indices[1];
	SVertex Vertices[2048];
	
	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
    if (LoadGLTF(ArrayCount(Vertices), Vertices, ArrayCount(Indices), Indices, "Models\\editor_arrow.gltf", VertexCount, IndexCount))
	{
		Assert(IndexCount == 0);

		ArrowVertexCount = VertexCount;
		ArrowVertexBuffer = CreateBuffer(Vulkan.MemoryAllocator, sizeof(Vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		memcpy(ArrowVertexBuffer.Data, Vertices, VertexCount*sizeof(SVertex));

		ArrowAABB.Min = Vec3(FLT_MAX);
		ArrowAABB.Max = Vec3(-FLT_MAX);
		for (uint32_t I = 0; I < VertexCount; I++)
		{
			if (Vertices[I].Pos.x < ArrowAABB.Min.x) ArrowAABB.Min.x = Vertices[I].Pos.x;
			if (Vertices[I].Pos.y < ArrowAABB.Min.y) ArrowAABB.Min.y = Vertices[I].Pos.y;
			if (Vertices[I].Pos.z < ArrowAABB.Min.z) ArrowAABB.Min.z = Vertices[I].Pos.z;
			if (Vertices[I].Pos.x > ArrowAABB.Max.x) ArrowAABB.Max.x = Vertices[I].Pos.x;
			if (Vertices[I].Pos.y > ArrowAABB.Max.y) ArrowAABB.Max.y = Vertices[I].Pos.y;
			if (Vertices[I].Pos.z > ArrowAABB.Max.z) ArrowAABB.Max.z = Vertices[I].Pos.z;
		}
	}
}