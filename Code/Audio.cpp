#include "Audio.h"

SPlayingSound* PlaySound(SAudioState* AudioState, bool bLoop, uint32_t SoundID, bool bExclusive = true, bool bThreeD = false, vec3 Pos = Vec3(0.0f))
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
		PlayingSound.bMusic = false;
		PlayingSound.SoundID = SoundID;
		PlayingSound.bThreeD = bThreeD;
		PlayingSound.Pos = Pos;

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

SPlayingSound* PlayMusic(SAudioState* AudioState, bool bLoop, uint32_t SoundID, bool bExclusive = true)
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
		PlayingSound.bMusic = true;
		PlayingSound.SoundID = SoundID;
		PlayingSound.bThreeD = false;
		PlayingSound.Pos = Vec3(0.0f);

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

void OutputPlayingSounds(SAudioState* AudioState, const SGameSoundBuffer& SoundBuffer, const SLoadedWAV* LoadedSounds, STempMemoryArena* MemoryArena, const vec3 ListenerPos, const SLevel* Level)
{
	BEGIN_PROFILER_BLOCK("SOUND_MIXING");

	if (SoundBuffer.SampleCount > 0)
	{
		float* SamplesFloat = (float*) PushMemory(MemoryArena->Arena, SoundBuffer.SampleCount * (SoundBuffer.ChannelCount * sizeof(float)));
		memset(SamplesFloat, 0, SoundBuffer.SampleCount * (SoundBuffer.ChannelCount * sizeof(float)));

		uint32_t SamplesPerSec = SoundBuffer.SamplesPerSec;
		float SecsPerSample = 1.0f / SamplesPerSec;

		for (uint32_t I = 0; I < AudioState->PlayingSoundCount;)
		{
			SPlayingSound* PlayingSound = &AudioState->PlayingSounds[I];
			const SLoadedWAV* Sound = &LoadedSounds[PlayingSound->SoundID];
			int16_t* SoundSamples = Sound->Samples;

			vec2 SoundVolumeMultiplier = Vec2(1.0f);
			const float MusicCoeff = 0.5f;
			const float EffectsCoeff = 0.25f;
			if (PlayingSound->bMusic)
			{
				vec2 MusicVolume = MusicCoeff * Vec2(AudioState->MusicVolume / 100.0f);
				SoundVolumeMultiplier = Hadamard(SoundVolumeMultiplier, MusicVolume);
			}
			else
			{
				vec2 EffectsVolume = EffectsCoeff * Vec2(AudioState->EffectsVolume / 100.0f);
				SoundVolumeMultiplier = Hadamard(SoundVolumeMultiplier, EffectsVolume);
			}

			if (PlayingSound->bThreeD)
			{
				const float MaxSoundDistance = 30.0f;

				vec3 DistanceVec = ListenerPos - PlayingSound->Pos;
				float DistanceSquared = Dot(DistanceVec, DistanceVec);

				if (DistanceSquared > Square(MaxSoundDistance))
				{
					// TODO(georgii): Think about this!
					SoundVolumeMultiplier = Vec2(0.0f);
				}
				else
				{
					float Distance = SquareRoot(DistanceSquared);

					float HitCoefficient = 1.0f;
					SRaytraceVoxelsResult RaytraceVoxelsResult = RaytraceVoxels(Level, PlayingSound->Pos, Normalize(DistanceVec), Distance);
					if (RaytraceVoxelsResult.bHit)
					{
						// TODO(georgii): Think about this!
						// HitCoefifcient *= RaytraceVoxelsResult.Distance / Distance;
					}

					if (DistanceSquared > FloatEpsilon)
					{
						SoundVolumeMultiplier *= HitCoefficient * Min(Square(9.0f) / DistanceSquared, 1.0f);
					}
				}
			}

			uint32_t TotalSamplesToMix = SoundBuffer.SampleCount;
			uint32_t SamplesMixed = 0; 
			while (!PlayingSound->bFinished && (TotalSamplesToMix > 0))
			{
				// NOTE(georgii): This formula is super strange, but it seems like it works.
				//				  Actually, if I'm not interested to be super precise I can use commented formulas,
				// 				  and when sampling this sound's buffer check for sampling outside the array.
				float FloatSamplesRemainingInSound = ((Sound->SampleCount - 1) - (PlayingSound->SamplePos - PlayingSound->Pitch)) / PlayingSound->Pitch;
				uint32_t SamplesRemainingInSound = FloorToUInt32(FloatSamplesRemainingInSound);
				// float FloatSamplesRemainingInSound = (Sound->SampleCount - roundf(PlayingSound->SamplePos)) / PlayingSound->Pitch;
				// uint32_t SamplesRemainingInSound = uint32_t(roundf(FloatSamplesRemainingInSound));
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

				float SamplePos = PlayingSound->SamplePos;
				for (uint32_t J = 0; J < SamplesToMix; J++)
				{
					vec2 Volume = Hadamard(PlayingSound->CurrentVolume, SoundVolumeMultiplier);

					uint32_t LeftSampleIndex = FloorToUInt32(SamplePos);
					uint32_t RightSampleIndex = CeilToUInt32(SamplePos);

					if ((LeftSampleIndex < Sound->SampleCount) && (RightSampleIndex < Sound->SampleCount))
					{
						float Frac = SamplePos - float(LeftSampleIndex);

						float LeftSample0 = SoundSamples[2 * LeftSampleIndex];
						float RightSample0 = SoundSamples[2 * RightSampleIndex];
						float Sample0 = Lerp(LeftSample0, RightSample0, Frac);

						float LeftSample1 = SoundSamples[2 * LeftSampleIndex + 1];
						float RightSample1 = SoundSamples[2 * RightSampleIndex + 1];
						float Sample1 = Lerp(LeftSample1, RightSample1, Frac);

						SamplesFloat[2 * (J + SamplesMixed)] += Volume.x * Sample0;
						SamplesFloat[2 * (J + SamplesMixed) + 1] += Volume.y * Sample1;

						PlayingSound->CurrentVolume += DeltaVolumePerSample;
					}
					
					SamplePos += PlayingSound->Pitch;
				}

				for (uint32_t J = 0; J < ArrayCount(VolumeEndSamples); J++)
				{
					if (SamplesToMix == VolumeEndSamples[J])
					{
						PlayingSound->CurrentVolume.E[J] = PlayingSound->TargetVolume.E[J];
						PlayingSound->DeltaVolume.E[J] = 0.0f;
					}
				}

				SamplesMixed += SamplesToMix;
				TotalSamplesToMix -= SamplesToMix;

				PlayingSound->SamplePos = SamplePos;
				if (PlayingSound->SamplePos >= (Sound->SampleCount - 1))
				{
					if (PlayingSound->bLoop)
					{
						PlayingSound->SamplePos = 0;
					}
					else
					{
						PlayingSound->bFinished = true;
					}
				}
			}

			if (PlayingSound->bFinished)
			{
				AudioState->PlayingSounds[I] = AudioState->PlayingSounds[--AudioState->PlayingSoundCount];
			}
			else
			{
				I++;
			}
		}

		const vec2 MasterVolume = Vec2(AudioState->MasterVolume / 100.0f);
		for (uint32_t I = 0; I < SoundBuffer.SampleCount; I++)
		{
			SamplesFloat[2 * I] *= MasterVolume.x;
			SamplesFloat[2 * I + 1] *= MasterVolume.y;

			Assert(SamplesFloat[2 * I] >= float(INT16_MIN));
			Assert(SamplesFloat[2 * I] <= float(INT16_MAX));
			Assert(SamplesFloat[2 * I + 1] >= float(INT16_MIN));
			Assert(SamplesFloat[2 * I + 1] <= float(INT16_MAX));

			SoundBuffer.Samples[2 * I] = int16_t(Clamp(SamplesFloat[2 * I], float(INT16_MIN), float(INT16_MAX)));
			SoundBuffer.Samples[2 * I + 1] = int16_t(Clamp(SamplesFloat[2 * I + 1], float(INT16_MIN), float(INT16_MAX)));
		}
	}

	END_PROFILER_BLOCK("SOUND_MIXING");
}