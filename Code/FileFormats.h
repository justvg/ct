#pragma once

//
// NOTE(georgii): BMP
//

#pragma pack(push, 1)
struct FileHeaderBMP
{
	uint16_t FileType;
	uint32_t FileSize;
	uint32_t Reserved;
	uint32_t BitmapOffset;
	uint32_t InfoHeaderSize;
	int32_t Width;
	int32_t Height;
	uint16_t Planes;
	uint16_t BitsPerPixel;
	uint32_t Compression;
	uint32_t SizeOfBitmap;
	uint32_t HorizontalResolution;
	uint32_t VerticalResolution;
	uint32_t ColorsUsed;
	uint32_t ColorsImportant;

	uint32_t RedMask;
	uint32_t GreenMask;
	uint32_t BlueMask;
};
#pragma pack(pop)

uint8_t* LoadBMP(const char* Path, uint32_t* Width, uint32_t* Height, uint32_t* ChannelCount, void** PtrToFree)
{
	uint8_t* Result = 0;

	SReadEntireFileResult FileData = ReadEntireFile(Path);

	if (FileData.Size != 0)
	{
		FileHeaderBMP* Header = (FileHeaderBMP *)FileData.Memory;
		uint32_t* Pixels = (uint32_t*)((uint8_t*)Header + Header->BitmapOffset);
		Result = (uint8_t*)Pixels;
		*Width = Header->Width;
		*Height = Header->Height;
		*ChannelCount = Header->BitsPerPixel / 8;
		*PtrToFree = FileData.Memory;

		Assert(Header->Height >= 0);
		Assert(Header->Compression == 3);

		uint32_t RedMask = Header->RedMask;
		uint32_t GreenMask = Header->GreenMask;
		uint32_t BlueMask = Header->BlueMask;
		uint32_t AlphaMask = ~(RedMask | GreenMask | BlueMask);

		uint32_t RedShiftRight = FindLeastSignificantSetBitIndex(RedMask);
		uint32_t GreenShiftRight = FindLeastSignificantSetBitIndex(GreenMask);
		uint32_t BlueShiftRight = FindLeastSignificantSetBitIndex(BlueMask);
		uint32_t AlphaShiftRight = FindLeastSignificantSetBitIndex(AlphaMask);

		uint32_t* SourceDest = Pixels;
		for(int32_t Y = 0; Y < Header->Height; Y++)
		{
			for(int32_t X = 0; X < Header->Width; X++)
			{
				uint32_t Color = *SourceDest;

				uint8_t Red = uint8_t(((Color & RedMask) >> RedShiftRight));
				uint8_t Green = uint8_t(((Color & GreenMask) >> GreenShiftRight));
				uint8_t Blue = uint8_t(((Color & BlueMask) >> BlueShiftRight));
				uint8_t Alpha = uint8_t(((Color & AlphaMask) >> AlphaShiftRight));

				*SourceDest++ = ((Alpha << 24) | (Blue << 16) | (Green << 8) | (Red << 0));
			}
		}
	}

	return Result;
}

//
// NOTE(georgii): WAV
//

#pragma pack(push, 1)
struct SWAVInfo
{
  uint16_t FormatTag;
  uint16_t ChannelCount;
  uint32_t SamplesPerSec;
  uint32_t AvgBytesPerSec;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
  uint16_t Size;
};
struct SWAVHeader
{
	uint32_t RIFFID;
	uint32_t Size;
	uint32_t WAVEID;
};
struct SWAVChunkHeader
{
	uint32_t ID;
	uint32_t Size;
};
#pragma pack(pop)

#define RIFF_FOURCC 'FFIR'
#define WAVE_FOURCC 'EVAW'
#define FMT_FOURCC ' tmf'
#define LIST_FOURCC 'TSIL'
#define DATA_FOURCC 'atad'

struct SLoadedWAV
{
	uint32_t SampleCount;
	int16_t* Samples;
	void *PtrToFree;
};

