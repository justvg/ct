#pragma once

#ifndef ENGINE_RELEASE

enum EEditorHelpersMode
{
	EditorHelpersMode_Translation,
	EditorHelpersMode_Rotation,
	EditorHelpersMode_Scale
};

enum ESelectedArrowType
{
	SelectedArrow_None,

	SelectedArrow_Red,
	SelectedArrow_Green,
	SelectedArrow_Blue,

	SelectedArrow_RedTargetOffset,
	SelectedArrow_GreenTargetOffset,
	SelectedArrow_BlueTargetOffset,
};

enum ESelectedDimHelperType
{
	SelectedDimHelper_None,

	SelectedDimHelper_Red,
	SelectedDimHelper_Green,
	SelectedDimHelper_Blue,
};

enum EWallSetMode
{
	WallSetMode_None,
	
	WallSetMode_XY,
	WallSetMode_ZY,
	WallSetMode_XZ
};

struct SEditorState
{
	float CameraSpeed;

	bool bIsImguiWindowHovered;
	bool bIsImguiWindowFocused;

	bool bBuildingMode;

	bool bSelectDoor;

	uint32_t SelectedVoxelsCount;
	uint32_t SelectedVoxels[LevelDimZ*LevelDimY*LevelDimX];
	vec3 SelectedVoxelColorFloat;
	float SelectedVoxelReflectFloat;
	float SelectedVoxelRoughFloat;

	uint32_t BuildingVoxelsToAddCount;
	uint32_t BuildingVoxelsToAdd[LevelDimZ*LevelDimY*LevelDimX];

	EWallSetMode WallSetMode;
	vec3 WallSetLastWorldPos;
	uint32_t WallSetFirstID;
	int32_t WallSetWidth;
	int32_t WallSetHeight;

	EEditorHelpersMode EditorHelpersMode;

	bool bGridMode;
	SEntity* SelectedEntity;
	ESelectedArrowType SelectedArrow;
	ESelectedDimHelperType SelectedDimHelper;
	bool bCircleSelected;
	float MoveDeltaAccum;

	SPointLight* SelectedPointLight;

	SLevel NewLevel; // NOTE(georgii): Memory for new level creation

	// NOTE(georgii): These are for Ctrl+Z stuff
	uint32_t LevelHistoryHead;
	uint32_t LevelHistoryTail;
	SLevel LevelHistory[64]; 

	bool bImGuiChangeStarted;
	float ImGuiChangeTimePassed;
};

#endif