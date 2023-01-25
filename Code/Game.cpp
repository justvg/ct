#include "Game.h"

void DisableNonPersistentSounds(SAudioState* AudioState)
{
	for (uint32_t I = 0; I < AudioState->PlayingSoundCount; I++)
	{
		if ((AudioState->PlayingSounds[I].SoundID == Sound_PortalAmbient) || (AudioState->PlayingSounds[I].SoundID == Sound_ColorFieldAmbient))
		{
			AudioState->PlayingSounds[I].bFinished = true;
		}
	}
}

struct SMainHubSavedData
{
	bool bValid;

	float CameraPitch;
	float CameraHead;

	uint32_t TextsToRenderCount;
	SText TextsToRender[ArrayCount(SEngineState::TextsToRender)];

	vec3 LastBaseLevelPos;
	float LastBaseLevelGatesAngle;
};

SMainHubSavedData LoadMainHubAndSavedData(SEngineState* EngineState)
{
	SMainHubSavedData MainHubSavedData = {};

	FILE* File = fopen("Saves\\MainHubSaved.ctl", "rb");
	if (File)
	{
		MainHubSavedData.bValid = true;

		LoadLevel(EngineState, &EngineState->LevelBaseState, File, "Levels\\MainHub.ctl");
		fread(&MainHubSavedData.CameraPitch, sizeof(MainHubSavedData.CameraPitch), 1, File);
		fread(&MainHubSavedData.CameraHead, sizeof(MainHubSavedData.CameraHead), 1, File);
		fread(&MainHubSavedData.TextsToRenderCount, sizeof(MainHubSavedData.TextsToRenderCount), 1, File);
		fread(MainHubSavedData.TextsToRender, MainHubSavedData.TextsToRenderCount * sizeof(SText), 1, File);
		fread(&MainHubSavedData.LastBaseLevelPos, sizeof(MainHubSavedData.LastBaseLevelPos), 1, File);
		fread(&MainHubSavedData.LastBaseLevelGatesAngle, sizeof(MainHubSavedData.LastBaseLevelGatesAngle), 1, File);
	}

	return MainHubSavedData;
}

void SaveMainHubSavedDataInternal(void* Data)
{
	FILE* File = fopen("Saves\\MainHubSaved.ctl", "wb");
	if (File && Data)
	{
		// Save level
		const SLevel* Level = (const SLevel*) Data;
		SaveLevel(*Level, File);

		// Save main hub data
		const SMainHubSavedData* MainHubSavedData = (const SMainHubSavedData*) ((uint8_t*) Data + sizeof(SLevel));
		fwrite(&MainHubSavedData->CameraPitch, sizeof(MainHubSavedData->CameraPitch), 1, File);
		fwrite(&MainHubSavedData->CameraHead, sizeof(MainHubSavedData->CameraHead), 1, File);
		fwrite(&MainHubSavedData->TextsToRenderCount, sizeof(MainHubSavedData->TextsToRenderCount), 1, File);
		fwrite(MainHubSavedData->TextsToRender, MainHubSavedData->TextsToRenderCount * sizeof(SText), 1, File);
		fwrite(&MainHubSavedData->LastBaseLevelPos, sizeof(MainHubSavedData->LastBaseLevelPos), 1, File);
		fwrite(&MainHubSavedData->LastBaseLevelGatesAngle, sizeof(MainHubSavedData->LastBaseLevelGatesAngle), 1, File);

		fclose(File);
	}
}

void SaveMainHubSavedData(SEngineState* EngineState, const SGameState* GameState)
{
	void* Data = malloc(sizeof(SLevel) + sizeof(SMainHubSavedData));
	if (Data)
	{
		// Save level
		memcpy(Data, &EngineState->Level, sizeof(SLevel));

		// Save main hub data
		SMainHubSavedData* MainHubSavedData = (SMainHubSavedData*) ((uint8_t*) Data + sizeof(SLevel));
		MainHubSavedData->CameraPitch = EngineState->Camera.Pitch;
		MainHubSavedData->CameraHead = EngineState->Camera.Head;
		MainHubSavedData->TextsToRenderCount = EngineState->TextsToRenderCount;
		memcpy(MainHubSavedData->TextsToRender, EngineState->TextsToRender, MainHubSavedData->TextsToRenderCount * sizeof(SText));
		MainHubSavedData->LastBaseLevelPos = GameState->LastBaseLevelPos;
		MainHubSavedData->LastBaseLevelGatesAngle = GameState->LastBaseLevelGatesAngle;

		SDiskWriteInfo DiskWriteInfo = {};
		DiskWriteInfo.Data = Data;
		DiskWriteInfo.WriteFunction = SaveMainHubSavedDataInternal;

		AddEntryToWriteDiskThread(&EngineState->WriteDiskThread, DiskWriteInfo);
	}
}

