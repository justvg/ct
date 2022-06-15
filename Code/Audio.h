#pragma once

struct SPlayingSound
{
	bool bLoop;
	bool bFinished;

    vec2 CurrentVolume;
	vec2 DeltaVolume;
    vec2 TargetVolume;

	float Pitch;

	uint32_t SamplesPlayed;

	uint32_t SoundID;
};

struct SAudioState
{
	vec2 MasterVolume;

	uint32_t PlayingSoundCount;
	SPlayingSound PlayingSounds[64];
};