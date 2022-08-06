#include "Audio.h"

SPlayingSound* PlaySound(SAudioState* AudioState, bool bLoop, uint32_t SoundID, bool bExclusive = true)
{
	Assert(AudioState->PlayingSoundCount < ArrayCount(AudioState->PlayingSounds));

	if (AudioState->PlayingSoundCount < ArrayCount(AudioState->PlayingSounds))
	{
		if (bExclusive)
		{
			for (uint32_t I = 0; I < AudioState->PlayingSoundCount; I++)
			{
				if (AudioState->PlayingSounds[I].SoundID == SoundID)
				{
					return 0;
				}
			}
		}

		SPlayingSound PlayingSound = {};
		PlayingSound.bLoop = bLoop;
		PlayingSound.SoundID = SoundID;

		PlayingSound.CurrentVolume = PlayingSound.TargetVolume = Vec2(1.0f, 1.0f);
		PlayingSound.Pitch = 1.0f;

		AudioState->PlayingSounds[AudioState->PlayingSoundCount++] = PlayingSound;
	
		return &AudioState->PlayingSounds[AudioState->PlayingSoundCount - 1];
	}
	else
	{
		return 0;
	}
}

void ChangeVolume(SPlayingSound* Sound, float Seconds, vec2 Volume)
{
    if (Sound)
    {
        if (Seconds <= 0.0f)
        {
            Sound->CurrentVolume = Sound->TargetVolume = Volume;
        }
        else
        {
            Sound->TargetVolume = Volume;
            Sound->DeltaVolume = (Volume - Sound->CurrentVolume) * (1.0f / Seconds);
        }
    }
}

void ChangePitch(SPlayingSound* Sound, float Pitch)
{
	if (Sound)
	{
		Sound->Pitch = Pitch;
	}
}

void OutputPlayingSounds(SAudioState* AudioState, const SGameSoundBuffer& SoundBuffer, const SLoadedWAV* LoadedSounds, STempMemoryArena* MemoryArena)
{
	BEGIN_PROFILER_BLOCK("SOUND_MIXING");

	if (SoundBuffer.SampleCount > 0)
	{
		float* SamplesFloat = (float*) PushMemory(MemoryArena->Arena, SoundBuffer.SampleCount * (SoundBuffer.ChannelCount * sizeof(float)));
		memset(SamplesFloat, 0, SoundBuffer.SampleCount * (SoundBuffer.ChannelCount * sizeof(float)));

		uint32_t SamplesPerSec = SoundBuffer.SamplesPerSec;
		float SecsPerSample = 1.0f / SamplesPerSec;

		for (uint32_t I = 0; I < AudioState->PlayingSoundCount; I++)
		{
			SPlayingSound* PlayingSound = &AudioState->PlayingSounds[I];
			const SLoadedWAV* Sound = &LoadedSounds[PlayingSound->SoundID];
			int16_t* SoundSamples = Sound->Samples;

			uint32_t TotalSamplesToMix = SoundBuffer.SampleCount;
			uint32_t SamplesMixed = 0; 
			while (!PlayingSound->bFinished && (TotalSamplesToMix > 0))
			{
				uint32_t SamplesRemainingInSound = CeilToUInt32((Sound->SampleCount - PlayingSound->SamplesPlayed) / PlayingSound->Pitch);
				uint32_t SamplesToMix = TotalSamplesToMix > SamplesRemainingInSound ? SamplesRemainingInSound : TotalSamplesToMix;

				vec2 DeltaVolumePerSample = PlayingSound->DeltaVolume * SecsPerSample;

				uint32_t VolumeEndSamples[2] = {};
				for (uint32_t J = 0; J < ArrayCount(VolumeEndSamples); J++)
				{
					if (DeltaVolumePerSample.E[J] != 0.0f)
					{
						float DeltaVolume = PlayingSound->TargetVolume.E[J] - PlayingSound->CurrentVolume.E[J];
						uint32_t VolumeSampleCount = (uint32_t) ((DeltaVolume / DeltaVolumePerSample.E[J]) + 0.5f);
						if (SamplesToMix > VolumeSampleCount)
						{
							SamplesToMix = VolumeSampleCount;
							VolumeEndSamples[J] = VolumeSampleCount;
						}
					}
				}

				const float Step = PlayingSound->Pitch;
				for (uint32_t J = 0; J < SamplesToMix; J++)
				{
					vec2 Volume = Hadamard(AudioState->MasterVolume, PlayingSound->CurrentVolume);

					float Position = J * Step;
					uint32_t LeftSampleIndex = FloorToUInt32(Position);
					uint32_t RightSampleIndex = CeilToUInt32(Position);
					float Frac = Position - float(LeftSampleIndex);

					float LeftSample0 = SoundSamples[2 * PlayingSound->SamplesPlayed + 2 * LeftSampleIndex];
					float RightSample0 = SoundSamples[2 * PlayingSound->SamplesPlayed + 2 * RightSampleIndex];
					float Sample0 = Lerp(LeftSample0, RightSample0, Frac);

					float LeftSample1 = SoundSamples[2 * PlayingSound->SamplesPlayed + 2 * LeftSampleIndex + 1];
					float RightSample1 = SoundSamples[2 * PlayingSound->SamplesPlayed + 2 * RightSampleIndex + 1];
					float Sample1 = Lerp(LeftSample1, RightSample1, Frac);

					SamplesFloat[2 * (J + SamplesMixed)] += Volume.x * Sample0;
					SamplesFloat[2 * (J + SamplesMixed) + 1] += Volume.y * Sample1;

					PlayingSound->CurrentVolume += DeltaVolumePerSample;
				}

				for (uint32_t J = 0; J < ArrayCount(VolumeEndSamples); J++)
				{
					if (SamplesToMix == VolumeEndSamples[J])
					{
						PlayingSound->CurrentVolume.E[J] = PlayingSound->TargetVolume.E[J];
						PlayingSound->DeltaVolume.E[J] = 0.0f;
					}
				}

				PlayingSound->SamplesPlayed += CeilToUInt32(PlayingSound->Pitch * SamplesToMix);
				if (PlayingSound->SamplesPlayed >= Sound->SampleCount)
				{
					if (PlayingSound->bLoop)
					{
						PlayingSound->SamplesPlayed = 0;
					}
					else
					{
						PlayingSound->bFinished = true;
					}
				}

				SamplesMixed += SamplesToMix;
				TotalSamplesToMix -= SamplesToMix;
			}

			if (PlayingSound->bFinished)
			{
				AudioState->PlayingSounds[I] = AudioState->PlayingSounds[--AudioState->PlayingSoundCount];
			}
		}

		for (uint32_t I = 0; I < SoundBuffer.SampleCount; I++)
		{
			SoundBuffer.Samples[2 * I] = int16_t(Clamp(SamplesFloat[2 * I], float(INT16_MIN), float(INT16_MAX)));
			SoundBuffer.Samples[2 * I + 1] = int16_t(Clamp(SamplesFloat[2 * I + 1], float(INT16_MIN), float(INT16_MAX)));
		}
	}

	END_PROFILER_BLOCK("SOUND_MIXING");
}