void UpdateGameMode(SGameState* GameState, SEngineState* EngineState, const SGameInput* GameInput)
{
	char CurrentLevelName[ArrayCount(EngineState->LevelName)];
	memcpy(CurrentLevelName, EngineState->LevelName, ArrayCount(CurrentLevelName));

	SCamera* Camera = &EngineState->Camera;
	SLevel* Level = &EngineState->Level;
	SAudioState* AudioState = &EngineState->AudioState;

	if (WasDown(GameInput->Buttons[Button_R]))
	{
		if (!CompareStrings(EngineState->LevelName, "Levels\\MainHub.ctl"))
		{
			ReloadGameLevel(EngineState);
			GameState->CurrentCheckpointIndex = 0;
		}
	}

	Camera->PrevDir = Camera->Dir;
	Camera->PrevRight = Camera->Right;
	Camera->PrevUp = Camera->Up;
	
	Camera->PrevPitch = Camera->Pitch;
	Camera->PrevHead = Camera->Head;
	
	Camera->Pitch -= 0.1f*GameInput->MouseDeltaY;
	if (EngineState->bFlyMode)
	{
		Camera->Pitch = Clamp(Camera->Pitch, -89.0f, 89.0f);
	}
	else
	{
		Camera->Pitch = Clamp(Camera->Pitch, -60.0f, 89.0f);
	}
	
	Camera->Head -= 0.1f*GameInput->MouseDeltaX;
	if (Absolute(Camera->Head) >= 360.0f)
	{
		Camera->Head += -Sign(Camera->Head) * 360.0f;
	}
	
	Camera->Dir.x = Cos(Radians(Camera->Pitch)) * Sin(Radians(Camera->Head));
	Camera->Dir.y = Sin(Radians(Camera->Pitch));
	Camera->Dir.z = Cos(Radians(Camera->Pitch)) * Cos(Radians(Camera->Head));
	Camera->Dir = Normalize(Camera->Dir);
	
	Camera->Right = Normalize(Cross(Camera->Dir, Vec3(0.0f, 1.0f, 0.0f)));
	Camera->Up = Cross(Camera->Right, Camera->Dir);
	
	vec3 MovementDir;
	MovementDir.x = Sin(Radians(Camera->Head));
	MovementDir.y = 0.0f;
	MovementDir.z = Cos(Radians(Camera->Head));
	MovementDir = Normalize(MovementDir);
	SHeroControl HeroControl = {};
	if (GameInput->Buttons[Button_W].IsDown)
	{
		HeroControl.Acceleration += Vec3(MovementDir.x, 0.0f, MovementDir.z);
		if (EngineState->bFlyMode) HeroControl.Acceleration = Camera->Dir;
	}
	if (GameInput->Buttons[Button_S].IsDown)
	{
		HeroControl.Acceleration -= Vec3(MovementDir.x, 0.0f, MovementDir.z);
		if (EngineState->bFlyMode) HeroControl.Acceleration = -Camera->Dir;
	} 
	if (GameInput->Buttons[Button_D].IsDown)
	{
		HeroControl.Acceleration += Vec3(Camera->Right.x, 0.0f, Camera->Right.z);
		if (EngineState->bFlyMode) HeroControl.Acceleration = Camera->Right;
	} 
	if (GameInput->Buttons[Button_A].IsDown)
	{
		HeroControl.Acceleration -= Vec3(Camera->Right.x, 0.0f, Camera->Right.z);
		if (EngineState->bFlyMode) HeroControl.Acceleration = -Camera->Right;
	}

#ifndef ENGINE_RELEASE
	if (WasDown(GameInput->Buttons[Button_F]))
	{
		EngineState->bFlyMode = !EngineState->bFlyMode;
	}

	// if (WasDown(GameInput->Buttons[Button_V]))
	// {
	// 	PlatformChangeVSync(!PlatformGetVSync());
	// }

	// NOTE(georgii): Teleport between checkpoints.
	if (WasDown(GameInput->Buttons[Button_F2]))
	{
		SEntity* FoundCheckpoint = 0;
		for (uint32_t I = 0; I < Level->EntityCount; I++)
		{
			SEntity* Entity = Level->Entities + I;
			if ((Entity->Type == Entity_Checkpoint) && (Entity->CheckpointIndex == GameState->CurrentCheckpointIndex))
			{
				FoundCheckpoint = Entity;
				break;
			}
		}

		if (FoundCheckpoint)
		{
			SEntity* HeroEntity = &Level->Entities[0];
			Assert(HeroEntity->Type == Entity_Hero);

			HeroEntity->Pos = FoundCheckpoint->Pos;
			HeroEntity->Color = Vec3(0.0f);

			GameState->CurrentCheckpointIndex++;
		}
		else
		{
			ReloadGameLevel(EngineState, true, false);

			if (GameState->CurrentCheckpointIndex == 0)
			{
				GameState->CurrentCheckpointIndex++;
			}
			else if (GameState->CurrentCheckpointIndex > 0)
			{
				GameState->CurrentCheckpointIndex = 1;
			}
		}
	}
#endif
	
	if ((WasDown(GameInput->Buttons[Button_MouseLeft]) || WasDown(GameInput->Buttons[Button_MouseRight])) && !GameInput->bShowMouse)
	{
		if (GameState->LampTime >= GameState->LampTimer)
		{
			HeroControl.bUseLamp = true;
			GameState->LampTime = false;
		}
	}

	const float StandartTimeToChangeColor = 0.5f;
	if (GameState->bReload)
	{
		for (uint32_t I = 0; I < Level->EntityCount; I++)
		{
			const SEntity* Entity = Level->Entities + I;

			if (Entity->Type == Entity_Torch)
			{
				if (IsEqual(Entity->Color, Entity->TargetColor))
				{
					if ((Entity->DoorIndex > 0) && (Entity->DoorIndex < Level->EntityCount))
					{
						uint32_t DoorIndex = Entity->DoorIndex;
						Assert(Level->Entities[DoorIndex].Type == Entity_Door);

						if (!Level->Entities[DoorIndex].bForceClose)
						{
							Level->Entities[DoorIndex].bOpen = true;
						}
					}

					const SMesh& TorchMesh = EngineState->Geometry.Meshes[Entity->MeshIndex];
					Rect TorchAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, TorchMesh.Dim), EulerToQuat(Entity->Orientation.xyz));

					const SEntity* TestEntity = Entity;
					Rect TestAABB = TorchAABB;
					vec3 TestColor = Entity->TargetColor;
					while (TestEntity)
					{
						bool bFoundCollision = false;
						for (uint32_t I = 0; I < Level->EntityCount; I++)
						{
							SEntity* WireEntity = Level->Entities + I;
							
							if ((WireEntity->Type == Entity_Wire) && !WireEntity->bActive && IsEqual(TestColor, WireEntity->Color))
							{
								const SMesh& WireMesh = EngineState->Geometry.Meshes[WireEntity->MeshIndex];
								Rect WireAABB = RectCenterDimOrientation(WireEntity->Pos, Hadamard(WireEntity->Dim + Vec3(0.01f), WireMesh.Dim), EulerToQuat(WireEntity->Orientation.xyz));
								
								if (IntersectRects(TestAABB, WireAABB))
								{
									WireEntity->bActive = true;
									AnimateEntityColor(WireEntity, Vec3(StandartColorBoost), StandartTimeToChangeColor);

									TestEntity = WireEntity;
									TestAABB = WireAABB;

									bFoundCollision = true;
								}
							}
						}

						if (!bFoundCollision)
						{
							TestEntity = 0;
						}
					}
				}
			}
		}
	}

	for (uint32_t I = 0; I < Level->EntityCount; I++)
	{
		SEntity* Entity = Level->Entities + I;

		if ((Entity->Type == Entity_Door) && !Entity->bForceClose)
		{
			bool bOpen = true;
			for (uint32_t J = 0; J < Level->EntityCount; J++)
			{
				const SEntity* TestEntity = Level->Entities + J;

				if ((TestEntity->Type == Entity_Torch) && (TestEntity->DoorIndex == I))
				{
					bOpen = bOpen && IsEqual(TestEntity->Color, TestEntity->TargetColor);
				}
			}

			Entity->bOpen = bOpen;
		}
	}
	
	Assert((Level->EntityCount == 0) || (Level->Entities[0].Type == Entity_Hero));
	if (HeroControl.bUseLamp && !GameState->bDeathAnimation && !Level->Entities[0].bChangeColorAnimation)
	{
		vec3 RayStart = Camera->Pos;
		vec3 RayDir = Camera->Dir;
		
		float tMin = GameState->HeroLampDistance;
		SEntity *HitEntity = 0;
		for (uint32_t I = 0; I < Level->EntityCount; I++)
		{
			SEntity* TestEntity = Level->Entities + I;
			
			if ((TestEntity->Type != Entity_Hero) && (TestEntity->Type != Entity_ColorField))
			{
				if ((TestEntity->Type != Entity_Door) || (TestEntity->Alpha > 0.05f))
				{
					const SMesh& TestMesh = EngineState->Geometry.Meshes[TestEntity->MeshIndex];
					Rect TestEntityAABB = RectCenterDimOrientation(TestEntity->Pos, Hadamard(TestEntity->Dim, TestMesh.Dim), EulerToQuat(TestEntity->Orientation.xyz));
					
					float tTest;
					if (IntersectRectRay(TestEntityAABB, RayStart, RayDir, tTest))
					{
						if (tTest < tMin)
						{
							tMin = tTest;
							HitEntity = TestEntity;
						}
					}
				}
			}
		}
		
		bool bVoxelHitFirst = false;
		SRaytraceVoxelsResult RaytraceVoxelsResult = RaytraceVoxels(Level, RayStart, RayDir, GameState->HeroLampDistance);
		if (RaytraceVoxelsResult.bHit)
		{
			if (RaytraceVoxelsResult.Distance < tMin)
			{
				bVoxelHitFirst = true;
				tMin = RaytraceVoxelsResult.Distance;

				HitEntity = 0;
			}
		}
		
		if (HitEntity)
		{
			SEntity* HeroEntity = &Level->Entities[0];
			Assert(HeroEntity->Type == Entity_Hero);

			switch (HitEntity->Type)
			{
				case Entity_Torch:
				{
					if (WasDown(GameInput->Buttons[Button_MouseLeft]))
					{
						vec3 ResultColor = HeroEntity->Color + HitEntity->Color;
						if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f) && (Length(HitEntity->Color) > 0.0f))
						{
							SPlayingSound* SuccessColorSound = PlaySound(&EngineState->AudioState, false, Sound_SuccessColor);
							ChangePitch(SuccessColorSound, RandomFloat(0.75f, 1.25f));
							ChangeVolume(SuccessColorSound, 0.0f, Vec2(0.1f));

							AnimateEntityColor(HitEntity, Vec3(0.0f), StandartTimeToChangeColor);
							AnimateEntityColor(HeroEntity, ResultColor, StandartTimeToChangeColor);

							vec3 HeroLampPos = HeroEntity->Pos + (HeroEntity->LampOffset.x * HeroEntity->Dim.x * Camera->Right) + (HeroEntity->LampOffset.z * Camera->Dir) + (HeroEntity->LampOffset.y * Camera->Up);
							SEntity* ColorParticleEntity = AddColorParticle(*Level, HitEntity->Pos, Vec3(0.0f), HeroLampPos, StandartTimeToChangeColor, true);
							AnimateEntityColor(ColorParticleEntity, HitEntity->Color, StandartTimeToChangeColor);
							
							const SMesh& TorchMesh = EngineState->Geometry.Meshes[HitEntity->MeshIndex];
							Rect TorchAABB = RectCenterDimOrientation(HitEntity->Pos, Hadamard(HitEntity->Dim, TorchMesh.Dim), EulerToQuat(HitEntity->Orientation.xyz));
		
							SEntity* TestEntity = HitEntity;
							Rect TestAABB = TorchAABB;
							while (TestEntity)
							{
								bool bFoundCollision = false;
								for (uint32_t I = 0; I < Level->EntityCount; I++)
								{
									SEntity* WireEntity = Level->Entities + I;
									
									if ((WireEntity->Type == Entity_Wire) && WireEntity->bActive)
									{
										const SMesh& WireMesh = EngineState->Geometry.Meshes[WireEntity->MeshIndex];
										Rect WireAABB = RectCenterDimOrientation(WireEntity->Pos, Hadamard(WireEntity->Dim + Vec3(0.01f), WireMesh.Dim), EulerToQuat(WireEntity->Orientation.xyz));
										
										if (IntersectRects(TestAABB, WireAABB))
										{
											WireEntity->bActive = false;
											AnimateEntityColor(WireEntity, Vec3(0.0f), StandartTimeToChangeColor);

											TestEntity = WireEntity;
											TestAABB = WireAABB;

											bFoundCollision = true;
										}
									}
								}

								if (!bFoundCollision)
								{
									TestEntity = 0;
								}
							}
						}
						else
						{
							SPlayingSound* NegativeColorSound = PlaySound(&EngineState->AudioState, false, Sound_NegativeColor);
							ChangePitch(NegativeColorSound, RandomFloat(0.9f, 1.25f));
							ChangeVolume(NegativeColorSound, 0.0f, Vec2(0.2f));
						}
					}
					else if (WasDown(GameInput->Buttons[Button_MouseRight]))
					{
						vec3 ResultColor = HeroEntity->Color + HitEntity->Color;
						if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f) && (Length(HeroEntity->Color) > 0.0f))
						{
							SPlayingSound* SuccessColorSound = PlaySound(&EngineState->AudioState, false, Sound_SuccessColor);
							ChangePitch(SuccessColorSound, RandomFloat(0.75f, 1.25f));
							ChangeVolume(SuccessColorSound, 0.0f, Vec2(0.1f));

							AnimateEntityColor(HitEntity, ResultColor, StandartTimeToChangeColor);
							AnimateEntityColor(HeroEntity, Vec3(0.0f), StandartTimeToChangeColor);

							vec3 HeroLampPos = HeroEntity->Pos + (HeroEntity->LampOffset.x * HeroEntity->Dim.x * Camera->Right) + (HeroEntity->LampOffset.z * Camera->Dir) + (HeroEntity->LampOffset.y * Camera->Up);
							SEntity* ColorParticleEntity = AddColorParticle(*Level, HeroLampPos, Vec3(0.0f), HitEntity->Pos, StandartTimeToChangeColor, false);
							AnimateEntityColor(ColorParticleEntity, HeroEntity->Color, StandartTimeToChangeColor);

							if (IsEqual(ResultColor, HitEntity->TargetColor))
							{
								const SMesh& TorchMesh = EngineState->Geometry.Meshes[HitEntity->MeshIndex];
								Rect TorchAABB = RectCenterDimOrientation(HitEntity->Pos, Hadamard(HitEntity->Dim, TorchMesh.Dim), EulerToQuat(HitEntity->Orientation.xyz));
			
								SEntity* TestEntity = HitEntity;
								Rect TestAABB = TorchAABB;
								vec3 TestColor = HitEntity->TargetColor;
								while (TestEntity)
								{
									bool bFoundCollision = false;
									for (uint32_t I = 0; I < Level->EntityCount; I++)
									{
										SEntity* WireEntity = Level->Entities + I;
										
										if ((WireEntity->Type == Entity_Wire) && !WireEntity->bActive && IsEqual(TestColor, WireEntity->Color))
										{
											const SMesh& WireMesh = EngineState->Geometry.Meshes[WireEntity->MeshIndex];
											Rect WireAABB = RectCenterDimOrientation(WireEntity->Pos, Hadamard(WireEntity->Dim + Vec3(0.01f), WireMesh.Dim), EulerToQuat(WireEntity->Orientation.xyz));
											
											if (IntersectRects(TestAABB, WireAABB))
											{
												WireEntity->bActive = true;
												AnimateEntityColor(WireEntity, Vec3(StandartColorBoost), StandartTimeToChangeColor);

												TestEntity = WireEntity;
												TestAABB = WireAABB;

												bFoundCollision = true;
											}
										}
									}

									if (!bFoundCollision)
									{
										TestEntity = 0;
									}
								}
							}
						}
						else
						{
							SPlayingSound* NegativeColorSound = PlaySound(&EngineState->AudioState, false, Sound_NegativeColor);
							ChangePitch(NegativeColorSound, RandomFloat(0.9f, 1.25f));
							ChangeVolume(NegativeColorSound, 0.0f, Vec2(0.2f));
						}
					}
				} break;

				case Entity_Container:
				{
					if (WasDown(GameInput->Buttons[Button_MouseLeft]) || WasDown(GameInput->Buttons[Button_MouseRight]))
					{
						vec3 TempColor = HeroEntity->Color;
						HeroEntity->Color = HitEntity->Color;
						HitEntity->Color = TempColor;
					} 
				} break;
				
				case Entity_Fireball:
				{
					if (WasDown(GameInput->Buttons[Button_MouseLeft]))
					{
						vec3 ResultColor = HeroEntity->Color + HitEntity->Color;
						if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f))
						{
							HitEntity->Color = Vec3(0.0f);
							HitEntity->bRemoved = true;

							AnimateEntityColor(HeroEntity, ResultColor, StandartTimeToChangeColor);
						}
					}
				} break;
			}
		}
	}
	
	BEGIN_PROFILER_BLOCK("ENTITIES_SIMULATION");
	if (!GameState->bDeathAnimation)
	{
		for (uint32_t EntityIndex = 0; EntityIndex < Level->EntityCount; EntityIndex++)
		{
			SEntity* Entity = Level->Entities + EntityIndex;
			
			if (Entity->bRemoved)
			{
				continue;
			}

			Entity->PrevPos = Entity->Pos;
			Entity->PrevOrientation = Entity->Orientation;

			if (CanAnimateColor(Entity) && Entity->bChangeColorAnimation)
			{
				if (Entity->TimeToChangeColorCurrent < Entity->TimeToChangeColor)
				{
					UpdateColorAnimation(Entity, GameInput->dt);
				}
				else
				{
					StopEntityColorAnimation(Entity);
				}
			}
			
			switch (Entity->Type)
			{
				case Entity_Hero:
				{
					Entity->PrevLampOffset = Entity->LampOffset;
					if (HeroControl.bUseLamp)
					{
						Entity->LampOffset.z -= 0.15f;
					}

					SMoveEntityInfo MoveInfo = { !EngineState->bFlyMode, EngineState->bFlyMode, true, true, HeroControl.Acceleration };
					MoveEntity(GameState, EngineState, Entity, Level, MoveInfo, GameInput->dt);

					if (Length(Vec3(Entity->Velocity.x, 0.0f, Entity->Velocity.z)) > 0.2f)
					{
						Entity->LampOffset.y += GameInput->dt * 0.1f * Sin(5.0f * GameState->StepHandMovementTime);
						GameState->StepHandMovementTime += GameInput->dt;

						if (GameState->StepSoundTime >= GameState->StepSoundTimer)
						{
							ChangePitch(PlaySound(&EngineState->AudioState, false, RandomU32(Sound_Footstep0, Sound_Footstep4), false), RandomFloat(0.75f, 1.0f));
						}
					}
					else
					{
						GameState->StepHandMovementTime = 0.0f;
					}

					Entity->LampOffset += 15.0f * (Entity->LampBaseOffset - Entity->LampOffset) * GameInput->dt;
				} break;
				
				case Entity_Door:
				{
					Entity->Alpha = Lerp(0.99999f, 0.0f, Min(Entity->TimeToDisappearCurrent / Entity->TimeToDisappear, 1.0f));

					if (!Entity->bForceClose)
					{
						if (Entity->bOpen)
						{
							Entity->TimeToDisappearCurrent += GameInput->dt;
						}
						else if (Entity->bGoBack)
						{
							Entity->TimeToDisappearCurrent -= GameInput->dt;
						}
					}
					else
					{
						Entity->TimeToDisappearCurrent -= GameInput->dt;
					}
					
					Entity->TimeToDisappearCurrent = Clamp(Entity->TimeToDisappearCurrent, 0.0f, Entity->TimeToDisappear);
				} break;
				
				case Entity_Turret:
				{
					if (Entity->TimeToShootCurrent < Entity->TimeToShoot)
					{
						Entity->TimeToShootCurrent += GameInput->dt;
					}
					else
					{
						PlaySound(&EngineState->AudioState, false, RandomU32(Sound_Turret0, Sound_Turret2), false, true, Entity->Pos);
						AddFireball(*Level, Entity->Pos, Entity->FireballDim, Entity->Orientation.xyz, Entity->FireballColor, RotateByQuaternion(Entity->TargetOffset, EulerToQuat(Entity->Orientation.xyz)), Entity->Speed, LengthSq(Entity->FireballColor) == 0.0f);
						Entity->TimeToShootCurrent = 0.0f;
					}
				} break;
				
				case Entity_Fireball:
				{
					if (LengthSq(Entity->Pos - Entity->BasePos) < LengthSq(Entity->TargetOffset))
					{
						vec3 Acceleration = Normalize(Entity->TargetOffset) * Entity->Speed;
						
						SMoveEntityInfo MoveInfo = { false, false, false, true, Acceleration };
						MoveEntity(GameState, EngineState, Entity, Level, MoveInfo, GameInput->dt);
						
						Entity->TimeToDisappearCurrent += GameInput->dt;
					}
					else
					{
						Entity->bRemoved = true;
					}
				} break;

				case Entity_ColorParticle:
				{
					if (Entity->bTargetHero)
					{
						const SEntity* HeroEntity = &Level->Entities[0];
						Assert(HeroEntity->Type == Entity_Hero);
						Entity->TargetPos = HeroEntity->Pos + (HeroEntity->LampOffset.x * HeroEntity->Dim.x * Camera->Right) + (HeroEntity->LampOffset.z * Camera->Dir) + (HeroEntity->LampOffset.y * Camera->Up);
					}

					float t = Entity->TimeToMoveCurrent / Entity->TimeToMove;
					Entity->Pos = Lerp(Entity->Pos, Entity->TargetPos, t);

					Entity->TimeToMoveCurrent += GameInput->dt;
					Entity->TimeToMoveCurrent = Clamp(Entity->TimeToMoveCurrent, 0.0f, Entity->TimeToMove);

					if (t >= 1.0f)
					{
						Entity->bRemoved = true;
					}
				} break;

				case Entity_Gates:
				{
					if (!Entity->bSoundStarted)
					{
						SPlayingSound* PortalAmbientSound = PlaySound(&EngineState->AudioState, true, Sound_PortalAmbient, false, true, Entity->Pos);
						ChangePitch(PortalAmbientSound, RandomFloat(0.8f, 1.2f));
						ChangeVolume(PortalAmbientSound, 0.0f, Vec2(0.25f));

						Entity->bSoundStarted = true;
					}
				} break;

				case Entity_ColorField:
				{
					if (!Entity->bSoundStarted)
					{
						SPlayingSound* PortalAmbientSound = PlaySound(&EngineState->AudioState, true, Sound_ColorFieldAmbient, false, true, Entity->Pos);
						ChangePitch(PortalAmbientSound, RandomFloat(0.8f, 1.2f));
						ChangeVolume(PortalAmbientSound, 0.0f, Vec2(0.25f));

						Entity->bSoundStarted = true;
					}
				} break;
			}
		}

		for (uint32_t EntityIndex = 0; EntityIndex < Level->EntityCount; EntityIndex++)
		{
			SEntity* Entity = Level->Entities + EntityIndex;
				
			if ((Entity->Type == Entity_Gates) && Entity->bCollisionWithHeroStarted)
			{
				if (Entity->CollisionWithHeroTimePassed > 0.015f)
				{
					ChangeVolume(PlaySound(&EngineState->AudioState, false, Sound_Portal), 0.0f, Vec2(0.3f));

					if (Entity->CollisionWithHeroTimePassed > 0.05f)
					{
						bool bTargetMainHub = CompareStrings(Entity->TargetLevelName, "MainHub");
					#if ENGINE_RELEASE
						if (bTargetMainHub)
						{
							SMainHubSavedData MainHubSavedData = LoadMainHubAndSavedData(EngineState);
							if (MainHubSavedData.bValid)
							{
								GameState->LastBaseLevelPos = MainHubSavedData.LastBaseLevelPos;
								GameState->LastBaseLevelGatesAngle = MainHubSavedData.LastBaseLevelGatesAngle;
							}
							else
							{
								char LevelName[MaxPath] = {};
								ConcStrings(LevelName, sizeof(LevelName), Entity->TargetLevelName, ".ctl");
								LoadLevel(EngineState, &EngineState->LevelBaseState, LevelName, true, Level->Entities[0].Velocity);
							}
						}
						else
					#endif
						{
							char LevelName[MaxPath] = {};
							ConcStrings(LevelName, sizeof(LevelName), Entity->TargetLevelName, ".ctl");
							LoadLevel(EngineState, &EngineState->LevelBaseState, LevelName, true, Level->Entities[0].Velocity);
						}

						if (bTargetMainHub && Entity->bFinishGates)
						{
							SLevel* LoadedLevel = &EngineState->LevelBaseState;
							for (uint32_t LoadedLevelEntityIndex = 0; LoadedLevelEntityIndex < LoadedLevel->EntityCount; LoadedLevelEntityIndex++)
							{
								SEntity* LoadedLevelEntity = LoadedLevel->Entities + LoadedLevelEntityIndex;

								if (LoadedLevelEntity->Type == Entity_Gates)
								{
									char LevelName[MaxPath] = {};
									ConcStrings(LevelName, sizeof(LevelName), "Levels\\", LoadedLevelEntity->TargetLevelName);
									ConcStrings(LevelName, sizeof(LevelName), LevelName, ".ctl");

									if (CompareStrings(LevelName, CurrentLevelName))
									{
										LoadedLevelEntity->bFinishedLevel = true;
										break;
									}
								}
							}
						}

						SEntity* HeroEntity = &EngineState->LevelBaseState.Entities[0];

						float GatesAngleDifference = 0.0f;
						if (bTargetMainHub)
						{
							GatesAngleDifference = GameState->LastBaseLevelGatesAngle - Entity->Orientation.y;
						}
						else
						{
							GatesAngleDifference = HeroEntity->Orientation.y - GameState->LastBaseLevelGatesAngle;
						}

						quat Rotation = Quat(Vec3(0.0f, 1.0f, 0.0f), GatesAngleDifference);
						HeroEntity->Velocity = RotateByQuaternion(HeroEntity->Velocity, Rotation);
						Camera->Head += GatesAngleDifference;

						if (bTargetMainHub)
						{
							HeroEntity->Pos = GameState->LastBaseLevelPos + 0.1f * HeroEntity->Velocity;
						}
						
						Camera->Dir.x = Cos(Radians(Camera->Pitch)) * Sin(Radians(Camera->Head));
						Camera->Dir.y = Sin(Radians(Camera->Pitch));
						Camera->Dir.z = Cos(Radians(Camera->Pitch)) * Cos(Radians(Camera->Head));
						Camera->Dir = Normalize(Camera->Dir);
						
						Camera->Right = Normalize(Cross(Camera->Dir, Vec3(0.0f, 1.0f, 0.0f)));
						Camera->Up = Cross(Camera->Right, Camera->Dir);
						
						Entity->bCollisionWithHeroStarted = false;
						Entity->CollisionWithHeroTimePassed = 0.0f;
					}
					else
					{
						Entity->CollisionWithHeroTimePassed += GameInput->dt;
					}
				}
				else
				{
					Rect HeroAABB = RectCenterDim(Level->Entities[0].Pos, Level->Entities[0].Dim);

					const SMesh& GatesMesh = EngineState->Geometry.Meshes[Entity->MeshIndex];
					Rect GatesAABB = RectCenterDimOrientation(Entity->Pos, Hadamard(Entity->Dim, GatesMesh.Dim), EulerToQuat(Entity->Orientation.xyz));
					
					if (!IntersectRects(HeroAABB, GatesAABB))
					{
						Entity->bCollisionWithHeroStarted = false;
						Entity->CollisionWithHeroTimePassed = 0.0f;
					}
					else
					{
						Entity->CollisionWithHeroTimePassed += GameInput->dt;
					}
				}
			}
		}

		if (GameState->StepSoundTime >= GameState->StepSoundTimer)
		{
			GameState->StepSoundTime -= GameState->StepSoundTimer;
		}
		GameState->StepSoundTime += GameInput->dt;

		GameState->LampTime += GameInput->dt;
	}
	
	END_PROFILER_BLOCK("ENTITIES_SIMULATION");
	
	// NOTE(georgii): Simple hack for velocity
	if (!GameState->bDeathAnimation && Length(Level->Entities[0].Pos - Level->Entities[0].PrevPos) > 0.00001f)
	{
		Camera->Pos = Level->Entities[0].Pos + Camera->OffsetFromPlayer;
	}

	if (!GameState->bDeathAnimation)
	{
		GameState->DeathPosTimer += GameInput->dt;
		
		const float DeathPosUpdateTime = 1.5f;
		if (GameState->DeathPosTimer >= DeathPosUpdateTime)
		{
			GameState->DeathPosTimer -= DeathPosUpdateTime;

			if (GameState->PosForDeathAnimationCount == 0)
			{
				GameState->PosForDeathAnimation[GameState->PosForDeathAnimationCount++] = Level->Entities[0].Pos;
			}
			else
			{
				const uint32_t LastDeathPosIndex = GameState->PosForDeathAnimationCount - 1;
				if (Length(Level->Entities[0].Pos - GameState->PosForDeathAnimation[LastDeathPosIndex]) > 0.5f)
				{
					if (GameState->PosForDeathAnimationCount < ArrayCount(GameState->PosForDeathAnimation))
					{
						GameState->PosForDeathAnimation[GameState->PosForDeathAnimationCount++] = Level->Entities[0].Pos;
					}
				}
			}
		}
	}

	if (GameState->bDeathAnimation && !EngineState->bReloadLevel && !EngineState->bReloadLevelEditor)
	{
		Assert(GameState->PosForDeathAnimationCount >= 2);
		const uint32_t CurrentIndex = GameState->PosForDeathAnimationCount - 1;
		const uint32_t NextIndex = GameState->PosForDeathAnimationCount - 2;

		vec3 MoveDelta = GameState->DeathAnimationSpeed * GameInput->dt * NormalizeSafe0(GameState->PosForDeathAnimation[NextIndex] - GameState->PosForDeathAnimation[CurrentIndex]);
		Camera->Pos += MoveDelta;
		GameState->DeathAnimationLengthMoved += Length(MoveDelta);

		if ((Length((Camera->Pos - Camera->OffsetFromPlayer) - GameState->PosForDeathAnimation[NextIndex]) < 0.07f) ||
			(GameState->DeathAnimationLengthMoved >= Length(GameState->PosForDeathAnimation[NextIndex] - GameState->PosForDeathAnimation[CurrentIndex])))
		{
			GameState->PosForDeathAnimationCount--;
			GameState->DeathAnimationLengthMoved = 0.0f;

			Assert(GameState->PosForDeathAnimationCount > 0);
			if (GameState->PosForDeathAnimationCount == 1)
			{
				GameState->bDeathAnimation = false;
				Level->Entities[0].Pos = GameState->LastCheckpointPos;
			}
		}
	}
}

