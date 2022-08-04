#pragma once

struct SVertex
{
	vec3 Pos;
	vec3 Normal;
};

struct SMesh
{
	vec3 Dim;

	uint32_t IndexCount;
	uint32_t IndexOffset;
	uint32_t VertexOffset;
};

struct SGeometry
{
	uint32_t VertexCount;
	SVertex Vertices[8192];

	uint32_t IndexCount;
	uint32_t Indices[32768];

	uint32_t MeshCount;
	SMesh Meshes[512];
};

struct SGlyph
{
	uint32_t Width;
	uint32_t Height;

	int32_t Advance;

	int32_t OffsetX;
	int32_t OffsetY;

	vec4 UVs;
};

struct SFont
{
	SImage BitmapFont;
	VkSampler BitmapSampler;
	VkDescriptorSet BitmapFontDescrSet;

	SGlyph Glyphs[128];
	int32_t Kerning[ArrayCount(Glyphs)][ArrayCount(Glyphs)];
	int32_t MaxHeight;
};