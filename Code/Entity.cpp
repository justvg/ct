#include "Entity.h"

SEntity* AddHero(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount == 0);
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Hero;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(0.7f, 1.5f, 0.7f);
	Entity->Scale = 0.25f;

	Entity->Alpha = 1.0f;

	Entity->Speed = 10.0f;
	Entity->Drag = 2.0f;
	Entity->JumpPower = 7.0f;

	Entity->LampBaseOffset = Vec3(0.7f, 0.6f, 0.65f);
	Entity->LampOffset = Entity->LampBaseOffset;
	Entity->LampRotationOffset = Vec2(10.0f, 10.0f);

	Entity->MeshIndex = 3;

	return Entity;
}

SEntity* AddTorch(SLevel& Level, vec3 Pos, vec3 Dim, vec3 Color)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Torch;
	Entity->Pos = Pos;
	Entity->Dim = Dim;
	Entity->Scale = 1.0f;

	Entity->Alpha = 1.0f;

	Entity->Color = Color;
	Entity->MeshIndex = 1;

	Entity->PointLight.Pos = Vec3(0.0f, 0.0f, 0.0f);
	Entity->PointLight.Radius = 1.0f;
	Entity->PointLight.Color = Vec4(Entity->Color, 0.0f);

	return Entity;	
}

SEntity* AddContainer(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Container;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(0.8f, 0.25f, 0.4f);
	Entity->Scale = 1.0f;

	Entity->Alpha = 1.0f;

	Entity->MeshIndex = 1;

	Entity->PointLight.Pos = Vec3(0.0f, 0.0f, 0.0f);
	Entity->PointLight.Radius = 1.0f;
	Entity->PointLight.Color = Vec4(Entity->Color, 0.0f);

	return Entity;	
}

SEntity* AddDoor(SLevel& Level, vec3 Pos, vec3 Dim, quat Orientation, vec3 Color, vec3 TargetColor, vec3 TargetOffset, float TimeToDisappear = 1.0f)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Door;
	Entity->Pos = Pos;
	Entity->Dim = Dim;
	Entity->Scale = 1.0f;

	Entity->Alpha = 0.99999f;

	Entity->TimeToDisappear = TimeToDisappear;

	Entity->Color = Color;
	Entity->MeshIndex = 2;

	return Entity;
}

SEntity* AddGates(SLevel& Level, vec3 Pos, vec3 Dim, quat Orientation)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Gates;
	Entity->Pos = Pos;
	Entity->Dim = Dim;
	Entity->Scale = 1.0f;
	
	Entity->Alpha = 0.8f;

	Entity->Color = Vec3(0.25f, 0.45f, 1.0f);
	Entity->MeshIndex = 2;

	return Entity;
}

SEntity* AddTurret(SLevel& Level, vec3 Pos, vec3 Dim, quat Orientation, vec3 Color, vec3 ShootTarget, float TimeToShoot)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Turret;
	Entity->Pos = Pos;
	Entity->Dim = Dim;
	Entity->Scale = 1.0f;

	Entity->Alpha = 1.0f;

	Entity->FireballDim = Vec3(0.35f, 0.35f, 0.35f);

	Entity->Speed = 3.0f;

	Entity->TargetOffset = ShootTarget;
	Entity->TimeToShoot = TimeToShoot;

	Entity->Color = Color;
	Entity->FireballColor = Vec3(1.0f, 0.0f, 0.0f);
	Entity->MeshIndex = 1;

	return Entity;
}

SEntity* AddFireball(SLevel& Level, vec3 Pos, vec3 Dim, vec3 Orientation, vec3 Color, vec3 TargetOffset, float Speed, bool bUndiffusable)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Fireball;
	Entity->Pos = Pos;
	Entity->Dim = Dim;
	Entity->Orientation = Vec4(Orientation, 0.0f);
	Entity->Scale = 1.0f;

	Entity->Alpha = 1.0f;

	Entity->Speed = 1.0f;
	Entity->BasePos = Pos;
	Entity->TargetOffset = TargetOffset;
	Entity->Speed = Speed;

	Entity->Color = Color;
	Entity->MeshIndex = 3;

	Entity->bUndiffusable = bUndiffusable;

	return Entity;
}