bool MenuItemDefault(SEngineState* EngineState, SMenuState* MenuState, const char* String, vec2 Pos, float Scale, vec2 ScreenDim, vec4 Color, vec2 MousePos, EFont Font = Font_KarminaRegular, ETextAlignment Alignment = TextAlignment_Center)
{
	float Blend = Clamp(MenuState->OpenTime / MenuState->OpenAnimationTime, 0.0f, 1.0f);
	AddTextMenu(EngineState, String, Pos, Scale, Font, Color, Blend, Alignment);

	vec2 ScreenSizeScale = Vec2(ScreenDim.x / 1920.0f, ScreenDim.y / 1080.0f);
	vec2 TextScale = Scale * ScreenSizeScale;
	vec2 TextSize = GetTextSize(&EngineState->Renderer.Fonts[Font], TextScale, String);

	vec2 ScreenPosition = Hadamard(Pos, 0.5f * ScreenDim);
	if (Alignment == TextAlignment_Center)
	{
		ScreenPosition -= 0.5f * TextSize;
	}
	else if (Alignment == TextAlignment_Left)
	{
		ScreenPosition.y -= 0.5f * TextSize.y;
	}
	else
	{
		Assert(Alignment == TextAlignment_Right);
		ScreenPosition -= Vec2(TextSize.x, 0.5f * TextSize.y);
	}

	vec4 TextRect = Vec4(ScreenPosition, ScreenPosition + TextSize);
	return IsPointInRect(MousePos, TextRect);
}

