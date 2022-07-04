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
	MenuElement_Vignetting,
	MenuElement_AOQuality,
	MenuElement_Resolution,

	MenuElement_SettingsCount,


	MenuElement_StartNewGameNone,

	MenuElement_StartNewGameYes,
	MenuElement_StartNewGameNo,

	MenuElement_StartNewGameCount
};

enum EMenuMode
{
	MenuMode_Default,
	MenuMode_Settings,
	MenuMode_StartNewGame
};

struct SMenuState
{
	EMenuMode MenuMode;
	EMenuElement SelectedMenuElement;

	vec2 MousePos;
	vec2 LastMousePos;

	vec2 ScreenDim;

	bool bJustOpened;

	bool bMousePosChanged;
	bool bEnterDown;
	bool bMouseLeftReleased;

	bool bArrowLeft;
	bool bArrowRight;
	bool bArrowUsed;

	bool bDisableStarted;

	float OpenTime;
	float OpenAnimationTime;

	float SelectedTime;
	float SelectedStayBrightTime;
	float SelectedAnimationTime;
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

	SMenuState MenuState;

    uint8_t CurrentCheckpointIndex;
	vec3 LastCheckpointPos;
};