SEntity* AddMessageToggler(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_MessageToggler;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(1.0f, 1.0f, 1.0f);
	Entity->Scale = 1.0f;

	Entity->MessagePos = Vec2(0.0f, 0.5f);
	Entity->MessageScale = 0.5f;
	Entity->MessageLifeTime = 5.0f;
	Entity->MessageTimeToAppear = 3.0f;
	Entity->MessageTimeToStartAppear = 0.0f;

	Entity->Alpha = 1.0f;

	Entity->MeshIndex = 1;

	return Entity;
}

SEntity* AddCheckpoint(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Checkpoint;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(1.0f, 1.0f, 1.0f);
	Entity->Scale = 1.0f;

	Entity->Alpha = 1.0f;

	Entity->MeshIndex = 1;

    uint8_t CheckpointsMaxIndex = 0;
    for (uint32_t I = 0; I < Level.EntityCount - 1; I++)
    {
        SEntity* CheckEntity = Level.Entities + I;

        if ((CheckEntity->Type == Entity_Checkpoint) && (CheckEntity->CheckpointIndex > CheckpointsMaxIndex))
        {
            CheckpointsMaxIndex = CheckEntity->CheckpointIndex;
        }
    }

    Entity->CheckpointIndex = CheckpointsMaxIndex + 1;

	return Entity;
}

SEntity* AddColorField(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_ColorField;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(2.0f, 0.5f, 2.0f);
	Entity->Scale = 1.0f;

	Entity->Color = Vec3(0.0f, 0.25f, 1.0f);
	Entity->Alpha = 0.75f;

	Entity->MeshIndex = 1;

	return Entity;
}

SEntity* AddWire(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Wire;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(0.05f, 0.03f, 0.5f);
	Entity->Scale = 1.0f;

	Entity->Color = Vec3(1.0f, 0.0f, 0.0f);
	Entity->Alpha = 1.0f;

	Entity->MeshIndex = 1;

	return Entity;
}

SEntity* AddCube(SLevel& Level, vec3 Pos)
{
	Assert(Level.EntityCount < ArrayCount(Level.Entities));
	SEntity* Entity = &Level.Entities[Level.EntityCount++];
	memset(Entity, 0, sizeof(SEntity));

	Entity->Type = Entity_Cube;
	Entity->Pos = Pos;
	Entity->Dim = Vec3(0.1f, 0.06f, 1.0f);
	Entity->Scale = 1.0f;

	Entity->Color = Vec3(0.0f, 0.0f, 0.0f);
	Entity->Alpha = 1.0f;

	Entity->MeshIndex = 1;

	return Entity;
}

SEntity* AddEntityCopy(SLevel* Level, const SEntity* EntityToCopy)
{
	Assert(Level->EntityCount < ArrayCount(Level->Entities));
	SEntity* Entity = &Level->Entities[Level->EntityCount++];
	*Entity = *EntityToCopy;

	return Entity;
}

bool HasTargetPos(const SEntity* Entity)
{
	bool bResult = (Entity->Type == Entity_Turret);

	return bResult;
}

bool CanCollide(const SEntity* A, const SEntity* B)
{
	bool bResult = true;

	// Add tests here

	return bResult;
}

bool BlockOnCollision(const SEntity* A, const SEntity* B)
{
	bool bResult = true;

	if (A->Type > B->Type)
	{
		const SEntity* Temp = A;
		A = B;
		B = Temp;
	}

	if ((A->Type == Entity_Fireball) || (B->Type == Entity_Fireball))
	{
		bResult = false;

		if (A->Type == Entity_Door)
		{
			bResult = A->Alpha > 0.05f;
		}
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_Door))
	{
		bResult = B->Alpha > 0.05f;
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_Gates))
	{
		bResult = false;
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_MessageToggler))
	{
		bResult = false;
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_Checkpoint))
	{
		bResult = false;
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_ColorField))
	{
		bResult = false;
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_Wire))
	{
		bResult = false;
	}

	return bResult;
}