vec4 GetMenuItemColor(SMenuState* MenuState, bool bSelected)
{
	vec4 Color = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
	if (bSelected)
	{
		vec4 MinColor = Vec4(0.6f, 0.6f, 0.6f, 1.0f);
		vec4 MaxColor = Vec4(0.9f, 0.9f, 0.9f, 1.0f);

		if (MenuState->SelectedTime <= MenuState->SelectedStayBrightTime)
		{
			Color = MaxColor;
		}
		else
		{
			float SelectedAnimationPeriod = 0.5f * MenuState->SelectedAnimationTime;
			float BlendFactor = Absolute(SelectedAnimationPeriod - (MenuState->SelectedTime - MenuState->SelectedStayBrightTime));
			Color = Lerp(MinColor, MaxColor, BlendFactor);
		}
	}

	return Color;
}

void UpdateMenuDefault(SMenuState* MenuState, SEngineState* EngineState, const SGameInput* GameInput, const SVulkanContext* Vulkan)
{
	AddTextMenu(EngineState, "COLOR THIEF", Vec2(0.0f, 0.7f), 0.4f, Font_KarminaBold, Vec4(1.0f));

	const float TextScale = 0.2f;
	for (int32_t MenuElement = MenuElement_DefaultNone; MenuElement < MenuElement_DefaultCount; MenuElement++)
	{
		bool bSelected = (MenuState->SelectedMenuElement == MenuElement);
		vec4 Color = GetMenuItemColor(MenuState, bSelected);

		bool bMouseInItem = false;
		switch (MenuElement)
		{
			case MenuElement_Settings:
			{
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Settings", Vec2(0.0f, 0.12f), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos);
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_Settings;
				}
			} break;

			case MenuElement_StartNewGame:
			{
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Start A New Game", Vec2(0.0f, 0.0f), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos);
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_StartNewGame;
				}
			} break;

			case MenuElement_Quit:
			{
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Quit", Vec2(0.0f, -0.12f), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos);
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_Quit;
				}
			} break;
		}

		if ((bSelected && MenuState->bEnterDown) || (bSelected && bMouseInItem && MenuState->bMouseLeftReleased))
		{
			switch (MenuElement)
			{
				case MenuElement_Settings:
				{
					MenuState->MenuMode = MenuMode_Settings;
				} break;

				case MenuElement_StartNewGame:
				{
					MenuState->MenuMode = MenuMode_StartNewGame;
				} break;

				case MenuElement_Quit:
				{
					PlatformQuitGame();
				} break;
			}
		}
	}
}