SLoadedWAV LoadWAV(const char* Path)
{
	SLoadedWAV Result = {};

	SReadEntireFileResult File = ReadEntireFile(Path);
	if (File.Memory)
	{
		SWAVHeader* Header = (SWAVHeader*) File.Memory;
		Assert(Header->RIFFID == RIFF_FOURCC);
		Assert(Header->WAVEID == WAVE_FOURCC);

		SWAVChunkHeader* HeaderFMT = (SWAVChunkHeader*) ((uint8_t*) Header + sizeof(SWAVHeader));
		Assert(HeaderFMT->ID == FMT_FOURCC);

		SWAVChunkHeader* HeaderData = (SWAVChunkHeader*) ((uint8_t*) HeaderFMT + sizeof(SWAVChunkHeader) + HeaderFMT->Size);
		if (HeaderData->ID == LIST_FOURCC)
		{
			HeaderData = (SWAVChunkHeader*) ((uint8_t*) HeaderData + sizeof(SWAVChunkHeader) + HeaderData->Size);
		}
		Assert(HeaderData->ID == DATA_FOURCC);

		SWAVInfo* Info = (SWAVInfo*) ((uint8_t*) HeaderFMT + sizeof(SWAVChunkHeader));
		Assert(Info->SamplesPerSec == 44100);
		Assert(Info->BitsPerSample == 16);
		Assert(Info->ChannelCount == 2);

		Result.SampleCount = HeaderData->Size / (Info->ChannelCount * (Info->BitsPerSample / 8));
		Result.Samples = (int16_t*) ((uint8_t*)HeaderData + sizeof(SWAVChunkHeader));
		Result.PtrToFree = File.Memory;
	}

    return Result;
}

//
// NOTE(georgii): JSON and glTF
//

enum EPrimitiveTypeJSON
{
	PrimitiveTypeJSON_None,

	PrimitiveTypeJSON_String,
	PrimitiveTypeJSON_Int,
	PrimitiveTypeJSON_Float,
};

struct SPrimitiveJSON
{
	EPrimitiveTypeJSON Type;

	uint32_t StringLength;
	union
	{
		int32_t I32;
		float Float;
		const char* String;
	};
};

struct STokenJSON;
struct SKeyValueJSON
{
	uint32_t NameLength;
	const char* Name;

	STokenJSON* Value;
};

enum ETokenTypeJSON
{
	TokenTypeJSON_None,

	TokenTypeJSON_Object,
	TokenTypeJSON_Array,
	TokenTypeJSON_KeyValue,
	TokenTypeJSON_Primitive
};

struct STokenJSON
{
	ETokenTypeJSON Type;
	
	union
	{
		DynamicArray<STokenJSON> Tokens;
		SKeyValueJSON KeyValue;
		SPrimitiveJSON Primitive;
	};

	STokenJSON() 
	{
		memset(this, 0, sizeof(STokenJSON));
	}

	STokenJSON(const STokenJSON& Other)
	{
		Type = Other.Type;

		Tokens.Elements = 0;	
		switch (Type)
		{
			case TokenTypeJSON_Object:
			case TokenTypeJSON_Array:
			{
				Tokens = Other.Tokens;
			} break;

			case TokenTypeJSON_KeyValue:
			{
				KeyValue = Other.KeyValue;
			} break;

			case TokenTypeJSON_Primitive:
			{
				Primitive = Other.Primitive;
			} break;
		}
	}

	STokenJSON& operator=(const STokenJSON& Other)
	{
		Type = Other.Type;
		
		switch (Type)
		{
			case TokenTypeJSON_Object:
			case TokenTypeJSON_Array:
			{
				Tokens = Other.Tokens;
			} break;

			case TokenTypeJSON_KeyValue:
			{
				KeyValue = Other.KeyValue;
			} break;

			case TokenTypeJSON_Primitive:
			{
				Primitive = Other.Primitive;
			} break;
		}

		return *this;
	}
};

