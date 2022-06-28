struct SVoxelCullingComputePass
{
public:
    static SVoxelCullingComputePass Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, const SBuffer* CameraBuffers, const SBuffer& DrawBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, const SBuffer& VisibilityBuffer, VkSampler DepthSampler, const SImage& DepthPyramidImage);
    void Dispatch(const SVulkanContext& Vulkan, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, const SImage& DepthPyramidImage, uint32_t ObjectsCount, uint32_t FrameID, bool bLate, bool bSwapchainChanged);
    void UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler DepthSampler, const SImage& DepthPyramidImage);

private:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    VkDescriptorSetLayout DescrSetLayout;
    VkDescriptorSet DescrSets[FramesInFlight];

    VkShaderModule Shader;
};

struct SVoxelCullingPushConstants
{
    uint32_t LateRender;
    uint32_t ObjectsCount;
    uint32_t MinMaxSampler;
};

SVoxelCullingComputePass SVoxelCullingComputePass::Create(const SVulkanContext& Vulkan, VkDescriptorPool DescrPool, 
                                                          const SBuffer* CameraBuffers, const SBuffer& DrawBuffer, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, const SBuffer& VisibilityBuffer,
                                                          VkSampler DepthSampler, const SImage& DepthPyramidImage)
{
    VkDescriptorSetLayoutBinding CameraBinding = CreateDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding DrawsBinding = CreateDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding CmdsBinding = CreateDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding CountBinding = CreateDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding VisibilityBinding = CreateDescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSetLayoutBinding DepthBinding = CreateDescriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

    VkDescriptorSetLayoutBinding DescrSetLayoutBindings[] = { CameraBinding, DrawsBinding, CmdsBinding, CountBinding, VisibilityBinding, DepthBinding };
    VkDescriptorSetLayout DescrSetLayout = CreateDescriptorSetLayout(Vulkan.Device, ArrayCount(DescrSetLayoutBindings), DescrSetLayoutBindings);

    VkDescriptorSet DescrSets[FramesInFlight] = {};
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
    {
        DescrSets[I] = CreateDescriptorSet(Vulkan.Device, DescrPool, DescrSetLayout);
        UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CameraBuffers[I], CameraBuffers[I].Size);
        UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DrawBuffer, DrawBuffer.Size);
        UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, IndirectBuffer, IndirectBuffer.Size);
        UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, CountBuffer, sizeof(uint32_t));
        UpdateDescriptorSetBuffer(Vulkan.Device, DescrSets[I], 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VisibilityBuffer, VisibilityBuffer.Size);
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DepthSampler, DepthPyramidImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    
    VkShaderModule Shader = LoadShader(Vulkan.Device, "Shaders\\VoxelCulling.comp.spv");

    VkPipelineLayout PipelineLayout = CreatePipelineLayout(Vulkan.Device, 1, &DescrSetLayout, sizeof(SVoxelCullingPushConstants), VK_SHADER_STAGE_COMPUTE_BIT);
    VkPipeline Pipeline = CreateComputePipeline(Vulkan.Device, PipelineLayout, Shader);

    SVoxelCullingComputePass CullingPass = {};
    CullingPass.Pipeline = Pipeline;
    CullingPass.PipelineLayout = PipelineLayout;
    CullingPass.DescrSetLayout = DescrSetLayout;
    CullingPass.Shader = Shader;
    memcpy(CullingPass.DescrSets, DescrSets, sizeof(CullingPass.DescrSets));

    return CullingPass;
}

void SVoxelCullingComputePass::Dispatch(const SVulkanContext& Vulkan, const SBuffer& IndirectBuffer, const SBuffer& CountBuffer, const SImage& DepthPyramidImage, uint32_t ObjectsCount, uint32_t FrameID, bool bLate, bool bSwapchainChanged)
{
    if (!bLate)
    {
    	BEGIN_GPU_PROFILER_BLOCK("CULLING_VOXEL_EARLY", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    }
    else
    {
    	BEGIN_GPU_PROFILER_BLOCK("CULLING_VOXEL_LATE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    }

	// NOTE(georgii): If there is no vkCmdDrawIndexedIndirectCount extensions, we should clear indirect draw buffer
	if (!Vulkan.vkCmdDrawIndexedIndirectCount)
	{
		VkBufferMemoryBarrier StartFillBufferBarrier = CreateBufferMemoryBarrier(VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, IndirectBuffer, IndirectBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &StartFillBufferBarrier, 0, 0);

		vkCmdFillBuffer(Vulkan.CommandBuffer, IndirectBuffer.Buffer, 0, IndirectBuffer.Size, 0);

		VkBufferMemoryBarrier FillBufferBarrier = CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, IndirectBuffer, IndirectBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, 1, &FillBufferBarrier, 0, 0);
	}

    VkBufferMemoryBarrier StartFillBufferBarrier = CreateBufferMemoryBarrier(VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, CountBuffer, sizeof(uint32_t));
    vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &StartFillBufferBarrier, 0, 0);

    vkCmdFillBuffer(Vulkan.CommandBuffer, CountBuffer.Buffer, 0, sizeof(uint32_t), 0);

    VkBufferMemoryBarrier FillBufferBarrier = CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, CountBuffer, sizeof(uint32_t));
    vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, 1, &FillBufferBarrier, 0, 0);

    if (!bLate && ((FrameID == 0) || bSwapchainChanged))
    {
		VkImageMemoryBarrier DepthPyramidBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DepthPyramidImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, 0, 0, 1, &DepthPyramidBarrier);
    }

    vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);

    vkCmdBindDescriptorSets(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout, 0, 1, &DescrSets[Vulkan.FrameInFlight], 0, 0);

    SVoxelCullingPushConstants PushConstants = { bLate, ObjectsCount, Vulkan.bMinMaxSampler ? 1u : 0u };
    vkCmdPushConstants(Vulkan.CommandBuffer, PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SVoxelCullingPushConstants), &PushConstants);

    vkCmdDispatch(Vulkan.CommandBuffer, (ObjectsCount + 31) / 32, 1, 1);

	// NOTE(georgii): If there is no vkCmdDrawIndexedIndirectCount extensions, we should clear indirect draw buffer
	if (!Vulkan.vkCmdDrawIndexedIndirectCount)
	{
		VkBufferMemoryBarrier CullBufferBarrier = CreateBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, IndirectBuffer, IndirectBuffer.Size);
    	vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, 0, 1, &CullBufferBarrier, 0, 0);	
	}

    VkBufferMemoryBarrier CullBufferBarrier = CreateBufferMemoryBarrier(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, CountBuffer, sizeof(uint32_t));
    vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, 0, 1, &CullBufferBarrier, 0, 0);

    if (!bLate)
    {
    	END_GPU_PROFILER_BLOCK("CULLING_VOXEL_EARLY", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    }
    else
    {
    	END_GPU_PROFILER_BLOCK("CULLING_VOXEL_LATE", Vulkan.CommandBuffer, Vulkan.FrameInFlight);
    }
}

void SVoxelCullingComputePass::UpdateAfterResize(const SVulkanContext& Vulkan, VkSampler DepthSampler, const SImage& DepthPyramidImage)
{
    for (uint32_t I = 0; I < ArrayCount(DescrSets); I++)
    {
        UpdateDescriptorSetImage(Vulkan.Device, DescrSets[I], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DepthSampler, DepthPyramidImage.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}