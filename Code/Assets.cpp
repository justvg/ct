#include "Assets.h"

SImage LoadTexture(VkDevice Device, VkCommandPool CommandPool, VkCommandBuffer CommandBuffer, VkQueue Queue, const SBuffer& StagingBuffer, VmaAllocator MemoryAllocator, const char* Path, bool bGenerateMips = false)
{
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t ChannelCount = 0;
	void *PtrToFree = 0;
	
	uint8_t* Data = LoadBMP(Path, &Width, &Height, &ChannelCount, &PtrToFree);
	Assert(Data);

	uint32_t MipCount = bGenerateMips ? GetMipsCount(Width, Height) : 1;
	VkImageUsageFlags ImageUsageFlags = bGenerateMips ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	SImage Image = CreateImage(Device, MemoryAllocator, VK_FORMAT_R8G8B8A8_UNORM, Width, Height, 0, MipCount, ImageUsageFlags, VK_IMAGE_ASPECT_COLOR_BIT);
	UploadImageFromBuffer(Device, CommandPool, CommandBuffer, Queue, Image, Width, Height, StagingBuffer, Data, uint64_t(Width) * uint64_t(Height) * uint64_t(ChannelCount));

	free(PtrToFree);

	if (MipCount > 1)
	{
		VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkCheck(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

		VkImageMemoryBarrier MipGenBarriers[] = 
		{
			CreateImageMemoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Image.Image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1),
			CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Image.Image, VK_IMAGE_ASPECT_COLOR_BIT, 1, MipCount - 1)
		};
		vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ArrayCount(MipGenBarriers), MipGenBarriers);

		for (uint32_t I = 0; I < MipCount - 1; I++)
		{
			VkImageBlit BlitRegion = {};
			BlitRegion.srcOffsets[0] = { 0, 0, 0 };
			BlitRegion.srcOffsets[1] = { (int32_t) Max(Width >> I, 1), (int32_t) Max(Height >> I, 1), 1 };
			BlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			BlitRegion.srcSubresource.mipLevel = I;
			BlitRegion.srcSubresource.layerCount = 1;
			BlitRegion.dstOffsets[0] = { 0, 0, 0 };
			BlitRegion.dstOffsets[1] = { (int32_t) Max(Width >> (I + 1), 1), (int32_t) Max(Height >> (I + 1), 1), 1 };
			BlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			BlitRegion.dstSubresource.mipLevel = I + 1;
			BlitRegion.dstSubresource.layerCount = 1;

			vkCmdBlitImage(CommandBuffer, Image.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BlitRegion, VK_FILTER_LINEAR);

			VkImageMemoryBarrier TransferBarrier = CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Image.Image, VK_IMAGE_ASPECT_COLOR_BIT, I + 1, 1);
			vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &TransferBarrier);
		}

		VkImageMemoryBarrier MipGenEndBarrier = CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Image.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &MipGenEndBarrier);

		VkCheck(vkEndCommandBuffer(CommandBuffer));

		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;
		VkCheck(vkQueueSubmit(Queue, 1, &SubmitInfo, 0));

		VkCheck(vkDeviceWaitIdle(Device));
	}

	return Image;
}

