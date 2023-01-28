#pragma once

#include <stdio.h>
#include "Math.h"
#include "Entity.h"

struct SVoxels
{
	// NOTE(georgii): 24 most significant bits - color, next 4 bits - reflectivity, next 3 bits - roughness, the least significant bit - active or not
	uint32_t ColorMatActive[LevelDimZ][LevelDimY][LevelDimX];
};

struct SLevel
{
	SVoxels Voxels;

	uint32_t EntityCount;
	SEntity Entities[128];

	uint32_t LightCount;
	SLight Lights[128];

	vec3 AmbientColor;
	vec3 AmbientConstant;

	vec3 FogInscatteringColor;
	float FogDensity;
	float FogHeightFalloff;
	float MinFogOpacity;
	float FogHeight;
	float FogCutoffDistance;
};

#define LEVEL_MAX_FILE_VERSION 1

void SaveLight(FILE* File, const SLight* Light)
{
	fwrite(&Light->Pos, sizeof(Light->Pos), 1, File);
	fwrite(&Light->Radius, sizeof(Light->Radius), 1, File);
	fwrite(&Light->Color, sizeof(Light->Color), 1, File);
	fwrite(&Light->Type, sizeof(Light->Type), 1, File);
	fwrite(&Light->Rotation, sizeof(Light->Rotation), 1, File);
	fwrite(&Light->Cutoff, sizeof(Light->Cutoff), 1, File);
}

// TODO(georgii): I think I don't have to serialize all of the properties? Maybe I can fix this in the future to minimize sizes on disk. But who cares....
void SaveLevel(const SLevel& Level, FILE* File)
{
    // Save file version
    const uint32_t FileVersion = LEVEL_MAX_FILE_VERSION;
    fwrite(&FileVersion, sizeof(FileVersion), 1, File);

    // Save voxels
    fwrite(&Level.Voxels, sizeof(Level.Voxels), 1, File);

    // Save entities
    fwrite(&Level.EntityCount, sizeof(Level.EntityCount), 1, File);
    for (uint32_t I = 0; I < Level.EntityCount; I++)
    {
        const SEntity* Entity = Level.Entities + I;

        fwrite(&Entity->Type, sizeof(Entity->Type), 1, File);
        fwrite(&Entity->Pos, sizeof(Entity->Pos), 1, File);
        fwrite(&Entity->Velocity, sizeof(Entity->Velocity), 1, File);
        fwrite(&Entity->Dim, sizeof(Entity->Dim), 1, File);
        fwrite(&Entity->Orientation, sizeof(Entity->Orientation), 1, File);
        fwrite(&Entity->LampBaseOffset, sizeof(Entity->LampBaseOffset), 1, File);
        fwrite(&Entity->LampOffset, sizeof(Entity->LampOffset), 1, File);
        fwrite(&Entity->LampRotationOffset, sizeof(Entity->LampRotationOffset), 1, File);
        fwrite(&Entity->PrevLampOffset, sizeof(Entity->PrevLampOffset), 1, File);
        fwrite(&Entity->Scale, sizeof(Entity->Scale), 1, File);
        fwrite(&Entity->Speed, sizeof(Entity->Speed), 1, File);
        fwrite(&Entity->Drag, sizeof(Entity->Drag), 1, File);
        fwrite(&Entity->DoorIndex, sizeof(Entity->DoorIndex), 1, File);
        fwrite(&Entity->Alpha, sizeof(Entity->Alpha), 1, File);
        fwrite(&Entity->Color, sizeof(Entity->Color), 1, File);
        fwrite(&Entity->PrevPos, sizeof(Entity->PrevPos), 1, File);
        fwrite(&Entity->PrevOrientation, sizeof(Entity->PrevOrientation), 1, File);

        // Save entity light
        SaveLight(File, &Entity->Light);

        fwrite(&Entity->MessagePos, sizeof(Entity->MessagePos), 1, File);
        fwrite(&Entity->MessageScale, sizeof(Entity->MessageScale), 1, File);
        fwrite(&Entity->MessageLifeTime, sizeof(Entity->MessageLifeTime), 1, File);
        fwrite(&Entity->MessageTimeToAppear, sizeof(Entity->MessageTimeToAppear), 1, File);
        fwrite(&Entity->MessageTimeToStartAppear, sizeof(Entity->MessageTimeToStartAppear), 1, File);
        fwrite(&Entity->AnimationColor, sizeof(Entity->AnimationColor), 1, File);
        fwrite(&Entity->TargetColor, sizeof(Entity->TargetColor), 1, File);
        fwrite(&Entity->TimeToDisappear, sizeof(Entity->TimeToDisappear), 1, File);
        fwrite(&Entity->TimeToDisappearCurrent, sizeof(Entity->TimeToDisappearCurrent), 1, File);
        fwrite(&Entity->BasePos, sizeof(Entity->BasePos), 1, File);
        fwrite(&Entity->TargetOffset, sizeof(Entity->TargetOffset), 1, File);
        fwrite(&Entity->bOpen, sizeof(Entity->bOpen), 1, File);
        fwrite(&Entity->ColorScale, sizeof(Entity->ColorScale), 1, File);
        fwrite(&Entity->MeshIndex, sizeof(Entity->MeshIndex), 1, File);
        fwrite(Entity->TargetLevelName, sizeof(Entity->TargetLevelName), 1, File);
        fwrite(&Entity->bGoBack, sizeof(Entity->bGoBack), 1, File);
        fwrite(&Entity->CheckpointIndex, sizeof(Entity->CheckpointIndex), 1, File);
        fwrite(&Entity->bLoadLevel, sizeof(Entity->bLoadLevel), 1, File);
        fwrite(&Entity->bRemoved, sizeof(Entity->bRemoved), 1, File);
        fwrite(&Entity->DistanceToCam, sizeof(Entity->DistanceToCam), 1, File);
    }

    // Save lights
    fwrite(&Level.LightCount, sizeof(Level.LightCount), 1, File);
    for (uint32_t I = 0; I < Level.LightCount; I++)
    {
        const SLight* Light = Level.Lights + I;
        SaveLight(File, Light);
    }

    // Save lighting and fog info
    fwrite(&Level.AmbientColor, sizeof(Level.AmbientColor), 1, File);
    fwrite(&Level.AmbientConstant, sizeof(Level.AmbientConstant), 1, File);
    fwrite(&Level.FogInscatteringColor, sizeof(Level.FogInscatteringColor), 1, File);
    fwrite(&Level.FogDensity, sizeof(Level.FogDensity), 1, File);
    fwrite(&Level.FogHeightFalloff, sizeof(Level.FogHeightFalloff), 1, File);
    fwrite(&Level.MinFogOpacity, sizeof(Level.MinFogOpacity), 1, File);
    fwrite(&Level.FogHeight, sizeof(Level.FogHeight), 1, File);
    fwrite(&Level.FogCutoffDistance, sizeof(Level.FogCutoffDistance), 1, File);
}

