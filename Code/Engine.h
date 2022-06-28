#pragma once

#include "MemoryArena.h"

typedef uint32_t uint;
#include "shaders\\VoxelDimension.h"
#include "Assets.h"
#include "FileFormats.h"
#include "Particles.h"
#include "Audio.h"
#include "Entity.h"

struct SCamera
{
    vec3 Pos;

    vec3 Dir;
	vec3 Right;
	vec3 Up;

    float Pitch;
    float Head;

    float Near;
    float Far;
    float FoV;

	vec3 PrevDir;
	vec3 PrevRight;
	vec3 PrevUp;

	float PrevPitch;
	float PrevHead;

	vec3 OffsetFromPlayer;
};

enum EAOQuality
{
	AOQuality_Low,
	AOQuality_Medium,
	AOQuality_High,
	AOQuality_VeryHigh,

	AOQuality_Count
};

#include "VoxelCulling.cpp"
#include "ForwardVoxelRender.cpp"
#include "ForwardRender.cpp"
#include "ForwardParticleRender.cpp"
#include "Downscale.cpp"
#include "Exposure.cpp"
#include "PostProcessingBloom.cpp"
#include "PostProcessingTAA.cpp"
#include "PostProcessingTonemapping.cpp"
#include "HUDRender.cpp"
#include "DebugRender.cpp"
#include "Renderer.h"

struct SVoxels
{
	uint32_t ColorActive[LevelDimZ][LevelDimY][LevelDimX];
};

struct SLevel
{
	SVoxels Voxels;

	uint32_t EntityCount;
	SEntity Entities[128];

	uint32_t ParticleEmitterCount;
	SParticleEmitter ParticleEmitters[1];

	uint32_t PointLightCount;
	SPointLight PointLights[128];

	vec3 AmbientColor;
	vec3 AmbientConstant;
};

#include "Editor.h"

struct SVoxelToAdd
{
	uint32_t ID;
	uint8_t Red, Green, Blue;
};

struct SVoxelChangeColor
{
	uint32_t ID;
	uint8_t Red, Green, Blue;
};

struct SVoxelDraw
{
	vec3 Pos;
	uint32_t FirstInstance;
};

struct SParticleDraw
{
	vec3 Pos;
    float Scale;

    vec3 Color;
    float _Padding1;
};

enum ETextAlignment
{
	TextAlignment_Center,
	TextAlignment_Left,
	TextAlignment_Right
};

struct SText
{
	char String[64];

	vec2 Pos; // NOTE(georgii): Pos is in NDC coordinates. Text center position.
	vec2 Scale;

	float CurrentTime;
	float Time;

	bool bAppearance;
	float TimeToAppear;
	float TimeToStartAppear;

	vec4 Color;
	float BlendFactor;
	EFont Font;

	bool bMenuText;
	ETextAlignment Alignment;
};

struct SHeroControl
{
	vec3 Acceleration;
	bool bJump;

	bool bUseLamp;
};

enum EEngineMode
{
	EngineMode_Game,
	EngineMode_Editor,
};

struct SEngineState
{
    bool bInitialized;

	SGeometry Geometry;
	SRenderer Renderer;

	bool bSwapchainChanged;
	uint32_t NewInternalWidth;
	uint32_t NewInternalHeight;

	uint32_t LastFullscreenInternalWidth;
	uint32_t LastFullscreenInternalHeight;

	bool bSampleCountMSAAChanged;
	uint32_t NewSampleCountMSAA;

	SVoxelDraw VoxelDraws[LevelDimZ*LevelDimY*LevelDimX];
	uint32_t VoxelVisibilities[LevelDimZ*LevelDimY*LevelDimX];

	uint32_t ParticlesDrawCount;
	SParticleDraw ParticleDraws[1024];

	SCamera Camera;

	char LevelName[260];
	SLevel LevelBaseState;
	SLevel Level;

	uint32_t VoxelsToDeleteCount;
	uint32_t VoxelsToDelete[LevelDimZ*LevelDimY*LevelDimX];

	uint32_t VoxelsToAddCount;
	SVoxelToAdd VoxelsToAdd[LevelDimZ*LevelDimY*LevelDimX];

