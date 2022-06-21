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

struct SCameraBuffer
{
	mat4 View;
	mat4 Proj;
	mat4 ProjUnjittered;

	mat4 PrevView;
	mat4 PrevProj;

	vec4 Pos;
	vec4 Viewport;
	vec4 Frustums[6];
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

struct SVoxelGameInfo
{
	bool bDestroyable;
};

struct SVoxels
{
	uint32_t ColorActive[LevelDimZ][LevelDimY][LevelDimX];
	SVoxelGameInfo GameInfo[LevelDimZ][LevelDimY][LevelDimX];
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

struct SHUDProjectionBuffer
{
	mat4 OrthoProj;
};

struct SLightBuffer
{
	vec4 AmbientColor; // w - unused
	vec4 AmbientConstant; // w - unused
};

struct SText
{
	char String[64];

	vec2 Pos; // NOTE(georgii): Pos is in NDC coordinates. Text center position.
	float Scale;

	float CurrentTime;
	float Time;

	bool bAppearance;
	float TimeToAppear;
	float TimeToStartAppear;
};

struct SHeroControl
{
	vec3 Acceleration;
	bool bJump;

	bool bUseLamp;
};

enum EGameMode
{
	GameMode_Game,
	GameMode_Editor,
};

struct SGameState
{
    bool bInitialized;

	SGeometry Geometry;

	SRenderer Renderer;

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

	vec3 LastBaseLevelPos;
	float LastBaseLevelGatesAngle;

	bool bReloadLevel;

	SLoadedWAV LoadedSounds[32];
	SAudioState AudioState;
	SPlayingSound* BackgroundSound;

	SMemoryArena MemoryArena;

	uint32_t TextsToRenderCount;
	SText TextsToRender[8];

	vec3 LastCheckpointPos;

    bool bDeathAnimation;
    float DeathAnimationSpeed;
    float DeathAnimationLengthMoved;
    vec3 DeathPos;
	vec3 DeathAnimationTargetPos;

	// NOTE(georgii): Don't wanna resave every level when I change this params for hero, so I just store them in SGameState.
	float HeroSpeed;
	float HeroDrag;
	float HeroJumpPower;
	float HeroLampDistance;

	EGameMode GameMode;

	// Debug and editor stuff
	bool bFlyMode;
	bool bHideEntities;
	bool bReloadLevelEditor;

	SLevel LevelGameStartState;
	bool bForceUpdateVoxels;

