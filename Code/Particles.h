#pragma once

struct SParticle
{
	vec3 Pos;
	float Scale;
	vec3 Velocity;
	vec3 Color;

	float LifeTime;
};

struct SParticleEmitter
{
	bool bLoop;
	bool bVelocityFromOffset;
	bool bGravity;

	vec3 Pos;
	vec3 PosOffsetMin, PosOffsetMax;
	float ScaleMin, ScaleMax;
	vec3 VelocityMin, VelocityMax;
	float VelocityMultMin, VelocityMultMax; // NOTE(georgii): Used when bVelocityFromOffset = true
	vec3 ColorMin, ColorMax;
	float LifeTimeMin, LifeTimeMax;
	
	union
	{
		float ParticlesInSecond;
		uint32_t BurstParticles;
	};
	float SpawnRemainder;

	uint32_t ParticleCount;
	SParticle Particles[256];
};