	uint32_t VoxelsToChangeColorCount;
	SVoxelChangeColor VoxelsToChangeColor[LevelDimZ*LevelDimY*LevelDimX];

	bool bReloadLevel;
	bool bReloadGame;

	SLoadedWAV LoadedSounds[32];
	SAudioState AudioState;

	SMemoryArena MemoryArena;

	uint32_t TextsToRenderCount;
	SText TextsToRender[64];

	EEngineMode EngineMode;

	bool bMenuOpened;
	float MenuOpenedBlend;

	bool bVignetteEnabled;

	float GameTime;

	// Debug and editor stuff
	bool bFlyMode;
	bool bHideEntities;
	bool bReloadLevelEditor;

	SLevel LevelGameStartState;
	bool bForceUpdateVoxels;

#ifndef ENGINE_RELEASE
	SEditorState EditorState;
#endif
};

inline void AddText(SEngineState* EngineState, const char* String, vec2 Pos, float Scale, vec4 Color = Vec4(1.0f), float Time = FloatMax, bool bAppearance = false, float TimeToAppear = 0.0f, float TimeToStartAppear = 0.0f)
{
	Assert(EngineState->TextsToRenderCount < ArrayCount(EngineState->TextsToRender));
	SText* Text = &EngineState->TextsToRender[EngineState->TextsToRenderCount++];

	uint32_t Length = StringLength(String);
	Assert(ArrayCount(Text->String) >= Length + 1);

	memcpy(Text->String, String, Length);
	Text->String[Length] = 0;

	Text->Pos = Pos;
	Text->Scale = Vec2(Scale, Scale);

	Text->CurrentTime = 0.0f;
	Text->Time = Time;
	
	Text->bAppearance = bAppearance;
	Text->TimeToAppear = TimeToAppear;
	Text->TimeToStartAppear = TimeToStartAppear;

	Text->Color = Color;
	Text->BlendFactor = 1.0f;
	Text->bMenuText = false;
	Text->Alignment = TextAlignment_Center;

	Text->Font = Font_KarminaRegular;
}

inline void AddTextMenu(SEngineState* EngineState, const char* String, vec2 Pos, float Scale, EFont Font, vec4 Color = Vec4(1.0f), float BlendFactor = 1.0f, ETextAlignment Alignment = TextAlignment_Center)
{
	Assert(EngineState->TextsToRenderCount < ArrayCount(EngineState->TextsToRender));
	SText* Text = &EngineState->TextsToRender[EngineState->TextsToRenderCount++];

	uint32_t Length = StringLength(String);
	Assert(ArrayCount(Text->String) >= Length + 1);

	memcpy(Text->String, String, Length);
	Text->String[Length] = 0;

	Text->Pos = Pos;
	Text->Scale = Vec2(Scale, Scale);

	Text->CurrentTime = 0.0f;
	Text->Time = 0.0f;
	
	Text->bAppearance = false;
	Text->TimeToAppear = 0.0f;
	Text->TimeToStartAppear = 0.0f;

	Text->Color = Color;
	Text->BlendFactor = BlendFactor;
	Text->bMenuText = true;
	Text->Alignment = Alignment;

	Text->Font = Font;
}

inline bool IsVoxelActive(const SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z)
{
	bool bResult = Voxels.ColorActive[Z][Y][X] & 1;
	return bResult;
}

inline void SetVoxelActive(SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z, bool bActive)
{
	Voxels.ColorActive[Z][Y][X] &= 0xFFFFFFFE;
	Voxels.ColorActive[Z][Y][X] |= uint32_t(bActive);
}

inline void SetVoxelColor(SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z, uint8_t Red, uint8_t Green, uint8_t Blue)
{
	uint32_t Color = (uint32_t(Red) << 24) | (uint32_t(Green) << 16) | (uint32_t(Blue) << 8);
	Voxels.ColorActive[Z][Y][X] &= 0x000000FF;
	Voxels.ColorActive[Z][Y][X] |= Color;
}