void SaveLevel(const SLevel& Level, const char* Path)
{
	FILE* File = fopen(Path, "wb");
	Assert(File);
	if (File)
	{
		SaveLevel(Level, File);

		fclose(File);
	}
}

void LoadLight(FILE* File, SLight* Light)
{
	fread(&Light->Pos, sizeof(Light->Pos), 1, File);
	fread(&Light->Radius, sizeof(Light->Radius), 1, File);
	fread(&Light->Color, sizeof(Light->Color), 1, File);
	fread(&Light->Type, sizeof(Light->Type), 1, File);
	fread(&Light->Rotation, sizeof(Light->Rotation), 1, File);
	fread(&Light->Cutoff, sizeof(Light->Cutoff), 1, File);
}

void LoadLevel(SLevel& Level, FILE* File)
{
    // Load file version
    uint32_t FileVersion;
    fread(&FileVersion, sizeof(FileVersion), 1, File);

    // Load voxels
    fread(&Level.Voxels, sizeof(Level.Voxels), 1, File);

    // Load entities
    fread(&Level.EntityCount, sizeof(Level.EntityCount), 1, File);
    for (uint32_t I = 0; I < Level.EntityCount; I++)
    {
        SEntity* Entity = Level.Entities + I;

        fread(&Entity->Type, sizeof(Entity->Type), 1, File);
        fread(&Entity->Pos, sizeof(Entity->Pos), 1, File);
        fread(&Entity->Velocity, sizeof(Entity->Velocity), 1, File);
        fread(&Entity->Dim, sizeof(Entity->Dim), 1, File);
        fread(&Entity->Orientation, sizeof(Entity->Orientation), 1, File);
        fread(&Entity->LampBaseOffset, sizeof(Entity->LampBaseOffset), 1, File);
        fread(&Entity->LampOffset, sizeof(Entity->LampOffset), 1, File);
        fread(&Entity->LampRotationOffset, sizeof(Entity->LampRotationOffset), 1, File);
        fread(&Entity->PrevLampOffset, sizeof(Entity->PrevLampOffset), 1, File);
        fread(&Entity->Scale, sizeof(Entity->Scale), 1, File);
        fread(&Entity->Speed, sizeof(Entity->Speed), 1, File);
        fread(&Entity->Drag, sizeof(Entity->Drag), 1, File);
        fread(&Entity->DoorIndex, sizeof(Entity->DoorIndex), 1, File);
        fread(&Entity->Alpha, sizeof(Entity->Alpha), 1, File);
        fread(&Entity->Color, sizeof(Entity->Color), 1, File);
        fread(&Entity->PrevPos, sizeof(Entity->PrevPos), 1, File);
        fread(&Entity->PrevOrientation, sizeof(Entity->PrevOrientation), 1, File);

        // Load entity light
        LoadLight(File, &Entity->Light);

        fread(&Entity->MessagePos, sizeof(Entity->MessagePos), 1, File);
        fread(&Entity->MessageScale, sizeof(Entity->MessageScale), 1, File);
        fread(&Entity->MessageLifeTime, sizeof(Entity->MessageLifeTime), 1, File);
        fread(&Entity->MessageTimeToAppear, sizeof(Entity->MessageTimeToAppear), 1, File);
        fread(&Entity->MessageTimeToStartAppear, sizeof(Entity->MessageTimeToStartAppear), 1, File);
        fread(&Entity->AnimationColor, sizeof(Entity->AnimationColor), 1, File);
        fread(&Entity->TargetColor, sizeof(Entity->TargetColor), 1, File);
        fread(&Entity->TimeToDisappear, sizeof(Entity->TimeToDisappear), 1, File);
        fread(&Entity->TimeToDisappearCurrent, sizeof(Entity->TimeToDisappearCurrent), 1, File);
        fread(&Entity->BasePos, sizeof(Entity->BasePos), 1, File);
        fread(&Entity->TargetOffset, sizeof(Entity->TargetOffset), 1, File);
        fread(&Entity->bOpen, sizeof(Entity->bOpen), 1, File);
        fread(&Entity->ColorScale, sizeof(Entity->ColorScale), 1, File);
        fread(&Entity->MeshIndex, sizeof(Entity->MeshIndex), 1, File);
        fread(Entity->TargetLevelName, sizeof(Entity->TargetLevelName), 1, File);
        fread(&Entity->bGoBack, sizeof(Entity->bGoBack), 1, File);
        fread(&Entity->CheckpointIndex, sizeof(Entity->CheckpointIndex), 1, File);
        fread(&Entity->bLoadLevel, sizeof(Entity->bLoadLevel), 1, File);
        fread(&Entity->bRemoved, sizeof(Entity->bRemoved), 1, File);
        fread(&Entity->DistanceToCam, sizeof(Entity->DistanceToCam), 1, File);
    }

    // Load lights
    fread(&Level.LightCount, sizeof(Level.LightCount), 1, File);
    for (uint32_t I = 0; I < Level.LightCount; I++)
    {
        SLight* Light = Level.Lights + I;
        LoadLight(File, Light);
    }

    // Load lighting and fog info
    fread(&Level.AmbientColor, sizeof(Level.AmbientColor), 1, File);
    fread(&Level.AmbientConstant, sizeof(Level.AmbientConstant), 1, File);
    fread(&Level.FogInscatteringColor, sizeof(Level.FogInscatteringColor), 1, File);
    fread(&Level.FogDensity, sizeof(Level.FogDensity), 1, File);
    fread(&Level.FogHeightFalloff, sizeof(Level.FogHeightFalloff), 1, File);
    fread(&Level.MinFogOpacity, sizeof(Level.MinFogOpacity), 1, File);
    fread(&Level.FogHeight, sizeof(Level.FogHeight), 1, File);
    fread(&Level.FogCutoffDistance, sizeof(Level.FogCutoffDistance), 1, File);
}

bool LoadLevel(SLevel& Level, const char* Path)
{
    memset(&Level, 0, sizeof(SLevel));

    FILE* File = fopen(Path, "rb");
	Assert(File);
	if (File)
	{
		LoadLevel(Level, File);
		fclose(File);

        return true;
	}

    return false;
}