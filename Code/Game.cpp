void UpdateGameMode(SGameState* GameState, const SGameInput* GameInput)
{
	SCamera* Camera = &GameState->Camera;
	SLevel* Level = &GameState->Level;
	SAudioState* AudioState = &GameState->AudioState;

	if (WasDown(GameInput->Buttons[Button_R]))
	{
		ReloadGameLevel(GameState);
		GameState->CurrentCheckpointIndex = 0;
	}

	Camera->PrevDir = Camera->Dir;
	Camera->PrevRight = Camera->Right;
	Camera->PrevUp = Camera->Up;
	
	Camera->PrevPitch = Camera->Pitch;
	Camera->PrevHead = Camera->Head;
	
	Camera->Pitch -= 0.1f*GameInput->MouseDeltaY;
	if (GameState->bFlyMode)
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
		if (GameState->bFlyMode) HeroControl.Acceleration = Camera->Dir;
	}
	if (GameInput->Buttons[Button_S].IsDown)
	{
		HeroControl.Acceleration -= Vec3(MovementDir.x, 0.0f, MovementDir.z);
		if (GameState->bFlyMode) HeroControl.Acceleration = -Camera->Dir;
	} 
	if (GameInput->Buttons[Button_D].IsDown)
	{
		HeroControl.Acceleration += Vec3(Camera->Right.x, 0.0f, Camera->Right.z);
		if (GameState->bFlyMode) HeroControl.Acceleration = Camera->Right;
	} 
	if (GameInput->Buttons[Button_A].IsDown)
	{
		HeroControl.Acceleration -= Vec3(Camera->Right.x, 0.0f, Camera->Right.z);
		if (GameState->bFlyMode) HeroControl.Acceleration = -Camera->Right;
	}

#ifndef ENGINE_RELEASE
	if (WasDown(GameInput->Buttons[Button_F]))
	{
		GameState->bFlyMode = !GameState->bFlyMode;
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
			ReloadGameLevel(GameState);

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
				Rect TestEntityAABB = RectCenterDimOrientation(TestEntity->Pos, TestEntity->Dim, EulerToQuat(TestEntity->Orientation.xyz));
				
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
		
		if (bVoxelHitFirst)
		{
			uint32_t X = RaytraceVoxelsResult.X;
			uint32_t Y = RaytraceVoxelsResult.Y;
			uint32_t Z = RaytraceVoxelsResult.Z;
			
			if (IsVoxelDestroyable(Level->Voxels, RaytraceVoxelsResult.X, RaytraceVoxelsResult.Y, RaytraceVoxelsResult.Z))
			{
				MarkVoxelForDeletion(GameState, X, Y, Z);
				
				AddParticleEmitterBurst(Level->ParticleEmitters, Level->ParticleEmitterCount, true, true, 
										VoxelDim * Vec3i(X, Y, Z) + 0.5f * Vec3(VoxelDim), -0.4f * Vec3(VoxelDim), 0.4f * Vec3(VoxelDim), 0.15f, 0.2f, 
										Vec3(0.0f), Vec3(0.0f), GetVoxelColorVec3(Level->Voxels, X, Y, Z), GetVoxelColorVec3(Level->Voxels, X, Y, Z), 
										0.15f, 0.2f, 8);
			}
		}
		else if (HitEntity)
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
					
					SMoveEntityInfo MoveInfo = { !GameState->bFlyMode, GameState->bFlyMode, true, true, HeroControl.Acceleration };
					MoveEntity(GameState, Entity, Level, MoveInfo, GameInput->dt);

					Entity->PointLight.Color.rgb = Entity->Color;
					Entity->PointLight.Radius = 2.0f;
					Entity->PointLight.Pos = Vec3(0.0f);//(Entity->LampOffset.x * Entity->Dim.x * Camera->Right) + (0.3f * Entity->Dim.z * Camera->Dir) + (Entity->LampOffset.y * Camera->Up);
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
						MoveEntity(GameState, Entity, Level, MoveInfo, GameInput->dt);
						
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
					char LevelName[264] = {};
					ConcStrings(LevelName, sizeof(LevelName), Entity->TargetLevelName, ".ctl");
					LoadLevel(GameState, &GameState->LevelBaseState, LevelName, Level->Entities[0].Velocity);

					SEntity* HeroEntity = &GameState->LevelBaseState.Entities[0];

					float GatesAngleDifference = 0.0f;
					bool bTargetMainHub = CompareStrings(Entity->TargetLevelName, "MainHub");
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
					Rect GatesAABB = RectCenterDimOrientation(Entity->Pos, Entity->Dim, EulerToQuat(Entity->Orientation.xyz));
					
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

	if (GameState->bDeathAnimation && !GameState->bReloadLevel && !GameState->bReloadLevelEditor)
	{
		vec3 MoveDelta = GameState->DeathAnimationSpeed * GameInput->dt * NormalizeSafe0(GameState->DeathAnimationTargetPos - GameState->DeathPos);
		GameState->DeathAnimationLengthMoved += Length(MoveDelta);
		Camera->Pos += MoveDelta;
		
		if ((Length((Camera->Pos - Camera->OffsetFromPlayer) - GameState->DeathAnimationTargetPos) < 0.07f) ||
			(GameState->DeathAnimationLengthMoved >= Length(GameState->DeathAnimationTargetPos - GameState->DeathPos)))
		{
			GameState->bDeathAnimation = false;
			GameState->DeathAnimationLengthMoved = 0.0f;
		}
	}
}