void UpdateMenuSettings(SMenuState* MenuState, SEngineState* EngineState, const SGameInput* GameInput, SVulkanContext* Vulkan)
{
	AddTextMenu(EngineState, "SETTINGS", Vec2(0.0f, 0.7f), 0.4f, Font_KarminaBold, Vec4(1.0f));

	const float TextScale = 0.15f;
	const float LeftPos = -0.5f;
	const float RightPos = 0.5f;
	const EFont Font = Font_KarminaRegular;
	for (int32_t MenuElement = MenuElement_SettingsNone; MenuElement < MenuElement_SettingsCount; MenuElement++)
	{
		bool bSelected = (MenuState->SelectedMenuElement == MenuElement);
		vec4 Color = GetMenuItemColor(MenuState, bSelected);

		bool bMouseInItem = false;
		switch (MenuElement)
		{
			case MenuElement_Fullscreen:
			{
				const float YPos = 0.36f;

				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Fullscreen:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, PlatformGetFullscreen() ? "Yes" : "No", Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_Fullscreen;
				}
			} break;

			case MenuElement_VSync:
			{
				const float YPos = 0.24f;

				bMouseInItem = MenuItemDefault(EngineState, MenuState, "V-Sync:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, PlatformGetVSync() ? "Yes" : "No", Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_VSync;
				}
			} break;

			case MenuElement_Vignetting:
			{
				const float YPos = 0.12f;

				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Vignetting:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, EngineState->bVignetteEnabled ? "Yes" : "No", Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_Vignetting;
				}
			} break;

			case MenuElement_AOQuality:
			{
				const float YPos = 0.0f;

				char *AOText = 0;
				switch (EngineState->Renderer.AOQuality)
				{
					case AOQuality_Low:
					{
						AOText = "Low";
					} break;

					case AOQuality_Medium:
					{
						AOText = "Medium";
					} break;

					case AOQuality_High:
					{
						AOText = "High";
					} break;

					case AOQuality_VeryHigh:
					{
						AOText = "Very High";
					} break;
				}

				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Ambient Occlusion:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, AOText, Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_AOQuality;
				}
			} break;

			case MenuElement_Resolution:
			{
				const float YPos = -0.12f;

				char ResolutionText[32] = {};
				snprintf(ResolutionText, sizeof(ResolutionText), "%d%%", EngineState->FullscreenResolutionPercent);
	
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Resolution:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, ResolutionText, Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_Resolution;
				}
			} break;

			case MenuElement_Volume:
			{
				const float YPos = -0.24f;

				char VolumeText[32] = {};
				snprintf(VolumeText, sizeof(VolumeText), "%d%%", EngineState->AudioState.MasterVolume);
	
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Audio volume:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, VolumeText, Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_Volume;
				}
			} break;

			case MenuElement_MusicVolume:
			{
				const float YPos = -0.36f;

				char VolumeText[32] = {};
				snprintf(VolumeText, sizeof(VolumeText), "%d%%", EngineState->AudioState.MusicVolume);
	
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Music volume:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, VolumeText, Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_MusicVolume;
				}
			} break;

			case MenuElement_EffectsVolume:
			{
				const float YPos = -0.48f;

				char VolumeText[32] = {};
				snprintf(VolumeText, sizeof(VolumeText), "%d%%", EngineState->AudioState.EffectsVolume);
	
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Effects volume:", Vec2(LeftPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Left);
				bMouseInItem = MenuItemDefault(EngineState, MenuState, VolumeText, Vec2(RightPos, YPos), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos, Font, TextAlignment_Right) || bMouseInItem;
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_EffectsVolume;
				}
			} break;
		}

		if (bSelected &&
			(MenuState->bArrowUsed || 
			(bMouseInItem && MenuState->bMouseLeftReleased) || 
			(((MenuElement == MenuElement_Volume) || (MenuElement == MenuElement_MusicVolume) || (MenuElement == MenuElement_EffectsVolume)) && MenuState->bArrowPressed)))
		{
			switch (MenuElement)
			{
				case MenuElement_Fullscreen:
				{
					PlatformChangeFullscreen(!PlatformGetFullscreen());

					if (PlatformGetFullscreen())
					{
						EngineState->bSwapchainChanged = true;
					}
				} break;

				case MenuElement_VSync:
				{
					PlatformChangeVSync(!PlatformGetVSync());
				} break;

				case MenuElement_Vignetting:
				{
					EngineState->bVignetteEnabled = !EngineState->bVignetteEnabled;
				} break;

				case MenuElement_AOQuality:
				{
					int32_t AOQuality = EngineState->Renderer.AOQuality;
					if (MenuState->bArrowRight || (!MenuState->bArrowRight && !MenuState->bArrowLeft))
					{
						AOQuality++;
						if (AOQuality == AOQuality_Count)
						{
							AOQuality = AOQuality_Low;
						}
					}
					else
					{
						AOQuality--;
						if (AOQuality < 0)
						{
							AOQuality = AOQuality_VeryHigh;
						}
					}

					EngineState->Renderer.AOQuality = EAOQuality(AOQuality);
				} break;

				case MenuElement_Resolution:
				{
					if (PlatformGetFullscreen())
					{
						const uint32_t MinResolutionPercent = 50;
						const uint32_t MaxResolutionPercent = 100;
						const uint32_t ResolutionStep = 10;

						uint32_t ResolutionPercent = EngineState->FullscreenResolutionPercent;
						if (MenuState->bArrowRight || (!MenuState->bArrowRight && !MenuState->bArrowLeft))
						{
							if (ResolutionPercent == MaxResolutionPercent)
							{
								ResolutionPercent = MinResolutionPercent;
							}
							else
							{
								ResolutionPercent += ResolutionStep;
							}
						}
						else
						{
							if (ResolutionPercent == MinResolutionPercent)
							{
								ResolutionPercent = MaxResolutionPercent;
							}
							else
							{
								ResolutionPercent -= ResolutionStep;
							}
						}

						Assert((ResolutionPercent <= MaxResolutionPercent) && (ResolutionPercent >= MinResolutionPercent));
						EngineState->FullscreenResolutionPercent = ResolutionPercent;
						EngineState->bSwapchainChanged = true;
					}
				} break;

				case MenuElement_Volume:
				{
					uint32_t NewMasterVolume = EngineState->AudioState.MasterVolume;
					if (MenuState->bArrowRightPressed || (!MenuState->bArrowRightPressed && !MenuState->bArrowLeftPressed))
					{
						NewMasterVolume += 1;
					}
					else
					{
						NewMasterVolume -= 1;
					}

					EngineState->AudioState.MasterVolume = Clamp(NewMasterVolume, 0, 100);
				} break;

				case MenuElement_MusicVolume:
				{
					uint32_t NewMusicVolume = EngineState->AudioState.MusicVolume;
					if (MenuState->bArrowRightPressed || (!MenuState->bArrowRightPressed && !MenuState->bArrowLeftPressed))
					{
						NewMusicVolume += 1;
					}
					else
					{
						NewMusicVolume -= 1;
					}

					EngineState->AudioState.MusicVolume = Clamp(NewMusicVolume, 0, 100);
				} break;

				case MenuElement_EffectsVolume:
				{
					uint32_t NewEffectsVolume = EngineState->AudioState.EffectsVolume;
					if (MenuState->bArrowRightPressed || (!MenuState->bArrowRightPressed && !MenuState->bArrowLeftPressed))
					{
						NewEffectsVolume += 1;
					}
					else
					{
						NewEffectsVolume -= 1;
					}

					EngineState->AudioState.EffectsVolume = Clamp(NewEffectsVolume, 0, 100);
				} break;
			}
		}
	}

	if (WasDown(GameInput->Buttons[Button_Escape]))
	{
		MenuState->MenuMode = MenuMode_Default;
		MenuState->SelectedMenuElement = MenuElement_Settings;
	}
}

