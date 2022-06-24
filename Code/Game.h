#pragma once

enum EMenuElement
{
	MenuElement_DefaultNone,

	MenuElement_Settings,
	MenuElement_StartNewGame,
	MenuElement_Quit,

	MenuElement_DefaultCount,


	MenuElement_SettingsNone,

	MenuElement_Fullscreen,
	MenuElement_VSync,
	MenuElement_Resolution,
	MenuElement_Vignetting,
	MenuElement_Multisampling,

	MenuElement_SettingsCount,
};

enum EMenuMode
{
	MenuMode_Default,
	MenuMode_Settings
};

struct SGameState
{
	bool bInitialized;
	bool bReload;

	vec3 LastBaseLevelPos;
	float LastBaseLevelGatesAngle;

	bool bDeathAnimation;
    float DeathAnimationSpeed;
    float DeathAnimationLengthMoved;
    vec3 DeathPos;
	vec3 DeathAnimationTargetPos;

	// NOTE(georgii): Don't wanna resave every level when I change these params for hero, so I just store them in SEngineState.
	float HeroSpeed;
	float HeroDrag;
	float HeroJumpPower;
	float HeroLampDistance;

	EMenuMode MenuMode;
	EMenuElement SelectedMenuElement;

    uint8_t CurrentCheckpointIndex;
	vec3 LastCheckpointPos;
};