STokenJSON* ParseJSON(const char* Ptr)
{
	uint32_t TokenStackTop = 0;
	STokenJSON* TokenStack[1024] = {};

	// NOTE(georgii): These are used to temporarily store a number's string representation and length
	uint32_t NumberStrLengthTemp = 0;
	const char* NumberStrTemp = 0; 

	bool bKeyValueNameStarted = false;
	bool bExpectValue = false;
	bool bPrimitiveNumberStarted = false;
	while (*Ptr)
	{
		STokenJSON* CurrentToken = (TokenStackTop > 0) ? TokenStack[TokenStackTop - 1] : 0;

		if (CurrentToken)
		{
			switch (CurrentToken->Type)
			{
				case TokenTypeJSON_Object:
				{
					if (*Ptr == '\"')
					{
						STokenJSON NewKeyValueToken = {};
						NewKeyValueToken.Type = TokenTypeJSON_KeyValue;
						NewKeyValueToken.KeyValue.Value = (STokenJSON*) malloc(sizeof(STokenJSON));
						memset(NewKeyValueToken.KeyValue.Value, 0, sizeof(STokenJSON));
						CurrentToken->Tokens.Push(NewKeyValueToken);

						TokenStack[TokenStackTop++] = &CurrentToken->Tokens[CurrentToken->Tokens.Size - 1];
						bKeyValueNameStarted = true;
					}

					if (*Ptr == '}')
					{
						TokenStackTop--;
					}

					Ptr++;
				} break;

				case TokenTypeJSON_KeyValue:
				{
					if ((*Ptr == '\"') && bKeyValueNameStarted)
					{
						bKeyValueNameStarted = false;
					}

					if (bKeyValueNameStarted)
					{
						if (CurrentToken->KeyValue.NameLength == 0)
							CurrentToken->KeyValue.Name = Ptr;

						CurrentToken->KeyValue.NameLength++;	
					}

					if (bExpectValue)
					{
						if (*Ptr == '\"')
						{
							CurrentToken->KeyValue.Value->Type = TokenTypeJSON_Primitive;
							CurrentToken->KeyValue.Value->Primitive.Type = PrimitiveTypeJSON_String;
							TokenStack[TokenStackTop++] = CurrentToken->KeyValue.Value;

							bExpectValue = false;
						}
						else if (IsDigit(*Ptr) || (*Ptr == '-') || (*Ptr == '+'))
						{
							CurrentToken->KeyValue.Value->Type = TokenTypeJSON_Primitive;
							TokenStack[TokenStackTop++] = CurrentToken->KeyValue.Value;
							bPrimitiveNumberStarted = true;

							bExpectValue = false;
						}
						else if (*Ptr == '[')
						{
							CurrentToken->KeyValue.Value->Type = TokenTypeJSON_Array;
							TokenStack[TokenStackTop++] = CurrentToken->KeyValue.Value;

							bExpectValue = false;
						} 
						else if (*Ptr == '{')
						{
							CurrentToken->KeyValue.Value->Type = TokenTypeJSON_Object;
							TokenStack[TokenStackTop++] = CurrentToken->KeyValue.Value;

							bExpectValue = false;
						}
					}

					if (*Ptr == ':')
						bExpectValue = true;

					if (*Ptr == '}')
					{
						TokenStackTop--;
					}
					else
					{
						if (*Ptr == ',')
							TokenStackTop--;

						if (!bPrimitiveNumberStarted)
							Ptr++;
					}
				} break;

				case TokenTypeJSON_Array:
				{
					if (*Ptr == ']')
						TokenStackTop--;

					if (*Ptr == '\"')
					{
						STokenJSON NewPrimitiveStringToken = {};
						NewPrimitiveStringToken.Type = TokenTypeJSON_Primitive;
						CurrentToken->Tokens.Push(NewPrimitiveStringToken);

						TokenStack[TokenStackTop++] = &CurrentToken->Tokens[CurrentToken->Tokens.Size - 1];
					}
					else if (IsDigit(*Ptr) || (*Ptr == '-') || (*Ptr == '+'))
					{
						STokenJSON NewPrimitiveNumberToken = {};
						NewPrimitiveNumberToken.Type = TokenTypeJSON_Primitive;
						CurrentToken->Tokens.Push(NewPrimitiveNumberToken);

						TokenStack[TokenStackTop++] = &CurrentToken->Tokens[CurrentToken->Tokens.Size - 1];
						bPrimitiveNumberStarted = true;
					}
					else if (*Ptr == '[')
					{
						STokenJSON NewArrayToken = {};
						NewArrayToken.Type = TokenTypeJSON_Array;
						CurrentToken->Tokens.Push(NewArrayToken);

						TokenStack[TokenStackTop++] = &CurrentToken->Tokens[CurrentToken->Tokens.Size - 1];
					} 
					else if (*Ptr == '{')
					{
						STokenJSON NewObjectToken = {};
						NewObjectToken.Type = TokenTypeJSON_Object;
						CurrentToken->Tokens.Push(NewObjectToken);

						TokenStack[TokenStackTop++] = &CurrentToken->Tokens[CurrentToken->Tokens.Size - 1];
					}
					
					if (!bPrimitiveNumberStarted && (*Ptr != ']'))
						Ptr++;
				} break;

				case TokenTypeJSON_Primitive:
				{
					if (CurrentToken->Primitive.Type == PrimitiveTypeJSON_String)
					{
						if (*Ptr == '\"')
						{
							Assert(CurrentToken->Primitive.StringLength > 0);
							TokenStackTop--;
						}
						else
						{
							if (CurrentToken->Primitive.StringLength == 0)
								CurrentToken->Primitive.String = Ptr;
							CurrentToken->Primitive.StringLength++;
						}

						Ptr++;
					}
					else if (bPrimitiveNumberStarted)
					{
						if (IsDigit(*Ptr) || (*Ptr == '.') || (*Ptr == '-') || (*Ptr == '+'))
						{
							if (NumberStrLengthTemp == 0)
								NumberStrTemp = Ptr;

							if (*Ptr == '.')
								CurrentToken->Primitive.Type = PrimitiveTypeJSON_Float;

							NumberStrLengthTemp++;

							Ptr++;
						}
						else
						{
							Assert((NumberStrLengthTemp > 0) && NumberStrTemp);

							if (CurrentToken->Primitive.Type == PrimitiveTypeJSON_Float)
							{
								CurrentToken->Primitive.Float = StrToFloat(NumberStrTemp, NumberStrLengthTemp);
							}
							else
							{
								CurrentToken->Primitive.Type = PrimitiveTypeJSON_Int;
								CurrentToken->Primitive.I32 = StrToInt32(NumberStrTemp, NumberStrLengthTemp);
							}

							TokenStackTop--;

							NumberStrTemp = 0;
							NumberStrLengthTemp = 0;
							bPrimitiveNumberStarted = false;
						}
					}
				} break;
			}
		}
		else
		{
			if (*Ptr == '{')
			{
				if (CurrentToken == 0)
					CurrentToken = (STokenJSON*) malloc(sizeof(STokenJSON));
				memset(CurrentToken, 0, sizeof(STokenJSON));
				
				CurrentToken->Type = TokenTypeJSON_Object;
				TokenStack[TokenStackTop++] = CurrentToken;
			}

			Ptr++;
		}
	}

	return TokenStack[0];
}