inline vec3 GetVoxelColorVec3(const SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z)
{
	float Red = (Voxels.ColorActive[Z][Y][X] >> 24) / 255.0f;
	float Green = ((Voxels.ColorActive[Z][Y][X] >> 16) & 0xFF) / 255.0f;
	float Blue = ((Voxels.ColorActive[Z][Y][X] >> 8) & 0xFF) / 255.0f;

	return Vec3(Red, Green, Blue);
}

void AddVoxelToLevel(SEngineState* EngineState, uint32_t X, uint32_t Y, uint32_t Z, uint8_t Red = 77, uint8_t Green = 77, uint8_t Blue = 77)
{
	Assert(EngineState->VoxelsToAddCount < ArrayCount(EngineState->VoxelsToAdd));

	uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
	SVoxelToAdd VoxelToAdd = { ID, Red, Green, Blue };
	EngineState->VoxelsToAdd[EngineState->VoxelsToAddCount++] = VoxelToAdd;
}

void MarkVoxelForDeletion(SEngineState* EngineState, uint32_t X, uint32_t Y, uint32_t Z)
{
	Assert(EngineState->VoxelsToDeleteCount < ArrayCount(EngineState->VoxelsToDelete));

	uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
	EngineState->VoxelsToDelete[EngineState->VoxelsToDeleteCount++] = ID;
}

void MarkVoxelForChangingColor(SEngineState* EngineState, uint32_t X, uint32_t Y, uint32_t Z, uint8_t Red, uint8_t Green, uint8_t Blue)
{
	Assert(EngineState->VoxelsToChangeColorCount < ArrayCount(EngineState->VoxelsToChangeColor));

	uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
	SVoxelChangeColor VoxelChangeColor = { ID, Red, Green, Blue };
	EngineState->VoxelsToChangeColor[EngineState->VoxelsToChangeColorCount++] = VoxelChangeColor;
}

SPointLight* AddPointLight(SLevel& Level, vec3 Pos, float Radius, vec4 Color)
{
	Assert(Level.PointLightCount < ArrayCount(Level.PointLights));
	SPointLight* PointLight = &Level.PointLights[Level.PointLightCount++];

	PointLight->Pos = Pos;
	PointLight->Radius = Radius;
	PointLight->Color = Color;

	return PointLight;
}

struct SRaytraceVoxelsResult
{
	bool bHit;
	float Distance;
	uint32_t X, Y, Z;
	uint32_t LastEmptyX, LastEmptyY, LastEmptyZ;

