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

void CreateSphereMesh(SGeometry& Geometry)
{
	const uint32_t StackCount = 64;
	const uint32_t SectorCount = 64;

	const uint32_t VertexCount = (StackCount - 1) * SectorCount + 2;
	const uint32_t IndexCount = 2 * 3 * SectorCount + (StackCount - 2) * SectorCount * 6;
	SVertex Vertices[VertexCount];
	uint32_t Indices[IndexCount];

	uint32_t VertexIndex = 0;
	Vertices[VertexIndex].Pos = 0.5f * Vec3(0.0f, 1.0f, 0.0f);
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
			Vertices[VertexIndex].Pos = 0.5f * Vec3(X, Y, Z);
			Vertices[VertexIndex++].Normal = Normalize(Vertices[VertexIndex].Pos);
		}
	}
	Vertices[VertexIndex].Pos = 0.5f * Vec3(0.0f, -1.0f, 0.0f);
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

	AddVerticesToGeometry(Geometry, ArrayCount(Vertices), Vertices, ArrayCount(Indices), Indices);
}

SFont LoadFont(VkDevice Device, VkDescriptorPool DescriptorPool, VkCommandPool CommandPool, VkCommandBuffer CommandBuffer, VkQueue Queue, const SBuffer& StagingBuffer, VmaAllocator MemoryAllocator, VkDescriptorSetLayout DescrSetLayout, const char* FontName)
{
	SFont Font = {};

	char FontBmpPath[256] = {};
	ConcStrings(FontBmpPath, ArrayCount(FontBmpPath), "Fonts\\", FontName);
	ConcStrings(FontBmpPath, ArrayCount(FontBmpPath), FontBmpPath, ".bmp");

	char FontInfoPath[256] = {};
	ConcStrings(FontInfoPath, ArrayCount(FontInfoPath), "Fonts\\", FontName);
	ConcStrings(FontInfoPath, ArrayCount(FontInfoPath), FontInfoPath, ".fnt");

	Font.BitmapFont = LoadTexture(Device, CommandPool, CommandBuffer, Queue, StagingBuffer, MemoryAllocator, FontBmpPath, true);
	Font.BitmapSampler = CreateSampler(Device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, (float) GetMipsCount(Font.BitmapFont.Width, Font.BitmapFont.Height), VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	Font.BitmapFontDescrSet = CreateDescriptorSet(Device, DescriptorPool, DescrSetLayout);
	UpdateDescriptorSetImage(Device, Font.BitmapFontDescrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Font.BitmapSampler, Font.BitmapFont.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	SReadEntireFileResult FontInfoFile = ReadEntireTextFile(FontInfoPath);
	if (FontInfoFile.Memory && FontInfoFile.Size > 0)
	{
		bool bKerningStarted = false;
		const char *CurrentLine = (const char*) FontInfoFile.Memory;
		while (*CurrentLine)
		{
			// Read first symbol
			char FirstSymbol = CurrentLine[0];
			
			// Test if are at kerning info
			bKerningStarted = bKerningStarted || (FirstSymbol == 'K');
			
			if (!bKerningStarted)
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
				int32_t XLength = 0;
				while (*Iter != ' ')
				{
					XLength++;
					Iter++;
				}
				Iter++;

				const char* YPtr = Iter;
				int32_t YLength = 0;
				while (*Iter != ' ')
				{
					YLength++;
					Iter++;
				}
				Iter++;

				const char* WidthPtr = Iter;
				int32_t WidthLength = 0;
				while (*Iter != ' ')
				{
					WidthLength++;
					Iter++;
				}
				Iter++;

				const char* HeightPtr = Iter;
				int32_t HeightLength = 0;
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
				int32_t AdvanceLength = 0;
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
				int32_t OffsetXLength = 0;
				while (*Iter != ' ')
				{
					OffsetXLength++;
					Iter++;
				}
				Iter++;

				const char* OffsetYPtr = Iter;
				int32_t OffsetYLength = 0;
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

				int32_t GlyphX = StrToInt32(XPtr, XLength);
				int32_t GlyphY = StrToInt32(YPtr, YLength);
				int32_t GlyphWidth = StrToInt32(WidthPtr, WidthLength);
				int32_t GlyphHeight = StrToInt32(HeightPtr, HeightLength);

				int32_t GlyphAdvance = StrToInt32(AdvancePtr, AdvanceLength);
				
				int32_t GlyphOffsetX = StrToInt32(OffsetXPtr, OffsetXLength);
				int32_t GlyphOffsetY = StrToInt32(OffsetYPtr, OffsetYLength);

				Font.Glyphs[Character].Width = GlyphWidth;
				Font.Glyphs[Character].Height = GlyphHeight;
				Font.Glyphs[Character].Advance = GlyphAdvance;
				Font.Glyphs[Character].OffsetX = GlyphOffsetX;
				Font.Glyphs[Character].OffsetY = GlyphOffsetY;
				Font.Glyphs[Character].UVs.x = GlyphX / float(Font.BitmapFont.Width);
				Font.Glyphs[Character].UVs.y = GlyphY / float(Font.BitmapFont.Height);
				Font.Glyphs[Character].UVs.z = (GlyphX + GlyphWidth) / float(Font.BitmapFont.Width);
				Font.Glyphs[Character].UVs.w = (GlyphY + GlyphHeight) / float(Font.BitmapFont.Height);

				if (GlyphHeight > Font.MaxHeight)
				{
					Font.MaxHeight = GlyphHeight;
				}
			}
			else
			{
				const char *Iter = CurrentLine;

				// Skip KERNING line
				if (FirstSymbol == 'K')
				{
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
				}
				else
				{
					// Read character
					char Character = CurrentLine[1];
					Assert(Character < ArrayCount(Font.Glyphs));

					// Start reading kerning
					Iter = CurrentLine + 3;

					while ((*Iter != '\r') && (*Iter != '\n') && (*Iter != 0))
					{
						while (*Iter != '\"')
						{
							Iter++;
						}
						Iter++;

						char KerningChar = *Iter;
						Assert(KerningChar < ArrayCount(Font.Glyphs));

						Iter++;
						Assert(*Iter == '\"');
						Iter++;
						Iter++;
						Iter++;

						const char* AdvancePtr = Iter;
						int32_t AdvanceLength = 0;
						while (*Iter != '\"')
						{
							AdvanceLength++;
							Iter++;
						}
						Iter++;

						int32_t Advance = StrToInt32(AdvancePtr, AdvanceLength);
						Font.Kerning[Character][KerningChar] = Advance;
					}

					if (*Iter == '\r')
					{
						CurrentLine = Iter + 2;
					}
					else if (*Iter == '\n')
					{
						CurrentLine = Iter + 1;
					}
					else
					{
						CurrentLine = Iter;
					}
				}
			}
		}

		for (uint32_t I = 0; I < ArrayCount(Font.Glyphs); I++)
		{
			Font.Glyphs[I].OffsetY = Font.MaxHeight - Font.Glyphs[I].OffsetY;
		}	

		FreeEntireFile(&FontInfoFile);
	}

	return Font;
}

vec2 GetTextSize(const SFont* Font, vec2 FontScale, const char* String)
{
	vec2 Size = Vec2(0.0f, FontScale.y * Font->MaxHeight);

	const char* PrevC = 0;
	for (const char *C = String; *C; C++)
	{
		Assert(*C < ArrayCount(Font->Glyphs));
		const SGlyph& Glyph = Font->Glyphs[*C];

		if (PrevC)
		{
			int32_t Kerning = Font->Kerning[*C][*PrevC];
			Size.x += FontScale.x * Kerning;
		}
        
		Size.x += FontScale.x * Glyph.Advance;
	}

	return Size;
}