void UpdateMenuStartNewGame(SMenuState* MenuState, SEngineState* EngineState, const SGameInput* GameInput, const SVulkanContext* Vulkan)
{
	AddTextMenu(EngineState, "START A NEW GAME", Vec2(0.0f, 0.7f), 0.4f, Font_KarminaBold, Vec4(1.0f));

	const float TextScale = 0.2f;
	for (int32_t MenuElement = MenuElement_StartNewGameNone; MenuElement < MenuElement_StartNewGameCount; MenuElement++)
	{
		bool bSelected = (MenuState->SelectedMenuElement == MenuElement);
		vec4 Color = GetMenuItemColor(MenuState, bSelected);

		bool bMouseInItem = false;
		switch (MenuElement)
		{
			case MenuElement_StartNewGameYes:
			{
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "Yes", Vec2(-0.25f, 0.35f), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos);
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_StartNewGameYes;
				}
			} break;

			case MenuElement_StartNewGameNo:
			{
				bMouseInItem = MenuItemDefault(EngineState, MenuState, "No", Vec2(0.25f, 0.35f), TextScale, MenuState->ScreenDim, Color, MenuState->MousePos);
				if (bMouseInItem && MenuState->bMousePosChanged)
				{
					if (!bSelected)
					{
						MenuState->SelectedTime = 0.0f;
						bSelected = true;
					}
					MenuState->SelectedMenuElement = MenuElement_StartNewGameNo;
				}
			} break;
		}

		if ((bSelected && MenuState->bEnterDown) || (bSelected && bMouseInItem && MenuState->bMouseLeftReleased))
		{
			switch (MenuElement)
			{
				case MenuElement_StartNewGameYes:
				{
					LoadLevel(EngineState, &EngineState->LevelBaseState, "MainHub.ctl");
					ReloadGameLevel(EngineState);

					EngineState->bMenuOpened = false;
					PlatformDisableCursor((SGameInput*) GameInput);
				} break;

				case MenuElement_StartNewGameNo:
				{
					MenuState->MenuMode = MenuMode_Default;
					MenuState->SelectedMenuElement = MenuElement_StartNewGame;
				} break;
			}
		}
	}

	if (WasDown(GameInput->Buttons[Button_Escape]))
	{
		MenuState->MenuMode = MenuMode_Default;
		MenuState->SelectedMenuElement = MenuElement_StartNewGame;
	}
}

