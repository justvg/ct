#include "Particles.h"

void AddParticleEmitter(SParticleEmitter* Emitters, uint32_t& EmitterCount, bool bVelocityFromOffset, bool bGravity, 
						vec3 Pos, vec3 PosOffsetMin, vec3 PosOffsetMax, float ScaleMin, float ScaleMax, 
						vec3 VelocityMin, vec3 VelocityMax, vec3 ColorMin, vec3 ColorMax, 
						float LifeTimeMin, float LifeTimeMax, float ParticlesInSecond, 
						float VelocityMultMin = 1.0f, float VelocityMultMax = 1.0f)
{
	SParticleEmitter Emitter = {};

	Emitter.bLoop = true;
	Emitter.bVelocityFromOffset = bVelocityFromOffset;
	Emitter.bGravity = bGravity;

	Emitter.Pos = Pos;
	Emitter.PosOffsetMin = PosOffsetMin;
	Emitter.PosOffsetMax = PosOffsetMax;
	Emitter.ScaleMin = ScaleMin;
	Emitter.ScaleMax = ScaleMax;
	Emitter.VelocityMin = VelocityMin;
	Emitter.VelocityMax = VelocityMax;

	if (bVelocityFromOffset)
	{
		Emitter.VelocityMultMin = VelocityMultMin;
		Emitter.VelocityMultMax = VelocityMultMax;
	}

	Emitter.ColorMin = ColorMin;
	Emitter.ColorMax = ColorMax;
	Emitter.LifeTimeMin = LifeTimeMin;
	Emitter.LifeTimeMax = LifeTimeMax;
	Emitter.ParticlesInSecond = ParticlesInSecond;

	Emitters[EmitterCount++] = Emitter;
}

void AddParticleEmitterBurst(SParticleEmitter* Emitters, uint32_t& EmitterCount, bool bVelocityFromOffset, bool bGravity, 
							 vec3 Pos, vec3 PosOffsetMin, vec3 PosOffsetMax, float ScaleMin, float ScaleMax, 
							 vec3 VelocityMin, vec3 VelocityMax, vec3 ColorMin, vec3 ColorMax, 
							 float LifeTimeMin, float LifeTimeMax, uint32_t BurstParticles, 
							 float VelocityMultMin = 1.0f, float VelocityMultMax = 1.0f)
{
	SParticleEmitter Emitter = {};

	Emitter.bLoop = false;
	Emitter.bVelocityFromOffset = bVelocityFromOffset;
	Emitter.bGravity = bGravity;

	Emitter.Pos = Pos;
	Emitter.PosOffsetMin = PosOffsetMin;
	Emitter.PosOffsetMax = PosOffsetMax;
	Emitter.ScaleMin = ScaleMin;
	Emitter.ScaleMax = ScaleMax;
	Emitter.VelocityMin = VelocityMin;
	Emitter.VelocityMax = VelocityMax;

	if (bVelocityFromOffset)
	{
		Emitter.VelocityMultMin = VelocityMultMin;
		Emitter.VelocityMultMax = VelocityMultMax;
	}

	Emitter.ColorMin = ColorMin;
	Emitter.ColorMax = ColorMax;
	Emitter.LifeTimeMin = LifeTimeMin;
	Emitter.LifeTimeMax = LifeTimeMax;
	Emitter.BurstParticles = BurstParticles;

	Emitters[EmitterCount++] = Emitter;
}

void SpawnParticle(SParticleEmitter* Emitter)
{
	Assert(Emitter->ParticleCount < ArrayCount(Emitter->Particles));
	
	SParticle Particle = {};
	vec3 Offset = RandomVec3(Emitter->PosOffsetMin, Emitter->PosOffsetMax);
	Particle.Pos = Emitter->Pos + Offset;
	Particle.Scale = RandomFloat(Emitter->ScaleMin, Emitter->ScaleMax);
	if (Emitter->bVelocityFromOffset)
	{
		Particle.Velocity = RandomFloat(Emitter->VelocityMultMin, Emitter->VelocityMultMax) * Normalize(Offset);
	}
	else
	{
		Particle.Velocity = RandomVec3(Emitter->VelocityMin, Emitter->VelocityMax);
	}
	Particle.Color = RandomVec3(Emitter->ColorMin, Emitter->ColorMax);
	Particle.LifeTime = RandomFloat(Emitter->LifeTimeMin, Emitter->LifeTimeMax);

	Emitter->Particles[Emitter->ParticleCount++] = Particle;
}

uint32_t SimulateParticles(SParticleEmitter* Emitters, uint32_t& EmitterCount, float dt)
{
	BEGIN_PROFILER_BLOCK("PARTICLES_SIMULATION");

	uint32_t TotalParticleCount = 0;
	for (uint32_t I = 0; I < EmitterCount;)
	{
		SParticleEmitter* Emitter = &Emitters[I];

		// Simulate
		for (uint32_t J = 0; J < Emitter->ParticleCount;)
		{
			SParticle* Particle = &Emitter->Particles[J];

			Particle->LifeTime -= dt;
			if (Particle->LifeTime <= 0.0f)
			{
				*Particle = Emitter->Particles[--Emitter->ParticleCount];
			}
			else
			{
				vec3 Acceleration = Vec3(0.0f);
				if (Emitter->bGravity)
				{
					Acceleration = Vec3(0.0f, -9.8f, 0.0f);
				}
				Particle->Velocity += Acceleration * dt;
				Particle->Pos += Particle->Velocity * dt + 0.5f * Acceleration * Square(dt);

				J++;
			}
		}

		// Spawn
		if (Emitter->bLoop)
		{
			uint32_t SpawnCount = uint32_t(Emitter->ParticlesInSecond * dt);
			Emitter->SpawnRemainder += (Emitter->ParticlesInSecond * dt) - SpawnCount;
			if (Emitter->SpawnRemainder >= 1.0f)
			{
				SpawnCount++;
				Emitter->SpawnRemainder -= 1.0f;
			}

			for (uint32_t J = 0; J < SpawnCount; J++)
			{
				SpawnParticle(Emitter);
			}

			I++;
		}
		else
		{
			if (Emitter->BurstParticles > 0)
			{
				for (uint32_t J = 0; J < Emitter->BurstParticles; J++)
				{
					SpawnParticle(Emitter);
				}

				Emitter->BurstParticles = 0;
			}
			else if (Emitter->ParticleCount == 0)
			{
				*Emitter = Emitters[--EmitterCount];
				continue;
			}

			if (Emitter->ParticleCount != 0)
				I++;
		}

		TotalParticleCount += Emitter->ParticleCount;
	}
	
	END_PROFILER_BLOCK("PARTICLES_SIMULATION");

	return TotalParticleCount;
}