void HandleCollision(SGameState* GameState, SEngineState* EngineState, SLevel* Level, SEntity* A, SEntity* B)
{
	if (A->Type > B->Type)
	{
		SEntity* Temp = A;
		A = B;
		B = Temp;
	}

	if ((A->Type == Entity_Hero) && (B->Type == Entity_Gates))
	{
		if (!B->bClosedGates && !B->bCollisionWithHeroStarted)
		{
			B->bCollisionWithHeroStarted = true;
			B->CollisionWithHeroTimePassed = 0.0f;

			if (CompareStrings(EngineState->LevelName, "Levels\\MainHub.ctl"))
			{
				GameState->LastBaseLevelPos = A->PrevPos;
				GameState->LastBaseLevelGatesAngle = B->Orientation.y;
			}
		}
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_Fireball))
	{
		if ((Length(B->Color) > 0.0f) || B->bUndiffusable)
		{
			ReloadGameLevel(EngineState, false, false);

			StartDeathAnimation(GameState, A->Pos);
		}
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_MessageToggler))
	{
		B->bRemoved = true;

		AddText(EngineState, B->MessageText, B->MessagePos, B->MessageScale, Vec4(1.0f), B->MessageLifeTime, true, B->MessageTimeToAppear, B->MessageTimeToStartAppear);
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_Checkpoint))
	{
		A->bChangeColorAnimation = true;
		A->AnimationColor = Vec3(0.0f);
		A->TimeToChangeColor = 1.0f;
		A->TimeToChangeColorCurrent = 0.0f;

		GameState->LastCheckpointPos = B->Pos;

		GameState->PosForDeathAnimationCount = 1;
		GameState->PosForDeathAnimation[0] = GameState->LastCheckpointPos;

		B->bRemoved = true;

		if ((B->DoorIndex > 0) && (B->DoorIndex < Level->EntityCount))
		{
			uint32_t DoorIndex = B->DoorIndex;
			Assert(Level->Entities[DoorIndex].Type == Entity_Door);

			Level->Entities[DoorIndex].bForceClose = true;
		}
	}
	else if ((A->Type == Entity_Hero) && (B->Type == Entity_ColorField))
	{
		if (!A->bChangeColorAnimation)
		{
			A->bChangeColorAnimation = true;
			A->AnimationColor = Vec3(0.0f);
			A->TimeToChangeColor = 0.5f;
			A->TimeToChangeColorCurrent = 0.0f;
		}
	}
	else if ((A->Type == Entity_Fireball && B->Type != Entity_Fireball) || (A->Type != Entity_Fireball && B->Type == Entity_Fireball))
	{
		if ((A->Type != Entity_Turret) && (A->Type != Entity_Door || A->Alpha > 0.05f))
		{
			if (A->Type == Entity_Fireball)
			{
				A->bRemoved = true;
			}
			else
			{
				B->bRemoved = true;
			}
		}
	}
}

struct SMoveEntityInfo
{
	bool bGravity;
	bool bFlyMode;
	bool bNormalizeAcceleration;
	bool bVoxelsCollision;