void UpdateMenu(SMenuState* MenuState, SEngineState* EngineState, const SGameInput* GameInput, const SVulkanContext* Vulkan)
{
	EMenuElement MinMenuElement = (MenuState->MenuMode == MenuMode_Default) ? MenuElement_DefaultNone : (MenuState->MenuMode == MenuMode_Settings) ? MenuElement_SettingsNone : MenuElement_StartNewGameNone;
	EMenuElement MaxMenuElement = (MenuState->MenuMode == MenuMode_Default) ? MenuElement_DefaultCount : (MenuState->MenuMode == MenuMode_Settings) ? MenuElement_SettingsCount : MenuElement_StartNewGameCount;

	bool bNext = (MenuState->MenuMode != MenuMode_StartNewGame) ? WasDown(GameInput->Buttons[Button_W]) || WasDown(GameInput->Buttons[Button_ArrowUp]) : WasDown(GameInput->Buttons[Button_A]) || WasDown(GameInput->Buttons[Button_ArrowLeft]);
	bool bPrev = (MenuState->MenuMode != MenuMode_StartNewGame) ? WasDown(GameInput->Buttons[Button_S]) || WasDown(GameInput->Buttons[Button_ArrowDown]) : WasDown(GameInput->Buttons[Button_D]) || WasDown(GameInput->Buttons[Button_ArrowRight]);
	if (bNext || bPrev)
	{
		if (bNext)
		{
			MenuState->SelectedMenuElement = EMenuElement(MenuState->SelectedMenuElement - 1);

			if (MenuState->SelectedMenuElement == MinMenuElement)
			{
				MenuState->SelectedMenuElement = EMenuElement(MaxMenuElement - 1);
			}
		}
		else
		{
			MenuState->SelectedMenuElement = EMenuElement((MenuState->SelectedMenuElement + 1) % MaxMenuElement);
		}

		MenuState->SelectedMenuElement = EMenuElement(Clamp(MenuState->SelectedMenuElement, MinMenuElement + 1, MaxMenuElement - 1));
		MenuState->SelectedTime = 0.0f;
	}

	EngineState->MenuOpenedBlend = MenuState->OpenTime / MenuState->OpenAnimationTime;

	MenuState->MousePos = Vec2(float(GameInput->PlatformMouseX), float(GameInput->PlatformMouseY));
	MenuState->ScreenDim = Vec2i(Vulkan->Width, Vulkan->Height);

	if (MenuState->bJustOpened)
	{
		MenuState->LastMousePos = MenuState->MousePos;
		MenuState->bJustOpened = false;
	}

	MenuState->bMousePosChanged = !IsEqual(MenuState->MousePos, MenuState->LastMousePos) || WasDown(GameInput->Buttons[Button_MouseLeft]);
	MenuState->bEnterDown = WasDown(GameInput->Buttons[Button_Enter]) || WasDown(GameInput->Buttons[Button_Space]);
	MenuState->bMouseLeftReleased = WasReleased(GameInput->Buttons[Button_MouseLeft]);

	MenuState->bArrowLeft = WasDown(GameInput->Buttons[Button_A]) || WasDown(GameInput->Buttons[Button_ArrowLeft]);
	MenuState->bArrowRight = WasDown(GameInput->Buttons[Button_D]) || WasDown(GameInput->Buttons[Button_ArrowRight]);
	MenuState->bArrowUsed = MenuState->bArrowLeft || MenuState->bArrowRight;

	MenuState->bArrowLeftPressed = GameInput->Buttons[Button_A].IsDown || GameInput->Buttons[Button_ArrowLeft].IsDown;
	MenuState->bArrowRightPressed = GameInput->Buttons[Button_D].IsDown || GameInput->Buttons[Button_ArrowRight].IsDown;
	MenuState->bArrowPressed = MenuState->bArrowLeftPressed || MenuState->bArrowRightPressed;

	switch (MenuState->MenuMode)
	{
		case MenuMode_Default:
		{
			UpdateMenuDefault(MenuState, EngineState, GameInput, Vulkan);
		} break;

		case MenuMode_Settings:
		{
			UpdateMenuSettings(MenuState, EngineState, GameInput, (SVulkanContext*) Vulkan);
		} break;

		case MenuMode_StartNewGame:
		{
			UpdateMenuStartNewGame(MenuState, EngineState, GameInput, Vulkan);
		} break;
	}
	
	MenuState->LastMousePos = MenuState->MousePos;

	MenuState->SelectedTime += GameInput->dt;
	if (MenuState->SelectedTime >= (MenuState->SelectedStayBrightTime + MenuState->SelectedAnimationTime))
	{
		MenuState->SelectedTime -= MenuState->SelectedStayBrightTime + MenuState->SelectedAnimationTime;
	}

	if (MenuState->bDisableStarted)
	{
		MenuState->OpenTime = Max(MenuState->OpenTime - GameInput->dt, 0.0f);
	}
	else
	{
		MenuState->OpenTime = Min(MenuState->OpenTime + GameInput->dt, MenuState->OpenAnimationTime);
	}

	if ((MenuState->OpenTime == 0.0f) && MenuState->bDisableStarted)
	{
		EngineState->bMenuOpened = false;
		MenuState->MenuMode = MenuMode_Default;
		MenuState->SelectedMenuElement = MenuElement_DefaultNone;
		MenuState->bDisableStarted = false;

		PlatformToggleCursor((SGameInput*) GameInput, PlatformIsCursorShowed(), PlatformIsCursorShowed());
	}
}