    uint8_t CurrentCheckpointIndex;

#ifndef ENGINE_RELEASE
	SEditorState EditorState;
#endif
};

inline void AddText(SGameState* GameState, const char* String, vec2 Pos, float Scale, float Time, bool bAppearance = false, float TimeToAppear = 0.0f, float TimeToStartAppear = 0.0f)
{
	Assert(GameState->TextsToRenderCount < ArrayCount(GameState->TextsToRender));
	SText* Text = &GameState->TextsToRender[GameState->TextsToRenderCount++];

	uint32_t Length = StringLength(String);
	Assert(ArrayCount(Text->String) >= Length + 1);

	memcpy(Text->String, String, Length);
	Text->String[Length] = 0;

	Text->Pos = Pos;
	Text->Scale = Scale;

	Text->CurrentTime = 0.0f;
	Text->Time = Time;
	
	Text->bAppearance = bAppearance;
	Text->TimeToAppear = TimeToAppear;
	Text->TimeToStartAppear = TimeToStartAppear;
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

inline bool IsVoxelDestroyable(const SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z)
{
	bool bResult = Voxels.GameInfo[Z][Y][X].bDestroyable;
	return bResult;
}

inline void SetVoxelDestroyable(SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z, bool bDestroyable)
{
	Voxels.GameInfo[Z][Y][X].bDestroyable = bDestroyable;
}

inline vec3 GetVoxelColorVec3(const SVoxels& Voxels, uint32_t X, uint32_t Y, uint32_t Z)
{
	float Red = (Voxels.ColorActive[Z][Y][X] >> 24) / 255.0f;
	float Green = ((Voxels.ColorActive[Z][Y][X] >> 16) & 0xFF) / 255.0f;
	float Blue = ((Voxels.ColorActive[Z][Y][X] >> 8) & 0xFF) / 255.0f;

	return Vec3(Red, Green, Blue);
}

void AddVoxelToLevel(SGameState* GameState, uint32_t X, uint32_t Y, uint32_t Z, uint8_t Red = 77, uint8_t Green = 77, uint8_t Blue = 77)
{
	Assert(GameState->VoxelsToAddCount < ArrayCount(GameState->VoxelsToAdd));

	uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
	SVoxelToAdd VoxelToAdd = { ID, Red, Green, Blue };
	GameState->VoxelsToAdd[GameState->VoxelsToAddCount++] = VoxelToAdd;
}

void MarkVoxelForDeletion(SGameState* GameState, uint32_t X, uint32_t Y, uint32_t Z)
{
	Assert(GameState->VoxelsToDeleteCount < ArrayCount(GameState->VoxelsToDelete));

	uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
	GameState->VoxelsToDelete[GameState->VoxelsToDeleteCount++] = ID;
}

void MarkVoxelForChangingColor(SGameState* GameState, uint32_t X, uint32_t Y, uint32_t Z, uint8_t Red, uint8_t Green, uint8_t Blue)
{
	Assert(GameState->VoxelsToChangeColorCount < ArrayCount(GameState->VoxelsToChangeColor));

	uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
	SVoxelChangeColor VoxelChangeColor = { ID, Red, Green, Blue };
	GameState->VoxelsToChangeColor[GameState->VoxelsToChangeColorCount++] = VoxelChangeColor;
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

void LoadLevel(SGameState* GameState, SLevel* Level, const char* LevelName, vec3 HeroVelocity = Vec3(0.0f))
{
	char Path[264] = {};
	ConcStrings(Path, sizeof(Path), "Levels\\", LevelName);

	memset(Level, 0, sizeof(SLevel));
	ReadEntireFileResult File = ReadEntireFile(Path);
	if (File.Memory)
	{
		uint32_t FileVersion = *(uint32_t*) File.Memory;

		if (FileVersion == 1)
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

				if (Level->Entities[I].Type == Entity_Door)
				{
					Level->Entities[I].Color = Vec3(0.0f);
				}

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

				Level->Entities[I].BasePos = Level->Entities[I].Pos;

				AlignAddress(&LevelMemory, GetAlignmentOf(bool));
				memcpy(&Level->Entities[I].bCollisionWithHeroStarted, LevelMemory, sizeof(bool));
				LevelMemory += sizeof(bool);
				AlignAddress(&LevelMemory, GetAlignmentOf(float));
				memcpy(&Level->Entities[I].CollisionWithHeroTimePassed, LevelMemory, sizeof(float));
				LevelMemory += sizeof(float);

				AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
				memcpy(&Level->Entities[I].MeshIndex, LevelMemory, sizeof(uint32_t));
				LevelMemory += sizeof(uint32_t);

				if (Level->Entities[I].Type == Entity_Door)
				{
					Level->Entities[I].MeshIndex = 2;
				}

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
		}

		free(File.Memory);

#ifndef ENGINE_RELEASE
		GameState->LevelGameStartState = *Level;
		GameState->EditorState.LevelHistoryHead = GameState->EditorState.LevelHistoryTail = 0;
#endif

		memcpy(GameState->LevelName, Path, StringLength(Path) + 1);

		Assert(Level->Entities[0].Type == Entity_Hero);
		Level->Entities[0].Velocity = HeroVelocity;

		GameState->bReloadLevel = true;
	}
}

void ReloadGameLevel(SGameState* GameState, bool bDeath = false)
{
#ifndef ENGINE_RELEASE
	GameState->bReloadLevelEditor = true;
#else
	GameState->bReloadLevel = true;
#endif

    if (bDeath)
    {
        SEntity* HeroEntity = &GameState->Level.Entities[0];
        Assert(HeroEntity->Type == Entity_Hero);

        GameState->bDeathAnimation = true;
        GameState->DeathPos = HeroEntity->Pos;
		GameState->DeathAnimationTargetPos = GameState->LastCheckpointPos;
    }
	else
	{
		SEntity* HeroEntity = GameState->bReloadLevelEditor ? &GameState->LevelGameStartState.Entities[0] : &GameState->LevelBaseState.Entities[0];
        Assert(HeroEntity->Type == Entity_Hero);

	    GameState->Camera.Pitch = 0.0f;
	    GameState->Camera.Head = HeroEntity->Orientation.y;
	}
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