	vec3 VoxelColor;
};
SRaytraceVoxelsResult RaytraceVoxels(const SLevel* Level, vec3 RayP, vec3 RayDir, float MaxDistance, bool bTraceOutside = false)
{
	SRaytraceVoxelsResult Result = {};

	int32_t VoxelX = (int32_t)(RayP.x / VoxelDim);
	int32_t VoxelY = (int32_t)(RayP.y / VoxelDim);
	int32_t VoxelZ = (int32_t)(RayP.z / VoxelDim);

	float Distance = 0.0f;
	while (Distance < MaxDistance)
	{
		float XBorderDist = FLT_MAX;
		if (RayDir.x != 0.0f)
		{
			float PlaneD = (RayDir.x > 0) ? (VoxelX + 1)*VoxelDim : VoxelX*VoxelDim;
			XBorderDist = (PlaneD - RayP.x) / RayDir.x;
		}

		float YBorderDist = FLT_MAX;
		{
			if(RayDir.y != 0.0f)
			{
				float PlaneD = (RayDir.y > 0) ? (VoxelY + 1)*VoxelDim : VoxelY*VoxelDim;
				YBorderDist = (PlaneD - RayP.y) / RayDir.y;
			}
		}
		
		float ZBorderDist = FLT_MAX;
		{
			if(RayDir.z != 0.0f)
			{
				float PlaneD = (RayDir.z > 0) ? (VoxelZ + 1)*VoxelDim : VoxelZ*VoxelDim;
				ZBorderDist = (PlaneD - RayP.z) / RayDir.z;
			}
		}

		if (XBorderDist <= YBorderDist)
		{
			if (XBorderDist <= ZBorderDist)
			{
				VoxelX = (RayDir.x > 0.0f) ? VoxelX + 1 : VoxelX - 1;
				RayP += RayDir*XBorderDist;
				Distance += XBorderDist;
			}
			else
			{
				VoxelZ = (RayDir.z > 0.0f) ? VoxelZ + 1 : VoxelZ - 1;
				RayP += RayDir*ZBorderDist;
				Distance += ZBorderDist;
			}
		}
		else
		{
			if (YBorderDist <= ZBorderDist)
			{
				VoxelY = (RayDir.y > 0.0f) ? VoxelY + 1 : VoxelY - 1;
				RayP += RayDir*YBorderDist;
				Distance += YBorderDist;
			}
			else
			{
				VoxelZ = (RayDir.z > 0.0f) ? VoxelZ + 1 : VoxelZ - 1;
				RayP += RayDir*ZBorderDist;
				Distance += ZBorderDist;
			}
		}

		if ((VoxelX < 0) || (VoxelX > LevelDimX - 1) || (VoxelY < 0) || (VoxelY > LevelDimY - 1) || (VoxelZ < 0) || (VoxelZ > LevelDimZ - 1))
		{
			if (bTraceOutside)
				continue;
			break;
		}

		if (IsVoxelActive(Level->Voxels, VoxelX, VoxelY, VoxelZ))
		{
			Result.bHit = true;
			Result.Distance = Distance;
			Result.X = VoxelX; Result.Y = VoxelY; Result.Z = VoxelZ;
			Result.VoxelColor = GetVoxelColorVec3(Level->Voxels, VoxelX, VoxelY, VoxelZ);
			break;
		}
		else
		{
			if (!((VoxelX < 0) || (VoxelX > LevelDimX - 1) || (VoxelY < 0) || (VoxelY > LevelDimY - 1) || (VoxelZ < 0) || (VoxelZ > LevelDimZ - 1)))
			{
				Result.LastEmptyX = VoxelX; Result.LastEmptyY = VoxelY; Result.LastEmptyZ = VoxelZ;
			}
		}
	}

	return Result;
}

struct SDepthPyramidInfoResult
{
	uint32_t MipCount;
	uint32_t Width, Height;
};
SDepthPyramidInfoResult GetDepthPyramidInfo(uint32_t SourceImageWidth, uint32_t SourceImageHeight)
{
	SDepthPyramidInfoResult Result = {};
	Result.Width = NextOrEqualPowerOfTwo(SourceImageWidth);
	Result.Height = NextOrEqualPowerOfTwo(SourceImageHeight);
	Result.MipCount = GetMipsCount(Result.Width, Result.Height);

	return Result;
}

#define LEVEL_MAX_FILE_VERSION 1