void UpdateMenuDefault(SGameState* GameState, const SGameInput* GameInput)
{
	AddTextMenu(GameState, "COLOR THIEF", Vec2(0.0f, 0.7f), 0.4f, &GameState->Renderer.KarminaBold, Vec4(1.0f));
	for (int32_t MenuElement = MenuElement_DefaultNone; MenuElement < MenuElement_DefaultCount; MenuElement++)
	{
		bool bSelected = (GameState->SelectedMenuElement == MenuElement);
		bool bEnterDown = WasDown(GameInput->Buttons[Button_Enter]);

		const SFont* Font = &GameState->Renderer.KarminaRegular;
		float Scale = 0.2f;
		vec4 Color = bSelected ? Vec4(0.8f, 0.8f, 0.8f, 1.0f) : Vec4(0.5f, 0.5f, 0.5f, 1.0f);
		switch (MenuElement)
		{
			case MenuElement_Settings:
			{
				AddTextMenu(GameState, "Settings", Vec2(0.0f, 0.12f), Scale, Font, Color);
			} break;

			case MenuElement_StartNewGame:
			{
				AddTextMenu(GameState, "Start A New Game", Vec2(0.0f, 0.0f), Scale, Font, Color);
			} break;

			case MenuElement_Quit:
			{
				AddTextMenu(GameState, "Quit", Vec2(0.0f, -0.12f), Scale, Font, Color);
			} break;
		}

		if (bSelected && bEnterDown)
		{
			switch (MenuElement)
			{
				case MenuElement_Settings:
				{
					GameState->MenuMode = MenuMode_Settings;
				} break;

				case MenuElement_StartNewGame:
				{
					LoadLevel(GameState, &GameState->LevelBaseState, "MainHub.ctl");
					GameState->bMenuOpened = false;
				} break;

				case MenuElement_Quit:
				{
					PlatformQuitGame();
				} break;
			}
		}
	}
}

void UpdateMenuSettings(SGameState* GameState, const SGameInput* GameInput, SVulkanContext* Vulkan)
{
	AddTextMenu(GameState, "SETTINGS", Vec2(0.0f, 0.7f), 0.4f, &GameState->Renderer.KarminaBold, Vec4(1.0f));
	for (int32_t MenuElement = MenuElement_SettingsNone; MenuElement < MenuElement_SettingsCount; MenuElement++)
	{
		bool bSelected = (GameState->SelectedMenuElement == MenuElement);

		bool bArrowLeft = WasDown(GameInput->Buttons[Button_A]) || WasDown(GameInput->Buttons[Button_ArrowLeft]);
		bool bArrowRight = WasDown(GameInput->Buttons[Button_D]) || WasDown(GameInput->Buttons[Button_ArrowRight]);
		bool bArrowUsed = bArrowLeft || bArrowRight;

		const SFont* Font = &GameState->Renderer.KarminaRegular;
		const float LeftPos = -0.5f;
		const float RightPos = 0.5f;
		const float Scale = 0.15f;
		const vec4 Color = bSelected ? Vec4(0.8f, 0.8f, 0.8f, 1.0f) : Vec4(0.5f, 0.5f, 0.5f, 1.0f);
		switch (MenuElement)
		{
			case MenuElement_Fullscreen:
			{
				const float YPos = 0.24f;
				AddTextMenu(GameState, "Fullscreen:", Vec2(LeftPos, YPos), Scale, Font, Color, TextAlignment_Left);
				AddTextMenu(GameState, PlatformGetFullscreen() ? "Yes" : "No", Vec2(RightPos, YPos), Scale, Font, Color, TextAlignment_Right);
			} break;

			case MenuElement_VSync:
			{
				const float YPos = 0.12f;
				AddTextMenu(GameState, "V-Sync:", Vec2(LeftPos, YPos), Scale, Font, Color, TextAlignment_Left);
				AddTextMenu(GameState, PlatformGetVSync() ? "Yes" : "No", Vec2(RightPos, YPos), Scale, Font, Color, TextAlignment_Right);
			} break;

			case MenuElement_Resolution:
			{
				const float YPos = 0.0f;
				AddTextMenu(GameState, "Resolution:", Vec2(LeftPos, YPos), Scale, Font, Color, TextAlignment_Left);
				AddTextMenu(GameState, "1920x1080", Vec2(RightPos, YPos), Scale, Font, Color, TextAlignment_Right);
			} break;

			case MenuElement_Vignetting:
			{
				const float YPos = -0.12f;
				AddTextMenu(GameState, "Vignetting:", Vec2(LeftPos, YPos), Scale, Font, Color, TextAlignment_Left);
				AddTextMenu(GameState, GameState->bVignetteEnabled ? "Yes" : "No", Vec2(RightPos, YPos), Scale, Font, Color, TextAlignment_Right);
			} break;

			case MenuElement_Multisampling:
			{
				const float YPos = -0.24f;
				AddTextMenu(GameState, "Multisampling:", Vec2(LeftPos, YPos), Scale, Font, Color, TextAlignment_Left);

				char MultisamplingText[32] = {};
				const char ValueMSAA[2] = { char(Vulkan->SampleCountMSAA + '0'), '\0' };
				ConcStrings(MultisamplingText, ArrayCount(MultisamplingText), ValueMSAA, "x MSAA");

				AddTextMenu(GameState, MultisamplingText, Vec2(RightPos, YPos), Scale, Font, Color, TextAlignment_Right);
			} break;
		}

		if (bSelected && bArrowUsed)
		{
			switch (MenuElement)
			{
				case MenuElement_Fullscreen:
				{
					PlatformChangeFullscreen(!PlatformGetFullscreen());
				} break;

				case MenuElement_VSync:
				{
					PlatformChangeVSync(!PlatformGetVSync());
				} break;

				case MenuElement_Resolution:
				{
				} break;

				case MenuElement_Vignetting:
				{
					GameState->bVignetteEnabled = !GameState->bVignetteEnabled;
				} break;

				case MenuElement_Multisampling:
				{
					uint32_t MaxSampleCountMSAA = uint32_t(Vulkan->MaxSampleCountMSAA);
					uint32_t SampleCountMSAA = uint32_t(Vulkan->SampleCountMSAA);

					if (bArrowRight)
					{
						SampleCountMSAA *= 2;
						if (SampleCountMSAA > MaxSampleCountMSAA)
						{
							SampleCountMSAA = 1;
						}
					}
					else
					{
						SampleCountMSAA /= 2;
						if (SampleCountMSAA == 0)
						{
							SampleCountMSAA = MaxSampleCountMSAA;
						}
					}

					GameState->bSampleCountMSAAChanged = true;
					GameState->NewSampleCountMSAA = SampleCountMSAA;
				} break;
			}
		}
	}

	if (WasDown(GameInput->Buttons[Button_Escape]))
	{
		GameState->MenuMode = MenuMode_Default;
		GameState->SelectedMenuElement = MenuElement_Settings;
	}
}