void UpdateGame(SGameState* GameState, SEngineState* EngineState, const SGameInput* GameInput, const SVulkanContext* Vulkan)
{
	if (!GameState->bInitialized)
	{
		SParsedConfigFile ConfigFile = ParseConfigFile("Game.cfg");
		if (ConfigFile.ItemCount > 0)
		{
			for (uint32_t I = 0; I < ConfigFile.ItemCount; I++)
			{
				const SConfigFileItem& Item = ConfigFile.Items[I];

				if (CompareStrings(Item.Name, "HeroSpeed"))
				{
					GameState->HeroSpeed = Item.Value;
				}
				else if (CompareStrings(Item.Name, "HeroDrag"))
				{
					GameState->HeroDrag = Item.Value;
				}
				else if (CompareStrings(Item.Name, "HeroLampDistance"))
				{
					GameState->HeroLampDistance = Item.Value;
				}
			}
		}
		else
		{
			// NOTE(georgii): Use these values if for some reason there is no config file.
			GameState->HeroSpeed = 11.0f;
			GameState->HeroDrag = 3.1f;
			GameState->HeroLampDistance = 8.0f;
		}

        GameState->DeathAnimationSpeed = 10.0f;

		GameState->MenuState.OpenAnimationTime = 1.0f;
		GameState->MenuState.SelectedStayBrightTime = 1.0f;
		GameState->MenuState.SelectedAnimationTime = 2.0f;

		GameState->LampTimer = 0.75f;
		GameState->StepSoundTimer = 0.75f;

#if ENGINE_RELEASE
		// Load saved and set engine data
		SGeneralSaveData GeneralSaveData = LoadGeneralSaveData();
		if (GeneralSaveData.bValid)
		{
			if (CompareStrings(GeneralSaveData.LevelName, "Levels\\MainHub.ctl"))
			{
				SMainHubSavedData MainHubSavedData = LoadMainHubAndSavedData(EngineState);
				if (MainHubSavedData.bValid)
				{
					EngineState->Camera.Pitch = MainHubSavedData.CameraPitch;
					EngineState->Camera.Head = MainHubSavedData.CameraHead;

					EngineState->TextsToRenderCount = MainHubSavedData.TextsToRenderCount;
					memcpy(EngineState->TextsToRender, MainHubSavedData.TextsToRender, sizeof(EngineState->TextsToRender));

					// NOTE(georgii): Delete saved menu text
					for (uint32_t I = 0; I < EngineState->TextsToRenderCount;)
					{
						SText* Text = EngineState->TextsToRender + I;

						if (Text->bMenuText)
						{
							*Text = EngineState->TextsToRender[--EngineState->TextsToRenderCount];
						}
						else
						{
							I++;
						}
					}

					GameState->LastBaseLevelPos = MainHubSavedData.LastBaseLevelPos;
					GameState->LastBaseLevelGatesAngle = MainHubSavedData.LastBaseLevelGatesAngle;
				}
				else
				{
					LoadLevel(EngineState, &EngineState->LevelBaseState, "MainHub.ctl");
				}
			}
			else
			{
				LoadLevel(EngineState, &EngineState->LevelBaseState, GeneralSaveData.LevelName, false);
			}

			FreeGeneralSaveData(&GeneralSaveData);
		}
		else
		{
			LoadLevel(EngineState, &EngineState->LevelBaseState, "MainHub.ctl");
		}
#else
		LoadLevel(EngineState, &EngineState->LevelBaseState, "MainHub.ctl");
#endif

		PlayMusic(&EngineState->AudioState, true, Sound_PortalSoundtrack);

		GameState->bInitialized = true;
	}

	if (GameState->bReload)
	{
		GameState->bDeathAnimation = false;
		GameState->DeathAnimationLengthMoved = 0.0f;
		GameState->DeathPosTimer = 0.0f;

		GameState->CurrentCheckpointIndex = 0;
		GameState->LastCheckpointPos = EngineState->Level.Entities[0].Pos;

		GameState->PosForDeathAnimationCount = 1;
		GameState->PosForDeathAnimation[0] = GameState->LastCheckpointPos;

		DisableNonPersistentSounds(&EngineState->AudioState);
	}

	SMenuState* MenuState = &GameState->MenuState;
	if (WasDown(GameInput->Buttons[Button_Escape]))
	{
		if (!EngineState->bMenuOpened || ((MenuState->MenuMode != MenuMode_Settings) && (MenuState->MenuMode != MenuMode_StartNewGame)))
		{
			if (EngineState->bMenuOpened && !MenuState->bDisableStarted)
			{
				MenuState->bDisableStarted = true;
			}
			else
			{
				EngineState->bMenuOpened = true;
				MenuState->MenuMode = MenuMode_Default;
				MenuState->SelectedMenuElement = MenuElement_DefaultNone;
				MenuState->bJustOpened = true;
				MenuState->bDisableStarted = false;			
				
				PlatformToggleCursor((SGameInput*) GameInput, true, PlatformIsCursorShowed());
			}
		}
	}

	if (!EngineState->bMenuOpened && !MenuState->bDisableStarted)
	{
		UpdateGameMode(GameState, EngineState, GameInput);	
	}
	else
	{
		UpdateMenu(MenuState, EngineState, GameInput, Vulkan);
	}
	
#if ENGINE_RELEASE
	if (CompareStrings(EngineState->LevelName, "Levels\\MainHub.ctl"))
	{
		SaveMainHubSavedData(EngineState, GameState);
	}
#endif

	GameState->bReload = false;
}