uint8_t* LoadLevel(SEngineState* EngineState, SLevel* Level, const SReadEntireFileResult& File, char* Path, vec3 HeroVelocity = Vec3(0.0f))
{
	memset(Level, 0, sizeof(SLevel));

	uint8_t* EndPointer = (uint8_t*) File.Memory;
	if (File.Memory)
	{
		uint32_t FileVersion = *(uint32_t*) File.Memory;

		if (FileVersion == LEVEL_MAX_FILE_VERSION)
		{
			uint8_t* LevelMemory = (uint8_t*) File.Memory + sizeof(uint32_t);

			// Load voxels
			memcpy(&Level->Voxels, LevelMemory, sizeof(Level->Voxels));
			LevelMemory += sizeof(Level->Voxels);

			// Load entities
			AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
			memcpy(&Level->EntityCount, LevelMemory, sizeof(uint32_t));
			LevelMemory += sizeof(uint32_t);

			Assert(Level->EntityCount < ArrayCount(Level->Entities));

			for (uint32_t I = 0; I < Level->EntityCount; I++)
			{
				AlignAddress(&LevelMemory, GetAlignmentOf(SEntity));
				memcpy(&Level->Entities[I].Type, LevelMemory, sizeof(EEntityType));
				LevelMemory += sizeof(EEntityType);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].Pos, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].Velocity, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].Dim, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec4));
				memcpy(&Level->Entities[I].Orientation, LevelMemory, sizeof(vec4));
				LevelMemory += sizeof(vec4);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].LampBaseOffset, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].LampOffset, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec2));
				memcpy(&Level->Entities[I].LampRotationOffset, LevelMemory, sizeof(vec2)); 
				LevelMemory += sizeof(vec2);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].PrevLampOffset, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);

				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].Scale, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);

				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].Speed, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);
				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].Drag, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);
				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].JumpPower, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);

				AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
				memcpy(&Level->Entities[I].DoorIndex, LevelMemory, sizeof(uint32_t));
				LevelMemory += sizeof(uint32_t);

				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].Alpha, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].Color, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].PrevPos, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec4));
				memcpy(&Level->Entities[I].PrevOrientation, LevelMemory, sizeof(vec4));
				LevelMemory += sizeof(vec4);

				AlignAddress(&LevelMemory, GetAlignmentOf(SPointLight));
				memcpy(&Level->Entities[I].PointLight, LevelMemory, sizeof(SPointLight));
				LevelMemory += sizeof(SPointLight);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].CurrentColor, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].TargetColor, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);

				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].TimeToMove, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);
				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].TimeToMoveCurrent, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);

				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].BasePos, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);
				AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
				memcpy(&Level->Entities[I].TargetOffset, LevelMemory, sizeof(vec3));
				LevelMemory += sizeof(vec3);

				AlignAddress(&LevelMemory, GetAlignmentOf(bool));
				memcpy(&Level->Entities[I].bCollisionWithHeroStarted, LevelMemory, sizeof(bool));
				LevelMemory += sizeof(bool);
				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].CollisionWithHeroTimePassed, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);

				AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
				memcpy(&Level->Entities[I].MeshIndex, LevelMemory, sizeof(uint32_t));
				LevelMemory += sizeof(uint32_t);

				AlignAddress(&LevelMemory, GetAlignmentOf(char));
				memcpy(&Level->Entities[I].TargetLevelName, LevelMemory, sizeof(SEntity::MessageText));
				LevelMemory += sizeof(SEntity::MessageText);

				AlignAddress(&LevelMemory, GetAlignmentOf(bool));
				memcpy(&Level->Entities[I].bGoBack, LevelMemory, sizeof(bool));
				LevelMemory += sizeof(bool);

				AlignAddress(&LevelMemory, GetAlignmentOf(bool));
				memcpy(&Level->Entities[I].bForceClose, LevelMemory, sizeof(bool));
				LevelMemory += sizeof(bool);

				AlignAddress(&LevelMemory, GetAlignmentOf(bool));
				memcpy(&Level->Entities[I].bRemoved, LevelMemory, sizeof(bool));
				LevelMemory += sizeof(bool);

				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				Level->Entities[I].DistanceToCam = 0.0f;
				LevelMemory += sizeof(float);
			}

			AlignAddress(&LevelMemory, GetAlignmentOf(SEntity));
			LevelMemory += sizeof(SEntity) * (ArrayCount(Level->Entities) - Level->EntityCount);

			// Load particle emitters
			AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
			memcpy(&Level->ParticleEmitterCount, LevelMemory, sizeof(uint32_t));
			LevelMemory += sizeof(uint32_t);

			Assert(Level->ParticleEmitterCount < ArrayCount(Level->ParticleEmitters));

			AlignAddress(&LevelMemory, GetAlignmentOf(SParticleEmitter));
			memcpy(Level->ParticleEmitters, LevelMemory, sizeof(Level->ParticleEmitters));
			LevelMemory += sizeof(Level->ParticleEmitters);

			// Load point lights
			AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
			memcpy(&Level->PointLightCount, LevelMemory, sizeof(uint32_t));
			LevelMemory += sizeof(uint32_t);

			Assert(Level->PointLightCount < ArrayCount(Level->PointLights));

			AlignAddress(&LevelMemory, GetAlignmentOf(SPointLight));
			memcpy(Level->PointLights, LevelMemory, sizeof(Level->PointLights));
			LevelMemory += sizeof(Level->PointLights);

			AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
			memcpy(&Level->AmbientColor, LevelMemory, sizeof(Level->AmbientColor));
			LevelMemory += sizeof(Level->AmbientColor);
			AlignAddress(&LevelMemory, GetAlignmentOf(vec3));
			memcpy(&Level->AmbientConstant, LevelMemory, sizeof(Level->AmbientConstant));
			LevelMemory += sizeof(Level->AmbientConstant);

			EndPointer = LevelMemory;
		}