void UpdateMenu(SGameState* GameState, const SGameInput* GameInput, const SVulkanContext* Vulkan)
{
	EMenuElement MinMenuElement = (GameState->MenuMode == MenuMode_Default) ? MenuElement_DefaultNone : MenuElement_SettingsNone;
	EMenuElement MaxMenuElement = (GameState->MenuMode == MenuMode_Default) ? MenuElement_DefaultCount : MenuElement_SettingsCount;

	if (WasDown(GameInput->Buttons[Button_W]) || WasDown(GameInput->Buttons[Button_ArrowUp]) ||
		WasDown(GameInput->Buttons[Button_S]) || WasDown(GameInput->Buttons[Button_ArrowDown]))
	{
		if (WasDown(GameInput->Buttons[Button_W]) || WasDown(GameInput->Buttons[Button_ArrowUp]))
		{
			GameState->SelectedMenuElement = EMenuElement(GameState->SelectedMenuElement - 1);

			if (GameState->SelectedMenuElement == MinMenuElement)
			{
				GameState->SelectedMenuElement = EMenuElement(MaxMenuElement - 1);
			}
		}
		else
		{
			GameState->SelectedMenuElement = EMenuElement((GameState->SelectedMenuElement + 1) % MaxMenuElement);
		}

		GameState->SelectedMenuElement = EMenuElement(Clamp(GameState->SelectedMenuElement, MinMenuElement + 1, MaxMenuElement - 1));
	}

	switch (GameState->MenuMode)
	{
		case MenuMode_Default:
		{
			UpdateMenuDefault(GameState, GameInput);
		} break;

		case MenuMode_Settings:
		{
			UpdateMenuSettings(GameState, GameInput, (SVulkanContext*) Vulkan);
		} break;
	}
	
}

void UpdateGame(SGameState* GameState, const SGameInput* GameInput, const SVulkanContext* Vulkan)
{
	if (WasDown(GameInput->Buttons[Button_Escape]))
	{
		if (!GameState->bMenuOpened || (GameState->MenuMode != MenuMode_Settings))
		{
			GameState->bMenuOpened = !GameState->bMenuOpened;
			GameState->MenuMode = MenuMode_Default;
			GameState->SelectedMenuElement = MenuElement_DefaultNone;
		}
	}

	if (!GameState->bMenuOpened)
	{
		UpdateGameMode(GameState, GameInput);	
	}
	else
	{
		UpdateMenu(GameState, GameInput, Vulkan);
	}
}