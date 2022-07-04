#include "Game.h"

void UpdateGameMode(SGameState* GameState, SEngineState* EngineState, const SGameInput* GameInput, const char* CurrentLevelName)
{
	SCamera* Camera = &EngineState->Camera;
	SLevel* Level = &EngineState->Level;
	SAudioState* AudioState = &EngineState->AudioState;

	if (WasDown(GameInput->Buttons[Button_R]))
	{
		ReloadGameLevel(EngineState);
		GameState->CurrentCheckpointIndex = 0;
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
	
	if (WasDown(GameInput->Buttons[Button_Space]))
	{
		HeroControl.bJump = true;
	}
	if ((WasDown(GameInput->Buttons[Button_MouseLeft]) || WasDown(GameInput->Buttons[Button_MouseRight])) && !GameInput->bShowMouse)
	{
		HeroControl.bUseLamp = true;
	}
	
	if (HeroControl.bUseLamp && !GameState->bDeathAnimation)
	{
		// SPlayingSound* ShootSound = PlaySound(AudioState, false, 1);
		// ChangePitch(ShootSound, RandomFloat(1.15f, 2.0f));
		
		vec3 RayStart = Camera->Pos;
		vec3 RayDir = Camera->Dir;
		
		float tMin = GameState->HeroLampDistance;
		SEntity *HitEntity = 0;
		for (uint32_t I = 0; I < Level->EntityCount; I++)
		{
			SEntity* TestEntity = Level->Entities + I;
			
			if ((TestEntity->Type != Entity_Hero) && (TestEntity->Type != Entity_ColorField))
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
		
		bool bVoxelHitFirst = false;
		SRaytraceVoxelsResult RaytraceVoxelsResult = RaytraceVoxels(Level, RayStart, RayDir, GameState->HeroLampDistance);
		if (RaytraceVoxelsResult.bHit)
		{
			if (RaytraceVoxelsResult.Distance < tMin)
			{
				bVoxelHitFirst = true;
				tMin = RaytraceVoxelsResult.Distance;
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
						if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f))
						{
							HeroEntity->Color = ResultColor;
							HitEntity->Color = Vec3(0.0f);
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
				
				case Entity_Door:
				{
					if (!HitEntity->bForceClose)
					{
						if (WasDown(GameInput->Buttons[Button_MouseLeft]))
						{
							vec3 ResultColor = HeroEntity->Color + HitEntity->CurrentColor;
							if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f))
							{
								HeroEntity->Color = ResultColor;
								HitEntity->CurrentColor = Vec3(0.0f);
							}
						}
						else if (WasDown(GameInput->Buttons[Button_MouseRight]))
						{
							vec3 ResultColor = HeroEntity->Color + HitEntity->CurrentColor;
							if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f))
							{
								HitEntity->CurrentColor = ResultColor;
								HeroEntity->Color = Vec3(0.0f);
							}
						}
					}
				} break;
				
				case Entity_Fireball:
				{
					if (WasDown(GameInput->Buttons[Button_MouseLeft]))
					{
						vec3 ResultColor = HeroEntity->Color + HitEntity->Color;
						if ((ResultColor.r <= 1.0f) && (ResultColor.g <= 1.0f) && (ResultColor.b <= 1.0f))
						{
							HeroEntity->Color = ResultColor;
							HitEntity->Color = Vec3(0.0f);
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
			
			switch (Entity->Type)
			{
				case Entity_Hero:
				{
					if (HeroControl.bJump)
					{
						bool bCanJump = false;
						float MaxDistance = 0.1f;
						
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
						
						for (uint32_t Z = StartZ; (Z <= EndZ) && !bCanJump; Z++)
						{
							for (uint32_t Y = StartY; (Y <= EndY) && !bCanJump; Y++)
							{
								for (uint32_t X = StartX; (X <= EndX) && !bCanJump; X++)
								{
									if (IsVoxelActive(Level->Voxels, X, Y, Z))
									{
										Rect VoxelAABB = RectMinDim(VoxelDim * Vec3i(X, Y, Z), Vec3(VoxelDim));
										
										float tTest = 0.0f;
										if (IntersectRectRay(VoxelAABB, Entity->Pos + Vec3(0.0f, -0.5f*Entity->Dim.y, 0.0f), Vec3(0.0f, -1.0f, 0.0f), tTest))
										{
											if (tTest < MaxDistance)
											{
												bCanJump = true;
												break;
											}
										}
										
										if (IntersectRectRay(VoxelAABB, Entity->Pos + 0.5f*Vec3(Entity->Dim.x, -Entity->Dim.y, Entity->Dim.z), Vec3(0.0f, -1.0f, 0.0f), tTest))
										{
											if (tTest < MaxDistance)
											{
												bCanJump = true;
												break;
											}
										}
										
										if (IntersectRectRay(VoxelAABB, Entity->Pos + 0.5f*Vec3(Entity->Dim.x, -Entity->Dim.y, -Entity->Dim.z), Vec3(0.0f, -1.0f, 0.0f), tTest))
										{
											if (tTest < MaxDistance)
											{
												bCanJump = true;
												break;
											}
										}
										
										if (IntersectRectRay(VoxelAABB, Entity->Pos + 0.5f*Vec3(-Entity->Dim.x, -Entity->Dim.y, Entity->Dim.z), Vec3(0.0f, -1.0f, 0.0f), tTest))
										{
											if (tTest < MaxDistance)
											{
												bCanJump = true;
												break;
											}
										}
										
										if (IntersectRectRay(VoxelAABB, Entity->Pos + 0.5f*Vec3(-Entity->Dim.x, -Entity->Dim.y, -Entity->Dim.z), Vec3(0.0f, -1.0f, 0.0f), tTest))
										{
											if (tTest < MaxDistance)
											{
												bCanJump = true;
												break;
											}
										}
									}
								}
							}
						}
						
						if (bCanJump)
						{
							Entity->Velocity.y = GameState->HeroJumpPower;
						}
					}

					if (Entity->bChangeColorAnimation)
					{
						if (Entity->TimeToChangeColorCurrent < Entity->TimeToChangeColor)
						{
							Entity->Color = Lerp(Entity->Color, Entity->TargetColor, Entity->TimeToChangeColorCurrent / Entity->TimeToChangeColor);
							Entity->TimeToChangeColorCurrent += GameInput->dt;
						}
						else
						{
							Entity->Color = Entity->TargetColor;
							Entity->TimeToChangeColorCurrent = Entity->TimeToChangeColor = 0.0f;
							Entity->bChangeColorAnimation = false;
						}
					}
					
					Entity->PrevLampOffset = Entity->LampOffset;
					if (HeroControl.bUseLamp)
					{
						Entity->LampOffset.z -= 0.15f;
					}
					Entity->LampOffset += 15.0f * (Entity->LampBaseOffset - Entity->LampOffset) * GameInput->dt;
					
					SMoveEntityInfo MoveInfo = { !EngineState->bFlyMode, EngineState->bFlyMode, true, true, HeroControl.Acceleration };
					MoveEntity(GameState, EngineState, Entity, Level, MoveInfo, GameInput->dt);

					Entity->PointLight.Color.rgb = Entity->Color;
					Entity->PointLight.Radius = 3.0f;
					Entity->PointLight.Pos = (0.2f * Entity->LampOffset.x * Entity->Dim.x * Camera->Right) + (0.3f * Entity->Dim.z * Camera->Dir) + (0.2f * Entity->LampOffset.y * Camera->Up);
				} break;
				
				case Entity_Door:
				{
					Entity->Pos = Entity->BasePos + Min(Entity->TimeToMoveCurrent / Entity->TimeToMove, 1.0f) * Entity->TargetOffset;

					if (!Entity->bForceClose)
					{
						if (IsEqual(Entity->CurrentColor, Entity->TargetColor))
						{
							Entity->TimeToMoveCurrent += GameInput->dt;
						}
						else if (Entity->bGoBack)
						{
							Entity->TimeToMoveCurrent -= GameInput->dt;
						}
					}
					else
					{
						Entity->TimeToMoveCurrent -= GameInput->dt;
					}
					
					Entity->TimeToMoveCurrent = Clamp(Entity->TimeToMoveCurrent, 0.0f, Entity->TimeToMove);
				} break;
				
				case Entity_Turret:
				{
					if (Entity->TimeToShootCurrent < Entity->TimeToShoot)
					{
						Entity->TimeToShootCurrent += GameInput->dt;
					}
					else
					{
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
						
						Entity->TimeToMoveCurrent += GameInput->dt;
					}
					else
					{
						Entity->bRemoved = true;
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
					bool bTargetMainHub = CompareStrings(Entity->TargetLevelName, "MainHub");
				#if ENGINE_RELEASE
					if (bTargetMainHub)
					{
						SReadEntireFileResult MainHubSaveFile = ReadEntireFile("Saves\\MainHubSaved.ctl");
						if (MainHubSaveFile.Memory && MainHubSaveFile.Size)
						{
							uint8_t* EndPointer = LoadLevel(EngineState, &EngineState->LevelBaseState, MainHubSaveFile, "Levels\\MainHub.ctl", Level->Entities[0].Velocity);

							EndPointer += sizeof(float);
							EndPointer += sizeof(float);
							EndPointer += sizeof(uint32_t);
							EndPointer += sizeof(EngineState->TextsToRender);

							memcpy(&GameState->LastBaseLevelPos, EndPointer, sizeof(vec3));
							EndPointer += sizeof(vec3);
							memcpy(&GameState->LastBaseLevelGatesAngle, EndPointer, sizeof(float));
							EndPointer += sizeof(float);

							free(MainHubSaveFile.Memory);
						}
						else
						{
							char LevelName[264] = {};
							ConcStrings(LevelName, sizeof(LevelName), Entity->TargetLevelName, ".ctl");
							LoadLevel(EngineState, &EngineState->LevelBaseState, LevelName, true, Level->Entities[0].Velocity);
						}
					}
					else
				#endif
					{
						char LevelName[264] = {};
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
								char LevelName[264] = {};
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
	}
	
	END_PROFILER_BLOCK("ENTITIES_SIMULATION");
	
	if (!GameState->bDeathAnimation && Length(Level->Entities[0].Pos - Level->Entities[0].PrevPos) > 0.00001f)
	{
		Camera->Pos = Level->Entities[0].Pos + Camera->OffsetFromPlayer;
	}

	if (GameState->bDeathAnimation && !EngineState->bReloadLevel && !EngineState->bReloadLevelEditor)
	{
		vec3 MoveDelta = GameState->DeathAnimationSpeed * GameInput->dt * NormalizeSafe0(GameState->DeathAnimationTargetPos - GameState->DeathPos);
		GameState->DeathAnimationLengthMoved += Length(MoveDelta);
		Camera->Pos += MoveDelta;
		
		if ((Length((Camera->Pos - Camera->OffsetFromPlayer) - GameState->DeathAnimationTargetPos) < 0.07f) ||
			(GameState->DeathAnimationLengthMoved >= Length(GameState->DeathAnimationTargetPos - GameState->DeathPos)))
		{
			GameState->bDeathAnimation = false;
			GameState->DeathAnimationLengthMoved = 0.0f;

			Level->Entities[0].Pos = GameState->LastCheckpointPos;
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
				snprintf(ResolutionText, sizeof(ResolutionText), "%d", EngineState->LastFullscreenInternalHeight);
	
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
		}

		if ((bSelected && MenuState->bArrowUsed) || (bSelected && bMouseInItem && MenuState->bMouseLeftReleased))
		{
			switch (MenuElement)
			{
				case MenuElement_Fullscreen:
				{
					PlatformChangeFullscreen(!PlatformGetFullscreen());

					if (PlatformGetFullscreen())
					{
						EngineState->NewInternalWidth = EngineState->LastFullscreenInternalWidth;
						EngineState->NewInternalHeight = EngineState->LastFullscreenInternalHeight;
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
						// TODO(georgii): This is just for test. Need to proper get all available resolution for a monitor and use them!
						uint32_t Width = Vulkan->InternalWidth;
						uint32_t Height = Vulkan->InternalHeight;
						if (MenuState->bArrowRight || (!MenuState->bArrowRight && !MenuState->bArrowLeft))
						{
							if (Height == 1080)
							{
								Width = 1280;
								Height = 720;
							}
							else
							{
								Assert(Height == 720);

								Width = 1920;
								Height = 1080;
							}
						}
						else
						{
							if (Height == 1080)
							{
								Width = 1280;
								Height = 720;
							}
							else
							{
								Assert(Height == 720);

								Width = 1920;
								Height = 1080;
							}
						}

						EngineState->NewInternalWidth = Width;
						EngineState->NewInternalHeight = Height;
						EngineState->bSwapchainChanged = true;
					}
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
				else if (CompareStrings(Item.Name, "HeroJumpPower"))
				{
					GameState->HeroJumpPower = Item.Value;
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
			GameState->HeroJumpPower = 4.5f;
			GameState->HeroLampDistance = 8.0f;
		}

        GameState->DeathAnimationSpeed = 10.0f;

		GameState->MenuState.OpenAnimationTime = 1.0f;
		GameState->MenuState.SelectedStayBrightTime = 1.0f;
		GameState->MenuState.SelectedAnimationTime = 2.0f;

#if ENGINE_RELEASE
		SReadEntireFileResult GeneralSaveFile = ReadEntireFile("Saves\\GeneralSave");
		if (GeneralSaveFile.Memory && GeneralSaveFile.Size)
		{
			uint8_t* SaveFilePointer = (uint8_t*) GeneralSaveFile.Memory;

			char LastLevelName[260];
			uint32_t LastLevelNameLength = 0;
			memcpy(&LastLevelNameLength, SaveFilePointer, sizeof(uint32_t));
			SaveFilePointer += sizeof(uint32_t);
			memcpy(LastLevelName, SaveFilePointer, LastLevelNameLength);
			SaveFilePointer += LastLevelNameLength;

			bool bFullscreen;
			memcpy(&bFullscreen, SaveFilePointer, sizeof(bool));
			SaveFilePointer += sizeof(bool);

			bool bVSync;
			memcpy(&bVSync, SaveFilePointer, sizeof(bool));
			SaveFilePointer += sizeof(bool);

			bool bVignetteEnabled;
			memcpy(&bVignetteEnabled, SaveFilePointer, sizeof(bool));
			SaveFilePointer += sizeof(bool);

			int32_t AOQuality;
			memcpy(&AOQuality, SaveFilePointer, sizeof(int32_t));
			SaveFilePointer += sizeof(int32_t);

			uint32_t InternalWidth;
			memcpy(&InternalWidth, SaveFilePointer, sizeof(uint32_t));
			SaveFilePointer += sizeof(uint32_t);

			uint32_t InternalHeight;
			memcpy(&InternalHeight, SaveFilePointer, sizeof(uint32_t));
			SaveFilePointer += sizeof(uint32_t);

			uint64_t WindowPlacementSize;
			memcpy(&WindowPlacementSize, SaveFilePointer, sizeof(uint64_t));
			SaveFilePointer += sizeof(uint64_t);
			SaveFilePointer += WindowPlacementSize;

			PlatformChangeVSync(bVSync);
			EngineState->bVignetteEnabled = bVignetteEnabled;
			EngineState->Renderer.AOQuality = EAOQuality(AOQuality);
			EngineState->LastFullscreenInternalWidth = InternalWidth;
			EngineState->LastFullscreenInternalHeight = InternalHeight;

			if (CompareStrings(LastLevelName, "Levels\\MainHub.ctl"))
			{
				SReadEntireFileResult MainHubSaveFile = ReadEntireFile("Saves\\MainHubSaved.ctl");
				if (MainHubSaveFile.Memory && MainHubSaveFile.Size)
				{
					uint8_t* EndPointer = LoadLevel(EngineState, &EngineState->LevelBaseState, MainHubSaveFile, "Levels\\MainHub.ctl");

					memcpy(&EngineState->Camera.Pitch, EndPointer, sizeof(float));
					EndPointer += sizeof(float);
					memcpy(&EngineState->Camera.Head, EndPointer, sizeof(float));
					EndPointer += sizeof(float);

					memcpy(&EngineState->TextsToRenderCount, EndPointer, sizeof(uint32_t));
					EndPointer += sizeof(uint32_t);
					memcpy(EngineState->TextsToRender, EndPointer, sizeof(EngineState->TextsToRender));
					EndPointer += sizeof(EngineState->TextsToRender);

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

					memcpy(&GameState->LastBaseLevelPos, EndPointer, sizeof(vec3));
					EndPointer += sizeof(vec3);
					memcpy(&GameState->LastBaseLevelGatesAngle, EndPointer, sizeof(float));
					EndPointer += sizeof(float);

					free(MainHubSaveFile.Memory);
				}
			}
			else
			{
				LoadLevel(EngineState, &EngineState->LevelBaseState, LastLevelName, false);
			}
		}
		else
		{
			LoadLevel(EngineState, &EngineState->LevelBaseState, "MainHub.ctl");
		}
#else
		LoadLevel(EngineState, &EngineState->LevelBaseState, "MainHub.ctl");
#endif

		GameState->bInitialized = true;
	}

	if (GameState->bReload)
	{
		GameState->bDeathAnimation = false;
		GameState->DeathAnimationLengthMoved = 0.0f;
		GameState->DeathPos = Vec3(0.0f);

		GameState->CurrentCheckpointIndex = 0;
		GameState->LastCheckpointPos = EngineState->Level.Entities[0].Pos;

		GameState->bReload = false;
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

	char CurrentLevelName[ArrayCount(EngineState->LevelName)];
	memcpy(CurrentLevelName, EngineState->LevelName, ArrayCount(CurrentLevelName));

	if (!EngineState->bMenuOpened && !MenuState->bDisableStarted)
	{
		UpdateGameMode(GameState, EngineState, GameInput, CurrentLevelName);	
	}
	else
	{
		UpdateMenu(MenuState, EngineState, GameInput, Vulkan);
	}
	
#if ENGINE_RELEASE
	if (CompareStrings(CurrentLevelName, "Levels\\MainHub.ctl"))
	{
		FILE* MainHubSaveFile = fopen("Saves\\MainHubSaved.ctl", "wb");
		if (MainHubSaveFile)
		{
			// Save level
			const uint32_t FileVersion = LEVEL_MAX_FILE_VERSION;
			fwrite(&FileVersion, sizeof(uint32_t), 1, MainHubSaveFile);
			fwrite((void*) &EngineState->Level, sizeof(SLevel), 1, MainHubSaveFile);

			// Save some camera info
			fwrite(&EngineState->Camera.Pitch, sizeof(float), 1, MainHubSaveFile);
			fwrite(&EngineState->Camera.Head, sizeof(float), 1, MainHubSaveFile);

			// Save text stuff
			fwrite(&EngineState->TextsToRenderCount, sizeof(uint32_t), 1, MainHubSaveFile);
			fwrite(EngineState->TextsToRender, sizeof(EngineState->TextsToRender), 1, MainHubSaveFile);

			// Save last base level stuff
			fwrite(&GameState->LastBaseLevelPos, sizeof(vec3), 1, MainHubSaveFile);
			fwrite(&GameState->LastBaseLevelGatesAngle, sizeof(float), 1, MainHubSaveFile);

			fclose(MainHubSaveFile);
		}
	}

	FILE* GeneralSaveFile = fopen("Saves\\GeneralSave", "wb");
	if (GeneralSaveFile)
	{
		const uint32_t LevelNameLength = StringLength(CurrentLevelName) + 1;
		fwrite(&LevelNameLength, sizeof(uint32_t), 1, GeneralSaveFile);
		fwrite(CurrentLevelName, LevelNameLength, 1, GeneralSaveFile);

		bool bFullscreen = PlatformGetFullscreen();
		bool bVSync = PlatformGetVSync();
		bool bVignetteEnabled = EngineState->bVignetteEnabled;
		int32_t AOQuality = EngineState->Renderer.AOQuality;
		uint32_t Width = EngineState->LastFullscreenInternalWidth;
		uint32_t Height = EngineState->LastFullscreenInternalHeight;

		fwrite(&bFullscreen, sizeof(bool), 1, GeneralSaveFile);
		fwrite(&bVSync, sizeof(bool), 1, GeneralSaveFile);
		fwrite(&bVignetteEnabled, sizeof(bool), 1, GeneralSaveFile);
		fwrite(&AOQuality, sizeof(int32_t), 1, GeneralSaveFile);
		fwrite(&Width, sizeof(uint32_t), 1, GeneralSaveFile);
		fwrite(&Height, sizeof(uint32_t), 1, GeneralSaveFile);

		SWindowPlacementInfo WindowPlacement = PlatformGetWindowPlacement();
		fwrite(&WindowPlacement.InfoSizeInBytes, sizeof(uint64_t), 1, GeneralSaveFile);
		fwrite(WindowPlacement.InfoPointer, WindowPlacement.InfoSizeInBytes, 1, GeneralSaveFile);

		fclose(GeneralSaveFile);
	}
#endif
}