#ifndef ENGINE_RELEASE
		EngineState->LevelGameStartState = *Level;
		EngineState->EditorState.LevelHistoryHead = EngineState->EditorState.LevelHistoryTail = 0;
#endif

		memcpy(EngineState->LevelName, Path, StringLength(Path) + 1);

		Assert(Level->Entities[0].Type == Entity_Hero);
		Level->Entities[0].Velocity = HeroVelocity;

		EngineState->bReloadLevel = true;
		EngineState->bReloadGame = true;
	}

	return EndPointer;
}

void LoadLevel(SEngineState* EngineState, SLevel* Level, const char* LevelName, bool bAddLevelsPath = true, vec3 HeroVelocity = Vec3(0.0f))
{
	char Path[264] = {};
	ConcStrings(Path, sizeof(Path), bAddLevelsPath ? "Levels\\" : "", LevelName);

	SReadEntireFileResult File = ReadEntireFile(Path);
	LoadLevel(EngineState, Level, File, Path, HeroVelocity);

	free(File.Memory);
}

void ReloadGameLevel(SEngineState* EngineState, bool bResetCameraPosition = true, bool bReloadGame = true)
{
#ifndef ENGINE_RELEASE
	EngineState->bReloadLevelEditor = true;
#else
	EngineState->bReloadLevel = true;
#endif

	if (bResetCameraPosition)
	{
		SEntity* HeroEntity = EngineState->bReloadLevelEditor ? &EngineState->LevelGameStartState.Entities[0] : &EngineState->LevelBaseState.Entities[0];
        Assert(HeroEntity->Type == Entity_Hero);

	    EngineState->Camera.Pitch = 0.0f;
	    EngineState->Camera.Head = HeroEntity->Orientation.y;
	}

	EngineState->bReloadGame = bReloadGame;
}

void FixDoorIndex(SLevel* Level, uint32_t OldDoorIndex, uint32_t NewDoorIndex)
{
	for (uint32_t I = 0; I < Level->EntityCount; I++)
	{
		SEntity* Entity = Level->Entities + I;

		if ((Entity->Type == Entity_Checkpoint) && (Entity->DoorIndex == OldDoorIndex))
		{
			Entity->DoorIndex = NewDoorIndex;
		}
	}
}

struct SConfigFileItem
{
	char Name[64];
	float Value;
};

struct SParsedConfigFile
{
	uint32_t ItemCount;
	SConfigFileItem Items[64];
};

SParsedConfigFile ParseConfigFile(const char* ConfigFileName)
{
	SParsedConfigFile ConfigFile = {};

	SReadEntireFileResult File = ReadEntireTextFile(ConfigFileName);
	if (File.Memory && File.Size)
	{
		char* FileText = (char*) File.Memory;

		char* LineStart = FileText;
		uint32_t LineLength = 0;
		for (int I = 0; I < File.Size + 1; I++)
		{
			if ((FileText[I] != '\r') && (FileText[I] != '\n') && (FileText[I] != 0))
			{
				LineLength++;
			}
			else
			{
				char* NameStart = (char*) LineStart;
				uint32_t NameLength = 0;
				for (uint32_t J = 0; J < LineLength; J++)
				{
					if (NameStart[J] != ' ')
					{
						NameLength++;
					}
					else
					{
						break;
					}
				}

				char *ValueStart = NameStart + NameLength + 1;
				uint32_t ValueLength = LineLength - NameLength - 1;
				
				SConfigFileItem* Item = ConfigFile.Items + ConfigFile.ItemCount++;
				ConcStrings(Item->Name, ArrayCount(Item->Name), "", NameStart, NameLength);
				Item->Value = StrToFloat(ValueStart, ValueLength);

				if (FileText[I] == '\r')
				{
					I++;
				}
				LineStart = FileText + I + 1;
				LineLength = 0;
			}
		}

		free(File.Memory);
	}

	return ConfigFile;
}