	vec3 Acceleration;
};
void MoveEntity(SGameState* GameState, SEngineState* EngineState, SEntity* Entity, SLevel* Level, const SMoveEntityInfo& MoveInfo, float dt)
{
	bool bGravity = MoveInfo.bGravity;
	bool bFlyMode = MoveInfo.bFlyMode;
	bool bVoxelsCollision = MoveInfo.bVoxelsCollision;
	vec3 Acceleration = MoveInfo.Acceleration;

	if (MoveInfo.bNormalizeAcceleration && (LengthSq(Acceleration) > 1.0f))
	{
		Acceleration = Normalize(Acceleration);
	}

	vec3 Drag = Vec3(0.0f);
	if (Entity->Type == Entity_Hero)
	{
		Acceleration *= GameState->HeroSpeed;
		Drag = -GameState->HeroDrag * Entity->Velocity;
	}
	else
	{
		Acceleration *= Entity->Speed;
		Drag = -Entity->Drag * Entity->Velocity;
	}
	Drag.y = 0.0f;

	if (!bFlyMode)
	{
		Acceleration += Drag;
	}

	if (bGravity)
	{
		Acceleration.y -= 9.8f;
	}
	
	vec3 DeltaPos = {};
	if (!bFlyMode)
	{
		Entity->Velocity += Acceleration*dt;
		DeltaPos = Entity->Velocity*dt + 0.5f*Acceleration*Square(dt);

		if (Entity->Type == Entity_Fireball)
		{
			if (LengthSq(Entity->Pos + DeltaPos - Entity->BasePos) > LengthSq(Entity->TargetOffset))
			{
				DeltaPos = (Entity->TargetOffset + Entity->BasePos) - Entity->Pos;
				Entity->bRemoved = true;
			}
		}
	}
	else
	{
		Entity->Pos += Acceleration*dt;
	}

	if (!bFlyMode)
	{
		for (uint32_t Iteration = 0; Iteration < 4; Iteration++)
		{
			if (Length(DeltaPos) > 0.0f)
			{
				vec3 DesiredPos = Entity->Pos + DeltaPos;

				float t = 1.0f;
				vec3 Normal = Vec3(0.0f);
				SEntity* HitEntity = 0;

				const SMesh& Mesh = EngineState->Geometry.Meshes[Entity->MeshIndex];
				Rect EntityAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, Mesh.Dim), (Entity->Type == Entity_Hero) ? Quat(0, 0, 0, 1) : EulerToQuat(Entity->Orientation.xyz));

				if (bVoxelsCollision)
				{
					uint32_t BroadPhaseRadius = 5;
					uint32_t EntityX = uint32_t(Entity->Pos.x / VoxelDim);
					uint32_t EntityY = uint32_t(Entity->Pos.y / VoxelDim);
					uint32_t EntityZ = uint32_t(Entity->Pos.z / VoxelDim);

					uint32_t StartX = (EntityX > BroadPhaseRadius) ? EntityX - BroadPhaseRadius : 0;
					uint32_t StartY = (EntityY > BroadPhaseRadius) ? EntityY - BroadPhaseRadius : 0;
					uint32_t StartZ = (EntityZ > BroadPhaseRadius) ? EntityZ - BroadPhaseRadius : 0;

					uint32_t EndX = (EntityX < LevelDimX - BroadPhaseRadius) ? EntityX + BroadPhaseRadius : LevelDimX - 1;
					uint32_t EndY = (EntityY < LevelDimY - BroadPhaseRadius) ? EntityY + BroadPhaseRadius : LevelDimY - 1;
					uint32_t EndZ = (EntityZ < LevelDimZ - BroadPhaseRadius) ? EntityZ + BroadPhaseRadius : LevelDimZ - 1;

					// Collision detection against level voxels
					for (uint32_t Z = StartZ; Z <= EndZ; Z++)
					{
						for (uint32_t Y = StartY; Y <= EndY; Y++)
						{
							for (uint32_t X = StartX; X <= EndX;)
							{
								if (IsVoxelActive(Level->Voxels, X, Y, Z))
								{
									Rect VoxelAABB = RectMinDim(VoxelDim * Vec3i(X, Y, Z), Vec3(VoxelDim));

									float tTest = 1.0f;
									vec3 NormalTest = Vec3(0.0f);
									EIntersectMovingRectsResult IntersectRes = IntersectMovingRects(EntityAABB, VoxelAABB, DeltaPos, tTest, NormalTest);
									if (IntersectRes == Intersect_AtStart)
									{
										Entity->Pos += tTest*NormalTest;
										DesiredPos += tTest*NormalTest;
										EntityAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, Mesh.Dim), (Entity->Type == Entity_Hero) ? Quat(0, 0, 0, 1) : EulerToQuat(Entity->Orientation.xyz));

										float Step = 2.0f;
										while (IntersectMovingRects(EntityAABB, VoxelAABB, DeltaPos, tTest, NormalTest) == Intersect_AtStart)
										{
											Entity->Pos += Step * tTest*NormalTest;
											DesiredPos += Step * tTest*NormalTest;
											EntityAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, Mesh.Dim), (Entity->Type == Entity_Hero) ? Quat(0, 0, 0, 1) : EulerToQuat(Entity->Orientation.xyz));

											Step *= 2.0f;
										}
									}
									else if (IntersectRes == Intersect_Moving)
									{
										tTest = Max(0.0f, tTest - 0.05f);
										if (tTest < t)
										{
											t = tTest;
											Normal = NormalTest;
										}
									}

									if (IntersectRes != Intersect_AtStart)
									{
										X++;
									}
								}
								else
								{
									X++;
								}
							}
						}
					}
				}

				// Collision detection against entities
				for (uint32_t J = 0; J < Level->EntityCount;)
				{
					SEntity* TestEntity = Level->Entities + J;
					if ((Entity != TestEntity) && !TestEntity->bRemoved && CanCollide(Entity, TestEntity))
					{
						const SMesh& TestMesh = EngineState->Geometry.Meshes[TestEntity->MeshIndex];
						Rect TestEntityAABB = RectCenterDimOrientation(TestEntity->Pos, Hadamard(TestEntity->Dim, TestMesh.Dim), (TestEntity->Type == Entity_Hero) ? Quat(0, 0, 0, 1) : EulerToQuat(TestEntity->Orientation.xyz));

						float tTest = 1.0f;
						vec3 NormalTest = Vec3(0.0f);
						EIntersectMovingRectsResult IntersectRes = IntersectMovingRects(EntityAABB, TestEntityAABB, DeltaPos, tTest, NormalTest);
						if ((IntersectRes == Intersect_AtStart) && BlockOnCollision(Entity, TestEntity))
						{
							Entity->Pos += tTest*NormalTest;
							DesiredPos += tTest*NormalTest;
							EntityAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, Mesh.Dim), (Entity->Type == Entity_Hero) ? Quat(0, 0, 0, 1) : EulerToQuat(Entity->Orientation.xyz));

							float Step = 2.0f;
							while (IntersectMovingRects(EntityAABB, TestEntityAABB, DeltaPos, tTest, NormalTest) == Intersect_AtStart)
							{
								Entity->Pos += Step * tTest*NormalTest;
								DesiredPos += Step * tTest*NormalTest;
								EntityAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, Mesh.Dim), (Entity->Type == Entity_Hero) ? Quat(0, 0, 0, 1) : EulerToQuat(Entity->Orientation.xyz));

								Step *= 2.0f;
							}
						}
						else if (IntersectRes == Intersect_Moving)
						{
							tTest = Max(0.0f, tTest - 0.05f);
							if (tTest < t)
							{
								if (BlockOnCollision(Entity, TestEntity))
								{
									t = tTest;
									Normal = NormalTest;
								}
								HitEntity = TestEntity;
							}

							J++;
						}
						else
						{
							if (IntersectRes == Intersect_AtStart)
							{
								HitEntity = TestEntity;
							}
							J++;
						}
					}
					else
					{
						J++;
					}
				}

				Entity->Pos += t*DeltaPos;
				DeltaPos = DesiredPos - Entity->Pos;
				if (t < 1.0f)
				{
					DeltaPos -= 1.01f * Dot(Normal, DeltaPos)*Normal;
					Entity->Velocity -= 1.01f * Dot(Normal, Entity->Velocity)*Normal;
				}

				if (HitEntity)
				{
					HandleCollision(GameState, EngineState, Level, Entity, HitEntity);
				}

				if (Entity->Type == Entity_Fireball)
				{
					if (t < 1.0f)
					{
						Entity->bRemoved = true;
					}
				}
			}
		}
	}
}