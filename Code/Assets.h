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
	uint32_t Indices[16384];

	uint32_t MeshCount;
	SMesh Meshes[512];
};

struct SGlyph
{
	uint32_t Width;
	uint32_t Height;

	uint32_t Advance;

	uint32_t OffsetX;
	uint32_t OffsetY;

	vec4 UVs;
};

struct SFont
{
	SImage BitmapFont;
	VkSampler BitmapSampler;
	VkDescriptorSet BitmapFontDescrSet;

	SGlyph Glyphs[128];
};