void FreeParsedJSON(STokenJSON* Token)
{
	switch (Token->Type)
	{
		case TokenTypeJSON_Object:
		case TokenTypeJSON_Array:
		{
			for (uint32_t I = 0; I < Token->Tokens.Size; I++)
			{
				FreeParsedJSON(&Token->Tokens[I]);
			}
			Token->Tokens.Free();
		} break;

		case TokenTypeJSON_KeyValue:
		{
			FreeParsedJSON(Token->KeyValue.Value);
			free(Token->KeyValue.Value);
			Token->KeyValue.Value = 0;
		} break;

		case TokenTypeJSON_Primitive:
		{

		} break;
	}		
}

struct SAttributeInfoGLTF
{
	uint32_t ByteOffset;
	uint32_t ByteStride;
	uint32_t CompCount;
	uint32_t Count;
	uint32_t CompSize;
};

uint32_t GetComponentCountFromStringGLTF(STokenJSON* CompCountToken)
{
	uint32_t CompCount = 0;

	uint32_t StringLength = CompCountToken->KeyValue.Value->Primitive.StringLength;
	const char* String = CompCountToken->KeyValue.Value->Primitive.String;
	if (CompareStrings(String, StringLength, "SCALAR"))
		CompCount = 1;
	else if (CompareStrings(String, StringLength, "VEC2"))
		CompCount = 2;
	else if (CompareStrings(String, StringLength, "VEC3"))
		CompCount = 3;
	else if (CompareStrings(String, StringLength, "VEC4"))
		CompCount = 4;
	else if (CompareStrings(String, StringLength, "MAT2"))
		CompCount = 4;
	else if (CompareStrings(String, StringLength, "MAT3"))
		CompCount = 9;
	else if (CompareStrings(String, StringLength, "MAT4"))
		CompCount = 16;

	Assert(CompCount > 0);

	return CompCount;
}

uint32_t GetComponentSizeFromTypeGLTF(STokenJSON* CompType)
{
	uint32_t CompSize = 0;

	switch (CompType->KeyValue.Value->Primitive.I32)
	{
		case 5120:
		case 5121:
		{
			CompSize = 1;
		} break;

		case 5122:
		case 5123:
		{
			CompSize = 2;
		} break;

		case 5125:
		case 5126:
		{
			CompSize = 4;
		} break;
	}

	Assert(CompSize > 0);

	return CompSize;
}