void AddVerticesToGeometry(SGeometry& Geometry, uint32_t VertexCount, const SVertex* Vertices, uint32_t IndexCount, const uint32_t* Indices)
{
	Rect AABB = { Vec3(FLT_MAX), Vec3(-FLT_MAX) };
	for (uint32_t I = 0; I < VertexCount; I++)
	{
		if (Vertices[I].Pos.x < AABB.Min.x) AABB.Min.x = Vertices[I].Pos.x;
		if (Vertices[I].Pos.y < AABB.Min.y) AABB.Min.y = Vertices[I].Pos.y;
		if (Vertices[I].Pos.z < AABB.Min.z) AABB.Min.z = Vertices[I].Pos.z;
		if (Vertices[I].Pos.x > AABB.Max.x) AABB.Max.x = Vertices[I].Pos.x;
		if (Vertices[I].Pos.y > AABB.Max.y) AABB.Max.y = Vertices[I].Pos.y;
		if (Vertices[I].Pos.z > AABB.Max.z) AABB.Max.z = Vertices[I].Pos.z;
	}

	uint32_t PrevVertexCount = uint32_t(Geometry.VertexCount);
	uint32_t PrevIndexCount = uint32_t(Geometry.IndexCount);

	Assert(ArrayCount(Geometry.Vertices) >= Geometry.VertexCount + VertexCount);
	Assert(ArrayCount(Geometry.Indices) >= Geometry.IndexCount + IndexCount);
	for (uint32_t I = 0; I < VertexCount; I++)
	{
		Geometry.Vertices[Geometry.VertexCount++] = Vertices[I];
	}
	for (uint32_t I = 0; I < IndexCount; I++)
	{
		Geometry.Indices[Geometry.IndexCount++] = Indices[I];
	}

	SMesh Mesh = {};
	Mesh.Dim = GetDim(AABB);
	Mesh.VertexOffset = PrevVertexCount;
	Mesh.IndexOffset = PrevIndexCount;
	Mesh.IndexCount = IndexCount;

	Assert(Geometry.MeshCount < ArrayCount(Geometry.Meshes));
	Geometry.Meshes[Geometry.MeshCount++] = Mesh;
}

void AddGLTFMesh(SGeometry& Geometry, const char* Path)
{
    SVertex Vertices[2048];
    uint32_t Indices[4096];

	uint32_t VertexCount = 0;
	uint32_t IndexCount = 0;
    if (LoadGLTF(ArrayCount(Vertices), Vertices, ArrayCount(Indices), Indices, Path, VertexCount, IndexCount))
	{
		AddVerticesToGeometry(Geometry, VertexCount, Vertices, IndexCount, Indices);
	}
}

