#pragma once

struct SPlayingSound
{
	bool bLoop;
	bool bFinished;
	bool bMusic;
	
	bool bThreeD;
	vec3 Pos;

    vec2 CurrentVolume;
	vec2 DeltaVolume;
    vec2 TargetVolume;

	float Pitch;

	uint32_t SamplesPlayed;

	uint32_t SoundID;
};

struct SAudioState
{
	uint32_t MasterVolume;
	uint32_t MusicVolume;
	uint32_t EffectsVolume;

	uint32_t PlayingSoundCount;
	SPlayingSound PlayingSounds[128];
};