SAttributeInfoGLTF GetAttributeInfoGLTF(STokenJSON* Accessor, STokenJSON* BufferViewsArray)
{
	STokenJSON* BufferViewIndexToken = &Accessor->Tokens[0];
	Assert(BufferViewIndexToken && (BufferViewIndexToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(BufferViewIndexToken->KeyValue.Name, BufferViewIndexToken->KeyValue.NameLength, "bufferView"));

	STokenJSON* ByteOffsetToken = &Accessor->Tokens[1];
	Assert(ByteOffsetToken && (ByteOffsetToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(ByteOffsetToken->KeyValue.Name, ByteOffsetToken->KeyValue.NameLength, "byteOffset"));

	STokenJSON* CompType = &Accessor->Tokens[2];
	Assert(CompType && (CompType->Type == TokenTypeJSON_KeyValue) && CompareStrings(CompType->KeyValue.Name, CompType->KeyValue.NameLength, "componentType"));

	STokenJSON* Count = &Accessor->Tokens[3];
	Assert(Count && (Count->Type == TokenTypeJSON_KeyValue) && CompareStrings(Count->KeyValue.Name, Count->KeyValue.NameLength, "count"));

	STokenJSON* CompCount = &Accessor->Tokens[4];
	Assert(CompCount && (CompCount->Type == TokenTypeJSON_KeyValue) && CompareStrings(CompCount->KeyValue.Name, CompCount->KeyValue.NameLength, "type"));

	int32_t BufferViewIndex = BufferViewIndexToken->KeyValue.Value->Primitive.I32;
	STokenJSON* BufferView = &BufferViewsArray->Tokens[BufferViewIndex];

	STokenJSON* ByteStride = &BufferView->Tokens[3];
	Assert(ByteStride && (ByteStride->Type == TokenTypeJSON_KeyValue) && CompareStrings(ByteStride->KeyValue.Name, ByteStride->KeyValue.NameLength, "byteStride"));	

	STokenJSON* BufferViewByteOffsetToken = &BufferView->Tokens[2];
	Assert(BufferViewByteOffsetToken && (BufferViewByteOffsetToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(BufferViewByteOffsetToken->KeyValue.Name, BufferViewByteOffsetToken->KeyValue.NameLength, "byteOffset"));

	uint32_t ByteOffset = ByteOffsetToken->KeyValue.Value->Primitive.I32;
	uint32_t BufferViewByteOffset = BufferViewByteOffsetToken->KeyValue.Value->Primitive.I32;

	SAttributeInfoGLTF AttributeInfo = {};
	AttributeInfo.ByteOffset = ByteOffset + BufferViewByteOffset;
	AttributeInfo.ByteStride = ByteStride->KeyValue.Value->Primitive.I32;
	AttributeInfo.CompCount = GetComponentCountFromStringGLTF(CompCount);
	AttributeInfo.Count = Count->KeyValue.Value->Primitive.I32;
	AttributeInfo.CompSize = GetComponentSizeFromTypeGLTF(CompType);

	return AttributeInfo;
}

bool LoadGLTF(uint32_t MaxVertexCount, SVertex* Vertices, uint32_t MaxIndexCount, uint32_t* Indices, const char* Path, uint32_t& VertexCount, uint32_t& IndexCount)
{
    SReadEntireFileResult FileData = ReadEntireTextFile(Path);
	if (FileData.Memory)
	{
		char* JSONString = (char*) FileData.Memory;

		STokenJSON* ParsedJSON = ParseJSON(JSONString);
		Assert(ParsedJSON->Type == TokenTypeJSON_Object);

		STokenJSON* IsIndexedToken = &ParsedJSON->Tokens[0];
		Assert(IsIndexedToken && (IsIndexedToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(IsIndexedToken->KeyValue.Name, IsIndexedToken->KeyValue.NameLength, "indexed"))
		bool bIsIndexed = (IsIndexedToken->KeyValue.Value->Primitive.I32 > 0);

		STokenJSON* MeshesToken = &ParsedJSON->Tokens[1];
		Assert(MeshesToken && (MeshesToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(MeshesToken->KeyValue.Name, MeshesToken->KeyValue.NameLength, "meshes"));

		STokenJSON* MeshesArray = MeshesToken->KeyValue.Value;
		Assert(MeshesArray && (MeshesArray->Type == TokenTypeJSON_Array));

		STokenJSON* Primitives = &MeshesArray->Tokens[0].Tokens[0];
		Assert(Primitives && (Primitives->Type == TokenTypeJSON_KeyValue) && CompareStrings(Primitives->KeyValue.Name, Primitives->KeyValue.NameLength, "primitives"));

		STokenJSON* PrimitivesObject = &Primitives->KeyValue.Value->Tokens[0];
		Assert(PrimitivesObject && (PrimitivesObject->Type == TokenTypeJSON_Object));

		STokenJSON* Attributes = &PrimitivesObject->Tokens[0];
		Assert(Attributes && (Attributes->Type == TokenTypeJSON_KeyValue) && CompareStrings(Attributes->KeyValue.Name, Attributes->KeyValue.NameLength, "attributes"));

		STokenJSON* PositionAttribute = &Attributes->KeyValue.Value->Tokens[0];
		Assert(PositionAttribute && (PositionAttribute->Type == TokenTypeJSON_KeyValue) && CompareStrings(PositionAttribute->KeyValue.Name, PositionAttribute->KeyValue.NameLength, "POSITION"));

		STokenJSON* NormalAttribute = &Attributes->KeyValue.Value->Tokens[1];
		Assert(NormalAttribute && (NormalAttribute->Type == TokenTypeJSON_KeyValue) && CompareStrings(NormalAttribute->KeyValue.Name, NormalAttribute->KeyValue.NameLength, "NORMAL"));

		int32_t PosAccessIndex = PositionAttribute->KeyValue.Value->Primitive.I32;
		int32_t NormAccessIndex = NormalAttribute->KeyValue.Value->Primitive.I32;

		int32_t IndAccessIndex = -1;
		if (bIsIndexed)
		{
			STokenJSON* IndexAttribute = &PrimitivesObject->Tokens[1];
			Assert(IndexAttribute && (IndexAttribute->Type == TokenTypeJSON_KeyValue) && CompareStrings(IndexAttribute->KeyValue.Name, IndexAttribute->KeyValue.NameLength, "indices"));
			
			IndAccessIndex = IndexAttribute->KeyValue.Value->Primitive.I32;
		}

		STokenJSON* AccessorsToken = &ParsedJSON->Tokens[2];
		Assert(AccessorsToken && (AccessorsToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(AccessorsToken->KeyValue.Name, AccessorsToken->KeyValue.NameLength, "accessors"));

		STokenJSON* AccessorsArray = AccessorsToken->KeyValue.Value;
		Assert(AccessorsArray && (AccessorsArray->Type == TokenTypeJSON_Array));

		STokenJSON* BufferViewsToken = &ParsedJSON->Tokens[3];
		Assert(BufferViewsToken && (BufferViewsToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(BufferViewsToken->KeyValue.Name, BufferViewsToken->KeyValue.NameLength, "bufferViews"));
		
		STokenJSON* BufferViewsArray = BufferViewsToken->KeyValue.Value;
		Assert(BufferViewsArray && (BufferViewsArray->Type == TokenTypeJSON_Array));

		SAttributeInfoGLTF PositionAttributeInfo = GetAttributeInfoGLTF(&AccessorsArray->Tokens[PosAccessIndex], BufferViewsArray);
		SAttributeInfoGLTF NormalAttributeInfo = GetAttributeInfoGLTF(&AccessorsArray->Tokens[NormAccessIndex], BufferViewsArray);
		Assert(PositionAttributeInfo.Count == NormalAttributeInfo.Count);
		Assert((PositionAttributeInfo.CompCount == 3) && (PositionAttributeInfo.CompSize == 4));
		Assert((NormalAttributeInfo.CompCount == 3) && (NormalAttributeInfo.CompSize == 4));

		SAttributeInfoGLTF IndexAttributeInfo = {};
		if (bIsIndexed)
		{
			IndexAttributeInfo = GetAttributeInfoGLTF(&AccessorsArray->Tokens[IndAccessIndex], BufferViewsArray);
			Assert((IndexAttributeInfo.CompCount == 1) && (IndexAttributeInfo.CompSize == 4) || ((IndexAttributeInfo.CompSize == 2)));
		}

		STokenJSON* BuffersToken = &ParsedJSON->Tokens[4];
		Assert(BuffersToken && (BuffersToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(BuffersToken->KeyValue.Name, BuffersToken->KeyValue.NameLength, "buffers"));
		
		STokenJSON* BuffersArray = BuffersToken->KeyValue.Value;
		Assert(BuffersArray && (BuffersArray->Type == TokenTypeJSON_Array) && (BuffersArray->Tokens.Size == 1));

		STokenJSON* Buffer = &BuffersArray->Tokens[0];
		uint32_t BufferURILength = Buffer->Tokens[1].KeyValue.Value->Primitive.StringLength;
		const char* BufferURI = Buffer->Tokens[1].KeyValue.Value->Primitive.String;

		char BufferPath[128];
		ConcStrings(BufferPath, ArrayCount(BufferPath), "Models\\", BufferURI, BufferURILength);

		SReadEntireFileResult BufferFile = ReadEntireFile(BufferPath);

		Assert(MaxVertexCount >= PositionAttributeInfo.Count);
		VertexCount = PositionAttributeInfo.Count;
		IndexCount = 0;
		if (bIsIndexed)
		{
			Assert(MaxIndexCount >= IndexAttributeInfo.Count);
			IndexCount = IndexAttributeInfo.Count;
		}

		STokenJSON* OffsetToken = &ParsedJSON->Tokens[5];
		Assert(OffsetToken && (OffsetToken->Type == TokenTypeJSON_KeyValue) && CompareStrings(OffsetToken->KeyValue.Name, OffsetToken->KeyValue.NameLength, "offset"));

		vec3 PositionOffset = Vec3(OffsetToken->KeyValue.Value->Tokens[0].Primitive.Float, OffsetToken->KeyValue.Value->Tokens[1].Primitive.Float, OffsetToken->KeyValue.Value->Tokens[2].Primitive.Float);

		vec3* PosStartPtr = (vec3*) ((uint8_t*) BufferFile.Memory + PositionAttributeInfo.ByteOffset);
		vec3* NormStartPtr = (vec3*) ((uint8_t*) BufferFile.Memory + NormalAttributeInfo.ByteOffset);
		for (uint32_t I = 0; I < PositionAttributeInfo.Count; I++)
		{
			vec3 Pos = *PosStartPtr + PositionOffset;
			vec3 Normal = *NormStartPtr;
			
			Vertices[I].Pos = Pos;
			Vertices[I].Normal = Normal;

			if (PositionAttributeInfo.ByteStride != 0)
				PosStartPtr = (vec3*) ((uint8_t*) PosStartPtr + PositionAttributeInfo.ByteStride);
			else 
				PosStartPtr++;

			if (NormalAttributeInfo.ByteStride != 0)
				NormStartPtr = (vec3*) ((uint8_t*) NormStartPtr + NormalAttributeInfo.ByteStride);
			else 
				NormStartPtr++;
		}

		if (bIsIndexed)
		{
			if (IndexAttributeInfo.CompSize == 4)
			{
				uint32_t* IndStartPtr = (uint32_t*) ((uint8_t*) BufferFile.Memory + IndexAttributeInfo.ByteOffset);
				for (uint32_t I = 0; I < IndexAttributeInfo.Count; I++)
				{
					Indices[I] = *IndStartPtr;

					if (IndexAttributeInfo.ByteStride != 0)
						IndStartPtr = (uint32_t*) ((uint8_t*) IndStartPtr + IndexAttributeInfo.ByteStride);
					else 
						IndStartPtr++;
				}
			}
			else
			{
				Assert(IndexAttributeInfo.CompSize == 2);

				uint16_t* IndStartPtr = (uint16_t*) ((uint8_t*) BufferFile.Memory + IndexAttributeInfo.ByteOffset);
				for (uint32_t I = 0; I < IndexAttributeInfo.Count; I++)
				{
					Indices[I] = uint16_t(*IndStartPtr);

					if (IndexAttributeInfo.ByteStride != 0)
						IndStartPtr = (uint16_t*) ((uint8_t*) IndStartPtr + IndexAttributeInfo.ByteStride);
					else 
						IndStartPtr++;
				}
			}
		}

        FreeParsedJSON(ParsedJSON);
		FreeEntireFile(&BufferFile);
		FreeEntireFile(&FileData);

        return true;
    }
    else
    {
        return false;
    }
}