void CreateVoxelMesh(SGeometry& Geometry)
{
	SVertex Vertices[] = 
	{
		{ Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(1.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f) },
		{ Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f) },

		{ Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(1.0f, 0.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(1.0f, 1.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.0f, 1.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f) },

		{ Vec3(0.0f, 1.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(0.0f, 1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(0.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f) },
		{ Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f) },

		{ Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(1.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(1.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f) },
		{ Vec3(1.0f, 0.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f) },

		{ Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(1.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f) },
		{ Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f) },

		{ Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(1.0f, 1.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(1.0f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f) },
		{ Vec3(0.0f, 1.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f) },
	};

	uint32_t Indices[] = 
	{
		0, 1, 2, 1, 0, 3, 4, 5, 6, 6, 7, 4, 8, 9, 10, 10, 11, 8, 12, 13, 14, 13, 12, 15, 16, 17, 18, 18, 19, 16, 20, 21, 22, 21, 20, 23
	};

	AddVerticesToGeometry(Geometry, ArrayCount(Vertices), Vertices, ArrayCount(Indices), Indices);
}

void CreateQuadMesh(SGeometry& Geometry)
{
	SVertex Vertices[] = 
	{
		{ Vec3(-0.5f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(-0.5f, -0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.5f, 0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f) },
		{ Vec3(0.5f, -0.5f, 0.0f), Vec3(0.0f, 0.0f, 1.0f) }
	};

	uint32_t Indices[] = 
	{
		0, 1, 2, 2, 1, 3
	};

	AddVerticesToGeometry(Geometry, ArrayCount(Vertices), Vertices, ArrayCount(Indices), Indices);
}

SFont LoadFont(VkDevice Device, VkDescriptorPool DescriptorPool, VkCommandPool CommandPool, VkCommandBuffer CommandBuffer, VkQueue Queue, const SBuffer& StagingBuffer, VmaAllocator MemoryAllocator, VkDescriptorSetLayout DescrSetLayout)
{
	SFont Font = {};

	Font.BitmapFont = LoadTexture(Device, CommandPool, CommandBuffer, Queue, StagingBuffer, MemoryAllocator, "Fonts\\normaleste.bmp", true);
	Font.BitmapSampler = CreateSampler(Device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, (float) GetMipsCount(Font.BitmapFont.Width, Font.BitmapFont.Height));
	Font.BitmapFontDescrSet = CreateDescriptorSet(Device, DescriptorPool, DescrSetLayout);
	UpdateDescriptorSetImage(Device, Font.BitmapFontDescrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Font.BitmapSampler, Font.BitmapFont.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	ReadEntireFileResult FontInfoFile = ReadEntireTextFile("Fonts\\normaleste.fntinfo");
	Assert(FontInfoFile.Memory);

	const char *CurrentLine = (const char*) FontInfoFile.Memory;
	while (*CurrentLine)
	{
		// Read character
		char Character = CurrentLine[1];
		Assert(Character < ArrayCount(Font.Glyphs));

		// Read character rect
		const char *Iter = CurrentLine + 3;
		while (*Iter != '\"')
		{
			Iter++;
		}
		Iter++;

		const char* XPtr = Iter;
		uint32_t XLength = 0;
		while (*Iter != ' ')
		{
			XLength++;
			Iter++;
		}
		Iter++;

		const char* YPtr = Iter;
		uint32_t YLength = 0;
		while (*Iter != ' ')
		{
			YLength++;
			Iter++;
		}
		Iter++;

		const char* WidthPtr = Iter;
		uint32_t WidthLength = 0;
		while (*Iter != ' ')
		{
			WidthLength++;
			Iter++;
		}
		Iter++;

		const char* HeightPtr = Iter;
		uint32_t HeightLength = 0;
		while (*Iter != '\"')
		{
			HeightLength++;
			Iter++;
		}
		Iter++;

		// Read character advance
		while (*Iter != '\"')
		{
			Iter++;
		}
		Iter++;

		const char* AdvancePtr = Iter;
		uint32_t AdvanceLength = 0;
		while (*Iter != '\"')
		{
			AdvanceLength++;
			Iter++;
		}
		Iter++;

		// Read character offset
		while (*Iter != '\"')
		{
			Iter++;
		}
		Iter++;

		const char* OffsetXPtr = Iter;
		uint32_t OffsetXLength = 0;
		while (*Iter != ' ')
		{
			OffsetXLength++;
			Iter++;
		}
		Iter++;

		const char* OffsetYPtr = Iter;
		uint32_t OffsetYLength = 0;
		while (*Iter != '\"')
		{
			OffsetYLength++;
			Iter++;
		}

		// End line
		while ((*Iter != '\n') && (*Iter != 0))
		{
			Iter++;
		}

		if (*Iter == '\n')
		{
			CurrentLine = Iter + 1;
		}
		else
		{
			CurrentLine = Iter;
		}

		uint32_t GlyphX = StrToInt32(XPtr, XLength);
		uint32_t GlyphY = StrToInt32(YPtr, YLength);
		uint32_t GlyphWidth = StrToInt32(WidthPtr, WidthLength);
		uint32_t GlyphHeight = StrToInt32(HeightPtr, HeightLength);

		uint32_t GlyphAdvance = StrToInt32(AdvancePtr, AdvanceLength);
		
		uint32_t GlyphOffsetX = StrToInt32(OffsetXPtr, OffsetXLength);
		uint32_t GlyphOffsetY = StrToInt32(OffsetYPtr, OffsetYLength);

		Font.Glyphs[Character].Width = GlyphWidth;
		Font.Glyphs[Character].Height = GlyphHeight;
		Font.Glyphs[Character].Advance = GlyphAdvance;
		Font.Glyphs[Character].OffsetX = GlyphOffsetX;
		Font.Glyphs[Character].OffsetY = 81 - GlyphOffsetY;
		Font.Glyphs[Character].UVs.x = GlyphX / float(Font.BitmapFont.Width);
		Font.Glyphs[Character].UVs.y = GlyphY / float(Font.BitmapFont.Height);
		Font.Glyphs[Character].UVs.z = (GlyphX + GlyphWidth) / float(Font.BitmapFont.Width);
		Font.Glyphs[Character].UVs.w = (GlyphY + GlyphHeight) / float(Font.BitmapFont.Height);
	}

	return Font;
}

vec2 GetTextSize(const SFont& Font, float FontScale, const char* String)
{
	vec2 Size = Vec2(0.0f, FontScale * 81);

	for (const char *C = String; *C; C++)
	{
		Assert(*C < ArrayCount(Font.Glyphs));
		const SGlyph& Glyph = Font.Glyphs[*C];
        
		Size.x += FontScale * Glyph.Advance * 0.9f;
	}

	return Size;
}