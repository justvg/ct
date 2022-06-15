#pragma once

// NOTE(georgii): New types should be added to the end of this enum!
enum EEntityType
{
	Entity_None,
	
	Entity_Hero,
	Entity_Monster,
	Entity_Torch,
	Entity_Door,
	Entity_Gates,
	Entity_Turret,
	Entity_Fireball,
	Entity_MessageToggler,
	Entity_Container,

	Entity_Checkpoint,

	Entity_ColorField,
};

struct SPointLight
{
	vec3 Pos;
	float Radius;
	vec4 Color;
};

struct SEntity
{
	EEntityType Type;

	vec3 Pos;
	vec3 Velocity;
	vec3 Dim;

	vec4 Orientation; // NOTE(georgii): Euler angles. W is unused for now.

	vec3 LampBaseOffset;
	vec3 LampOffset;
	vec2 LampRotationOffset; // NOTE(georgii): x - pitch, y - head

	union
	{
		vec3 PrevLampOffset;
		vec3 FireballDim;
	};

	float Scale;

	float Speed;
	float Drag;
	float JumpPower;

	uint32_t DoorIndex; // NOTE(georgii): Used for checkpoint to close a door.
	
	float Alpha;

	vec3 Color;

	vec3 PrevPos;
	vec4 PrevOrientation; // NOTE(georgii): Euler angles. W is unused for now.

	SPointLight PointLight;

	// Gameplay colors
	vec3 CurrentColor;
	union
	{
		vec3 TargetColor;
		vec3 FireballColor;
	};

	union
	{
		struct
		{
			float TimeToMove;
			float TimeToMoveCurrent;
		};

		struct
		{
			float TimeToShoot;
			float TimeToShootCurrent;
		};

		// NOTE(georgii): These are used to animate color changing for hero.
		struct 
		{
			float TimeToChangeColor;
			float TimeToChangeColorCurrent;
		};
	};
	vec3 BasePos;
	vec3 TargetOffset;

	// NOTE(georgii): Currently this is used for hero/gates collision, so we don't immediately handle it, but after some time.
	bool bCollisionWithHeroStarted; 
	float CollisionWithHeroTimePassed;

	uint32_t MeshIndex;

	union
	{
		char TargetLevelName[32];
		char MessageText[64];
	};

	union
	{
		bool bGoBack; // NOTE(georgii): Used for doors to go back to base position when current color != target color.
		bool bUndiffusable; // NOTE(georgii): Used for fireballs that can't be diffused.
		bool bChangeColorAnimation; // NOTE(georgii): Used to check if we should animate color changing for hero.
	};

    union
    {
	    bool bForceClose; // NOTE(georgii): Used to force door to close. Currently only checkpoints do that.
        uint8_t CheckpointIndex; // NOTE(georgii): Used for checkpoint to easily teleport between them during development.
    };

	bool bRemoved;

	float DistanceToCam;
};