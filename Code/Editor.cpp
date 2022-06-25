#ifndef ENGINE_RELEASE
#include "Editor.h"

void SaveLevelHistory(SEditorState* EditorState, const SLevel* Level)
{
	uint32_t HeadNextIndex = (EditorState->LevelHistoryHead + 1) % ArrayCount(EditorState->LevelHistory);
	if (HeadNextIndex == EditorState->LevelHistoryTail)
	{
		EditorState->LevelHistoryTail = (EditorState->LevelHistoryTail + 1) % ArrayCount(EditorState->LevelHistory);
	}
    
	EditorState->LevelHistory[EditorState->LevelHistoryHead] = *Level;
	EditorState->LevelHistoryHead = HeadNextIndex;
}

void SaveLevelEditor(const SLevel& Level, const char* Path)
{
	const uint32_t FileVersion = LEVEL_MAX_FILE_VERSION;

	FILE* File = fopen(Path, "wb");
	Assert(File);
	if (File)
	{
		fwrite(&FileVersion, sizeof(uint32_t), 1, File);
		fwrite((void*) &Level, sizeof(SLevel), 1, File);
		fclose(File);
	}
}

bool EditorDragFloat(SEditorState* EditorState, const char* Name, float* Ptr, float Speed, float Min = 0.0f, float Max = 0.0f)
{
	bool bValueChanged = ImGui::DragFloat(Name, Ptr, Speed, Min, Max);
	EditorState->bIsImguiWindowHovered |= ImGui::IsItemActive();

	return bValueChanged;
}

bool EditorDragFloat2(SEditorState* EditorState, const char* Name, float* Ptr, float Speed)
{
	bool bValueChanged = ImGui::DragFloat2(Name, Ptr, Speed);
	EditorState->bIsImguiWindowHovered |= ImGui::IsItemActive();

	return bValueChanged;
}

bool EditorDragFloat3(SEditorState* EditorState, const char* Name, float* Ptr, float Speed)
{
	bool bValueChanged = ImGui::DragFloat3(Name, Ptr, Speed);
	EditorState->bIsImguiWindowHovered |= ImGui::IsItemActive();

	return bValueChanged;
}

bool EditorInputU32(SEditorState* EditorState, const char* Name, uint32_t* Ptr, ImGuiInputTextFlags AdditionalFlags = 0)
{
	bool bValueChanged = ImGui::InputScalar(Name, ImGuiDataType_U32, Ptr, 0, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue | AdditionalFlags);
	EditorState->bIsImguiWindowHovered |= ImGui::IsItemActive();

	return bValueChanged;
}

bool EditorInputU8(SEditorState* EditorState, const char* Name, uint8_t* Ptr, ImGuiInputTextFlags AdditionalFlags = 0)
{
	bool bValueChanged = ImGui::InputScalar(Name, ImGuiDataType_U8, Ptr, 0, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue | AdditionalFlags);
	EditorState->bIsImguiWindowHovered |= ImGui::IsItemActive();

	return bValueChanged;
}

bool EditorColorEdit3(SEditorState* EditorState, const char* Name, vec3* Ptr)
{
	bool bValueChanged = ImGui::ColorEdit3(Name, &Ptr->x);
	*Ptr = Clamp(*Ptr, 0.0f, 1.0f);
	EditorState->bIsImguiWindowHovered |= ImGui::IsItemActive();

	return bValueChanged;
}

bool EditorInputText(SEditorState* EditorState, const char* Name, char* Buffer, size_t BufferSize)
{
	bool bEnter = ImGui::InputText(Name, Buffer, BufferSize, ImGuiInputTextFlags_EnterReturnsTrue);
	if (bEnter)
	{
		// NOTE(georgii): Disable focus of this window, so the camera can move.
		// NOTE(georgii): This function was added by me, I'm not sure if it is good for DearImGui, but it seems to work.
		ImGui::DisableFocus();
	}
	EditorState->bIsImguiWindowFocused |= ImGui::IsItemFocused();

	return bEnter;
}

void RenderDearImgui(SEngineState* EngineState, const SVulkanContext* Vulkan, VkFramebuffer Framebuffer)
{
	SEditorState* EditorState = &EngineState->EditorState;
	EditorState->bIsImguiWindowHovered = false;
	EditorState->bIsImguiWindowFocused = false;
    
	VkRenderPassBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	BeginInfo.renderPass = Vulkan->ImguiRenderPass;
    BeginInfo.framebuffer = Framebuffer;
    BeginInfo.renderArea.extent.width = Vulkan->Width;
    BeginInfo.renderArea.extent.height = Vulkan->Height;
	vkCmdBeginRenderPass(Vulkan->CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
	if (EngineState->EngineMode == EngineMode_Game)
	{
		if (ImGui::Begin("Mode", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			if (ImGui::Button("Editor"))
			{
				EngineState->bReloadLevelEditor = true;
				EngineState->bFlyMode = false;
				
				EngineState->EngineMode = EngineMode_Editor;

				EngineState->TextsToRenderCount = 0;

				EngineState->bMenuOpened = false;
			}
            
			ImGui::SetWindowPos(ImVec2(0, 0), true);
			EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
		}
		ImGui::End();
        
		if (ImGui::Begin("DebugVars", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::Checkbox("Fly mode", &EngineState->bFlyMode);
            
			ImVec2 ImguiWindowSize = ImGui::GetWindowSize();
			ImGui::SetWindowPos(ImVec2(Vulkan->Width - ImguiWindowSize.x, 0), true);
			EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
		}
		ImGui::End();
	}
	else if (EngineState->EngineMode == EngineMode_Editor)
	{
		ImVec2 LevelsWindowSize = {};
		if (ImGui::Begin("Levels", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			if (ImGui::CollapsingHeader("Levels"))
			{
				const uint32_t MaxLevelCount = 64;
				const uint32_t MaxFilenameLength = 264;
				char LevelNames[MaxLevelCount][MaxFilenameLength] = {};
				
				uint32_t LevelCount = 0;
				PlatformGetAllFilenamesFromDir("Levels\\*.ctl", (char*)LevelNames, MaxFilenameLength, LevelCount);
				Assert(LevelCount <= MaxLevelCount);
                
				bool bNewLevelCreated = false;
            	static char NewLevelName[MaxFilenameLength] = ""; 
				if (EditorInputText(EditorState, "New level", NewLevelName, ArrayCount(NewLevelName)))
				{
					if (StringLength(NewLevelName) > 0)
					{
						ConcStrings(NewLevelName, ArrayCount(NewLevelName), NewLevelName, ".ctl");
                        
						bool bFound = false;
						for (uint32_t I = 0; I < LevelCount; I++)
						{
							if (CompareStrings(NewLevelName, LevelNames[I]))
							{
								bFound = true;
								break;
							}
						}
                        
						if (!bFound)
						{
							char NewLevelPath[MaxFilenameLength] = {};
							ConcStrings(NewLevelPath, ArrayCount(NewLevelPath), "Levels\\", NewLevelName);
                            							
							uint32_t ZCenter = LevelDimZ / 2;
							uint32_t XCenter = LevelDimX / 2;
							for (uint32_t Z = ZCenter - 7; Z < ZCenter + 7; Z++)
							{
								for (uint32_t Y = 0; Y < 1; Y++)
								{
									for (uint32_t X = XCenter - 7; X < XCenter + 7; X++)
									{
										SetVoxelActive(EditorState->NewLevel.Voxels, X, Y, Z, true);
										SetVoxelColor(EditorState->NewLevel.Voxels, X, Y, Z, 77, 77, 77);
									}	
								}	
							}

							EditorState->NewLevel.AmbientColor = Vec3(15.0f / 255.0f, 30.0f / 255.0f, 60.0f / 255.0f);
							EditorState->NewLevel.AmbientConstant = Vec3(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f);
                            
							AddHero(EditorState->NewLevel, Vec3(XCenter * VoxelDim, 1.5f, ZCenter * VoxelDim));
							EngineState->Camera.Pos = Vec3(XCenter * VoxelDim, 1.5f, ZCenter * VoxelDim) + Vec3(5.0f, 6.0f, -5.0f);
							EngineState->Camera.Pitch = -45.0f;
							EngineState->Camera.Head = -45.0f;

							SaveLevelEditor(EditorState->NewLevel, NewLevelPath);
							LoadLevel(EngineState, &EngineState->LevelBaseState, NewLevelName);
							NewLevelName[0] = 0;
							EditorState->NewLevel = {};

							bNewLevelCreated = true;
						}
					}
				}
				EditorState->bIsImguiWindowFocused |= ImGui::IsItemFocused();
				
				if (ImGui::BeginListBox(""))
				{
					for (uint32_t I = 0; I < LevelCount; I++)
					{
						if (ImGui::Selectable(LevelNames[I], false))
						{
							EditorState->SelectedEntity = 0;
							EditorState->SelectedVoxelsCount = 0;
                            
							LoadLevel(EngineState, &EngineState->LevelBaseState, LevelNames[I]);

							Assert(EngineState->LevelBaseState.Entities[0].Type == Entity_Hero);
							EngineState->Camera.Pos = EngineState->LevelBaseState.Entities[0].Pos;
						}
					}
					ImGui::EndListBox();
				}
			}
            
			LevelsWindowSize = ImGui::GetWindowSize();
			ImGui::SetWindowPos(ImVec2(0, 0), true);
			EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
		}
		ImGui::End();
        
		ImVec2 ModeWindowSize = {};
		if (ImGui::Begin("Mode", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::SameLine();
			if (ImGui::Button("SaveLevel"))
			{
				SaveLevelEditor(EngineState->Level, EngineState->LevelName);
				EngineState->LevelBaseState = EngineState->Level;
			}
            
			ImGui::SameLine();
			if (ImGui::Button("ToGame"))
			{
				EditorState->SelectedVoxelsCount = 0;
				EditorState->SelectedVoxelColorFloat = Vec3(0.0f);

				EngineState->LevelGameStartState = EngineState->Level;
				ReloadGameLevel(EngineState);
                
				EngineState->EngineMode = EngineMode_Game;
			}
            
			ModeWindowSize = ImGui::GetWindowSize();
			ImGui::SetWindowPos(ImVec2(LevelsWindowSize.x, 0), true);
			EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
		}
		ImGui::End();
        
		if (ImGui::Begin("Building", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			if (ImGui::Checkbox("Building", &EditorState->bBuildingMode))
			{
				EditorState->WallSetMode = WallSetMode_None;
				EditorState->SelectedVoxelsCount = 0;
				EditorState->SelectedVoxelColorFloat = Vec3(0.0f);
			}
            
			ImGui::SetWindowPos(ImVec2(0, Max(LevelsWindowSize.y, ModeWindowSize.y)), true);
			EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
		}
		ImGui::End();
        
		ImVec2 SelectedVoxelWindowSize = {};
		if (EditorState->SelectedVoxelsCount > 0)
		{
			if (ImGui::Begin("Selected", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
			{
				if (EditorColorEdit3(EditorState, "Color", &EditorState->SelectedVoxelColorFloat))
				{
					uint8_t Red = uint8_t(EditorState->SelectedVoxelColorFloat.r * 255.0f);
					uint8_t Green = uint8_t(EditorState->SelectedVoxelColorFloat.g * 255.0f);
					uint8_t Blue = uint8_t(EditorState->SelectedVoxelColorFloat.b * 255.0f);
                    
					for (uint32_t I = 0; I < EditorState->SelectedVoxelsCount; I++)
					{
						uint32_t ID = EditorState->SelectedVoxels[I];
						uint32_t X = ID % LevelDimX;
						uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
						uint32_t Z = ID / (LevelDimX*LevelDimY);
                        
						MarkVoxelForChangingColor(EngineState, X, Y, Z, Red, Green, Blue);
					}
                    
					if ((EditorState->SelectedVoxelsCount > 0) && !EditorState->bImGuiChangeStarted)
					{
						SaveLevelHistory(EditorState, &EngineState->Level);
                        
						EditorState->bImGuiChangeStarted = true;
						EditorState->ImGuiChangeTimePassed = 0.0f;
					}
				}
                
				SelectedVoxelWindowSize = ImGui::GetWindowSize();
				ImGui::SetWindowPos(ImVec2(Vulkan->Width - SelectedVoxelWindowSize.x, 0), true);
				EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
			}
			ImGui::End();
		}
        
		ImVec2 SelectedObjectWindowSize = {};
		if (EditorState->SelectedEntity || EditorState->SelectedPointLight)
		{
			if (ImGui::Begin("Selected Object", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
			{
				SEntity* Entity = EditorState->SelectedEntity;
				SPointLight* PointLight = EditorState->SelectedPointLight;
                
				bool bValueChanged = false;
				if (Entity)
				{
					bValueChanged |= EditorDragFloat3(EditorState, "Position", &Entity->Pos.x, 0.05f);
					Entity->BasePos = Entity->Pos;
                    
					if ((Entity->Type != Entity_MessageToggler) && (Entity->Type != Entity_Checkpoint))
					{
						if (Entity->Type == Entity_Gates)
						{
							bValueChanged |= EditorDragFloat(EditorState, "Rotation", &Entity->Orientation.y, 0.3f);
						}
						else
						{
							bValueChanged |= EditorDragFloat3(EditorState, "Rotation", &Entity->Orientation.x, 0.3f);
						}
					}
                    
					if (Entity->Type != Entity_Hero)
					{
						bValueChanged |= EditorDragFloat3(EditorState, "Dimension", &Entity->Dim.x, 0.025f);
					}

					if ((Entity->Type != Entity_MessageToggler) && (Entity->Type != Entity_Hero) && (Entity->Type != Entity_Checkpoint))
					{
						bValueChanged |= EditorColorEdit3(EditorState, "Color", &Entity->Color);
					}

					switch (Entity->Type)
					{
						case Entity_Torch:
						{
							bValueChanged |= EditorDragFloat3(EditorState, "LightOffset", &Entity->PointLight.Pos.x, 0.05f);
							bValueChanged |= EditorDragFloat(EditorState, "LightRadius", &Entity->PointLight.Radius, 0.05f, 0.0f, FloatMax);
						} break;
                        
						case Entity_Door:
						{
							bValueChanged |= EditorColorEdit3(EditorState, "CurrentColor", &Entity->CurrentColor);
							bValueChanged |= EditorColorEdit3(EditorState, "TargetColor", &Entity->TargetColor);
                            
							bValueChanged |= EditorDragFloat3(EditorState, "TargetOffset", &Entity->TargetOffset.x, 0.05f);
							bValueChanged |= EditorDragFloat(EditorState, "MoveTime", &Entity->TimeToMove, 0.05f, 0.0f, FloatMax);

							bValueChanged |= ImGui::Checkbox("GoBack", &Entity->bGoBack);
						} break;
                        
						case Entity_Turret:
						{
							bValueChanged |= EditorColorEdit3(EditorState, "FireballColor", &Entity->FireballColor);
							bValueChanged |= EditorDragFloat3(EditorState, "FireballDim", &Entity->FireballDim.x, 0.025f);
							bValueChanged |= EditorDragFloat3(EditorState, "TargetOffset", &Entity->TargetOffset.x, 0.05f);
							bValueChanged |= EditorDragFloat(EditorState, "ShootTime", &Entity->TimeToShoot, 0.05f, 0.0f, FloatMax);
							bValueChanged |= EditorDragFloat(EditorState, "FireballSpeed", &Entity->Speed, 0.05f, 0.0f, FloatMax);
						} break;
                        
						case Entity_Gates:
						{
							EditorInputText(EditorState, "TeleportLevel", Entity->TargetLevelName, ArrayCount(Entity->TargetLevelName));
							
							bValueChanged |= ImGui::Checkbox("FinishGates", &Entity->bFinishGates);
							bValueChanged |= ImGui::Checkbox("ClosedGates", &Entity->bClosedGates);

							if (Entity->bFinishGates)
							{
								Entity->bClosedGates = false;
							}
						} break;

						case Entity_MessageToggler:
						{
							EditorInputText(EditorState, "Message", Entity->MessageText, ArrayCount(Entity->MessageText));

							bValueChanged |= EditorDragFloat2(EditorState, "ScreenPos", &Entity->MessagePos.x, 0.025f);
							bValueChanged |= EditorDragFloat(EditorState, "Scale", &Entity->MessageScale, 0.025f);
							bValueChanged |= EditorDragFloat(EditorState, "LifeTime", &Entity->MessageLifeTime, 0.025f);
							bValueChanged |= EditorDragFloat(EditorState, "TimeToAppear", &Entity->MessageTimeToAppear, 0.025f);
							bValueChanged |= EditorDragFloat(EditorState, "TimeToStartAppear", &Entity->MessageTimeToStartAppear, 0.025f);
						} break;

						case Entity_Checkpoint:
						{
							bValueChanged |= EditorInputU32(EditorState, "DoorIndex", &Entity->DoorIndex, ImGuiInputTextFlags_ReadOnly);
							if (ImGui::Button("SelectDoor"))
							{
								EditorState->bSelectDoor = true;
							}
                            if (EditorInputU8(EditorState, "CheckpointIndex", &Entity->CheckpointIndex))
                            {
                                if (Entity->CheckpointIndex == 0)
                                {
                                    Entity->CheckpointIndex = 1;
                                }
                                bValueChanged |= true;
                            }
						} break;
					}
				}
				else
				{
					bValueChanged |= EditorDragFloat3(EditorState, "Position", &PointLight->Pos.x, 0.05f);
					bValueChanged |= EditorDragFloat(EditorState, "Radius", &PointLight->Radius, 0.05f, 0.0f, FloatMax);
					bValueChanged |= EditorColorEdit3(EditorState, "Color", &PointLight->Color.xyz);
				}
                
				if (bValueChanged && !EditorState->bImGuiChangeStarted)
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
                    
					EditorState->bImGuiChangeStarted = true;
					EditorState->ImGuiChangeTimePassed = 0.0f;
				}
                
				SelectedObjectWindowSize = ImGui::GetWindowSize();
				ImGui::SetWindowPos(ImVec2(Vulkan->Width - SelectedObjectWindowSize.x, 0), true);
				EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
			}
			ImGui::End();
		}
        
		if (ImGui::Begin("ObjectsAndLevelStuff", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			bool bValueChanged = false;
			bValueChanged |= EditorColorEdit3(EditorState, "AmbientColor", &EngineState->Level.AmbientColor);
			bValueChanged |= EditorColorEdit3(EditorState, "AmbientConstant", &EngineState->Level.AmbientConstant);
			
			vec3 SpawnPos = EngineState->Camera.Pos + 3.0f * EngineState->Camera.Dir;
			if (ImGui::CollapsingHeader("Entities"))
			{
				if (ImGui::Button("Torch"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddTorch(EngineState->Level, SpawnPos, Vec3(0.5f), Vec3(0.0f, 1.0f, 0.0f));
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
				if (ImGui::Button("Container"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddContainer(EngineState->Level, SpawnPos);
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
				if (ImGui::Button("Door"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddDoor(EngineState->Level, SpawnPos, Vec3(1.5f, 2.0f, 0.2f), Quat(0, 0, 0, 1), Vec3(0.58f, 0.29f, 0.0f), Vec3(1.0f, 1.0f, 1.0f), Vec3(0.0f, 2.0f, 0.0f));
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
				if (ImGui::Button("Turret"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddTurret(EngineState->Level, SpawnPos, Vec3(1.0f), Quat(0, 0, 0, 1), Vec3(0.8f, 0.8f, 0.8f), Vec3(0.0f, 0.0f, 1.0f), 1.0f);
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
				if (ImGui::Button("Gates"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddGates(EngineState->Level, SpawnPos, Vec3(2.0f, 3.5f, 0.1f), Quat(0, 0, 0, 1));
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;

					if (!CompareStrings(EngineState->LevelName, "Levels\\MainHub.ctl"))
					{
						memcpy(EditorState->SelectedEntity->TargetLevelName, "MainHub", StringLength("MainHub") + 1);
					}
				}
				if (ImGui::Button("MessageToggler"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddMessageToggler(EngineState->Level, SpawnPos);
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
				if (ImGui::Button("Checkpoint"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddCheckpoint(EngineState->Level, SpawnPos);
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
				if (ImGui::Button("ColorField"))
				{
					SaveLevelHistory(EditorState, &EngineState->Level);
					EditorState->SelectedEntity = AddColorField(EngineState->Level, SpawnPos);
					EditorState->SelectedPointLight = 0;
					EditorState->SelectedVoxelsCount = 0;
				}
			}
            
			if (ImGui::Button("PointLight"))
			{
				SaveLevelHistory(EditorState, &EngineState->Level);
				EditorState->SelectedPointLight = AddPointLight(EngineState->Level, SpawnPos, 1.0f, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
				EditorState->SelectedEntity = 0;
				EditorState->SelectedVoxelsCount = 0;
			}

			if (ImGui::Button("SelectAllBlocks"))
			{
				EditorState->SelectedEntity = 0;
				EditorState->SelectedPointLight = 0;
				EditorState->SelectedArrow = SelectedArrow_None;
				EditorState->bCircleSelected = false;
				EditorState->SelectedDimHelper = SelectedDimHelper_None;
				EditorState->bSelectDoor = false;

				EditorState->SelectedVoxelsCount = 0;
				for (uint32_t Z = 0; Z < LevelDimZ; Z++)
				{
					for (uint32_t Y = 0; Y < LevelDimY; Y++)
					{
						for (uint32_t X = 0; X < LevelDimX; X++)
						{
							if (IsVoxelActive(EngineState->Level.Voxels, X, Y, Z))
							{
								uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
								EditorState->SelectedVoxels[EditorState->SelectedVoxelsCount++] = ID;			
							}
						}
					}
				}
			}
            
			if (ImGui::Checkbox("HideEntities", &EngineState->bHideEntities))
			{
				EditorState->SelectedEntity = 0;
			}

			EditorDragFloat(EditorState, "CameraSpeed", &EditorState->CameraSpeed, 0.05f, 0.0f, FloatMax);
            
			if (bValueChanged && !EditorState->bImGuiChangeStarted)
			{
				SaveLevelHistory(EditorState, &EngineState->Level);
                
				EditorState->bImGuiChangeStarted = true;
				EditorState->ImGuiChangeTimePassed = 0.0f;
			}
			
			ImVec2 ImguiWindowSize = ImGui::GetWindowSize();
			ImGui::SetWindowPos(ImVec2(Vulkan->Width - ImguiWindowSize.x, SelectedVoxelWindowSize.y + SelectedObjectWindowSize.y), true);
			EditorState->bIsImguiWindowHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_RootAndChildWindows);
		}
		ImGui::End();
	}
    
	BEGIN_GPU_PROFILER_BLOCK("IMGUI_RENDER", Vulkan->CommandBuffer, Vulkan->FrameInFlight);
    
	ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Vulkan->CommandBuffer);
    
	vkCmdEndRenderPass(Vulkan->CommandBuffer);
    
	END_GPU_PROFILER_BLOCK("IMGUI_RENDER", Vulkan->CommandBuffer, Vulkan->FrameInFlight);
}

void UpdateEditor(SEngineState* EngineState, SGameInput* GameInput, const SVulkanContext* Vulkan, SRenderer* Renderer)
{
	SEditorState* EditorState = &EngineState->EditorState;
	SCamera* Camera = &EngineState->Camera;
	SLevel* Level = &EngineState->Level;

	// NOTE(georgii): This is for velocity vectors to work in editor mode.
	for (uint32_t EntityIndex = 0; EntityIndex < Level->EntityCount; EntityIndex++)
	{
		SEntity* Entity = Level->Entities + EntityIndex;
		
		Entity->PrevPos = Entity->Pos;
		Entity->PrevOrientation = Entity->Orientation;
	}
	
	if ((EditorState->SelectedArrow == SelectedArrow_None) && !EditorState->bCircleSelected && (EditorState->SelectedDimHelper == SelectedDimHelper_None))
	{
		Camera->PrevDir = Camera->Dir;
		Camera->PrevRight = Camera->Right;
		Camera->PrevUp = Camera->Up;
		
		Camera->PrevPitch = Camera->Pitch;
		Camera->PrevHead = Camera->Head;
		
		Camera->Pitch -= 0.1f*GameInput->MouseDeltaY;
		Camera->Pitch = Clamp(Camera->Pitch, -89.0f, 89.0f);
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
	}
	
	if (!EditorState->bIsImguiWindowFocused)
	{
		if (!GameInput->Buttons[Button_Control].IsDown)
		{
			float SpeedScale = GameInput->Buttons[Button_Shift].IsDown ? 0.2f * EditorState->CameraSpeed : EditorState->CameraSpeed;
			
			Camera->Pos += SpeedScale * 0.5f * Camera->Dir * GameInput->MouseWheelDelta;
			if (GameInput->Buttons[Button_W].IsDown)
			{
				Camera->Pos += SpeedScale * 0.3f * Camera->Dir;
			}
			if (GameInput->Buttons[Button_S].IsDown)
			{
				Camera->Pos -= SpeedScale * 0.3f * Camera->Dir;
			} 
			if (GameInput->Buttons[Button_D].IsDown)
			{
				Camera->Pos += SpeedScale * 0.3f * Camera->Right;
			} 
			if (GameInput->Buttons[Button_A].IsDown)
			{
				Camera->Pos -= SpeedScale * 0.3f * Camera->Right;
			}

			if (GameInput->Buttons[Button_E].IsDown)
			{
				Camera->Pos += SpeedScale * 0.3f * Vec3(0.0f, 1.0f, 0.0f);
			}
			if (GameInput->Buttons[Button_Q].IsDown)
			{
				Camera->Pos -= SpeedScale * 0.3f * Vec3(0.0f, 1.0f, 0.0f);
			}
		}
		else
		{
			EditorState->CameraSpeed += 0.1f * GameInput->MouseWheelDelta;

			if (WasDown(GameInput->Buttons[Button_W]))
			{
				EditorState->EditorHelpersMode = EditorHelpersMode_Translation;
				EditorState->bCircleSelected = false;
				EditorState->SelectedDimHelper = SelectedDimHelper_None;
			}
			if (WasDown(GameInput->Buttons[Button_R]))
			{
				EditorState->EditorHelpersMode = EditorHelpersMode_Rotation;
				EditorState->SelectedArrow = SelectedArrow_None;
				EditorState->SelectedDimHelper = SelectedDimHelper_None;
			}
			if (WasDown(GameInput->Buttons[Button_C]))
			{
				EditorState->EditorHelpersMode = EditorHelpersMode_Scale;
				EditorState->SelectedArrow = SelectedArrow_None;
				EditorState->bCircleSelected = false;
			}
		}

		if (WasDown(GameInput->Buttons[Button_B]))
		{
			EditorState->bBuildingMode = !EditorState->bBuildingMode;
		}
	}
	
	if (WasDown(GameInput->Buttons[Button_MouseRight]))
	{
		PlatformDisableCursor(GameInput);
	}
	if (WasReleased(GameInput->Buttons[Button_MouseRight]))
	{
		if ((EditorState->SelectedArrow == SelectedArrow_None) && !EditorState->bCircleSelected && (EditorState->SelectedDimHelper == SelectedDimHelper_None))
		{
			PlatformEnableCursor(GameInput);
		}
	}
	
	bool bRedArrowHit = false, bGreenArrowHit = false, bBlueArrowHit = false;
	bool bRedArrowHitTargetOffset = false, bGreenArrowHitTargetOffset = false, bBlueArrowHitTargetOffset = false;
	
	bool bCircleHit = false;
	
	const vec3 DimHelperDim = Vec3(0.1f, 0.1f, 1.0f);
	const vec3 DimHelperCubeDim = Vec3(0.17f, 0.17f, 0.17f);
	bool bRedDimHelperHit = false, bGreenDimHelperHit = false, bBlueDimHelperHit = false;
	
	float AspectRatio = float(Vulkan->Width) / float(Vulkan->Height);
	mat4 Reproj = Inverse(LookAt(Camera->Pos, Camera->Pos + Camera->Dir, Camera->Up)) * Inverse(Perspective(Camera->FoV, AspectRatio, Camera->Near, Camera->Far));
	if (PlatformIsCursorEnabled())
	{
		vec4 ScreenPos = Vec4(2.0f * (float(GameInput->PlatformMouseX) / Vulkan->Width), 2.0f * (float(GameInput->PlatformMouseY) / Vulkan->Height), 1.0f, 1.0f);
		vec4 WorldPos = Reproj * ScreenPos;
		WorldPos.xyz *= 1.0f / WorldPos.w;
		
		vec3 RayStartP = Camera->Pos;
		vec3 RayDir = Normalize(WorldPos.xyz - RayStartP);
		
		float tHitObject = FloatMax;
		int32_t HitEntityIndex = -1;
		SEntity *HitEntity = 0;
		SPointLight *HitPointLight = 0;
		if (!EditorState->bIsImguiWindowHovered)
		{
			if (!EngineState->bHideEntities)
			{
				for (uint32_t I = 0; I < Level->EntityCount; I++)
				{
					SEntity* Entity = Level->Entities + I;
					Rect EntityAABB = RectCenterDimOrientation(Entity->Pos, Entity->Dim, EulerToQuat(Entity->Orientation.xyz));

					if ((Entity->Type == Entity_MessageToggler) || (Entity->Type == Entity_Checkpoint))
					{
						EntityAABB = RectCenterDimOrientation(Entity->Pos, Vec3(0.3f, 0.3f, 0.3f), EulerToQuat(Entity->Orientation.xyz));
					}
					
					float tTest;
					if (IntersectRectRay(EntityAABB, RayStartP, RayDir, tTest))
					{
						if (tTest < tHitObject)
						{
							tHitObject = tTest;
							HitEntity = Entity;
							HitEntityIndex = I;
						}
					}
				}
			}
			
			for (uint32_t I = 0; I < Level->PointLightCount; I++)
			{
				SPointLight* PointLight = Level->PointLights + I;
				
				float tTest;
				if (IntersectSphereRay(PointLight->Pos, 0.1f, RayStartP, RayDir, tTest))
				{
					if (tTest < tHitObject)
					{
						HitEntity = 0;
						HitEntityIndex = -1;
						
						tHitObject = tTest;
						HitPointLight = PointLight;
					}
				}
			}
			
			if (EditorState->SelectedEntity || EditorState->SelectedPointLight)
			{
				SEntity* Entity = EditorState->SelectedEntity;
				SPointLight* PointLight = EditorState->SelectedPointLight;
				switch (EditorState->EditorHelpersMode)
				{
					case EditorHelpersMode_Translation:
					{
						vec3 Pos = Entity ? Entity->Pos : PointLight->Pos;
						Rect RedArrowAABB = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), Quat(Vec3(0.0f, 1.0f, 0.0f), 90.0f)), Pos);
						Rect GreenArrowAABB = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), Quat(Vec3(1.0f, 0.0f, 0.0f), -90.0f)), Pos);
						Rect BlueArrowAABB = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), Quat(Vec3(0.0f, 0.0f, 0.0f), 0.0f)), Pos);
						
						float tTest = FloatMax;
						if (IntersectRectRay(RedArrowAABB, RayStartP, RayDir, tTest))
						{
							if ((tTest < tHitObject) || (HitEntity == Entity))
							{
								bRedArrowHit = true;
								tHitObject = tTest;
							}
						}
						
						tTest = FloatMax;
						if (IntersectRectRay(GreenArrowAABB, RayStartP, RayDir, tTest))
						{
							if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedArrowHit))
							{
								bRedArrowHit = false;
								bGreenArrowHit = true;
								tHitObject = tTest;
							}
						}
						
						tTest = FloatMax;
						if (IntersectRectRay(BlueArrowAABB, RayStartP, RayDir, tTest))
						{
							if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedArrowHit && !bGreenArrowHit))
							{
								bRedArrowHit = bGreenArrowHit = false;
								bBlueArrowHit = true;
								tHitObject = tTest;
							}
						}
						
						if (Entity && HasTargetPos(Entity))
						{
							Rect RedArrowAABBOffset;
							Rect GreenArrowAABBOffset;
							Rect BlueArrowAABBOffset;
							if (Entity->Type == Entity_Turret)
							{
								quat OrientationQuat = EulerToQuat(Entity->Orientation.xyz);
								vec3 Offset = RotateByQuaternion(Entity->TargetOffset, OrientationQuat);
								RedArrowAABBOffset = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), OrientationQuat * Quat(Vec3(0.0f, 1.0f, 0.0f), 90.0f)), Entity->Pos + Offset);
								GreenArrowAABBOffset = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), OrientationQuat * Quat(Vec3(1.0f, 0.0f, 0.0f), -90.0f)), Entity->Pos + Offset);
								BlueArrowAABBOffset = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), OrientationQuat * Quat(Vec3(0.0f, 0.0f, 0.0f), 0.0f)), Entity->Pos + Offset);
							}
							else
							{
								RedArrowAABBOffset = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), Quat(Vec3(0.0f, 1.0f, 0.0f), 90.0f)), Entity->Pos + Entity->TargetOffset);
								GreenArrowAABBOffset = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), Quat(Vec3(1.0f, 0.0f, 0.0f), -90.0f)), Entity->Pos + Entity->TargetOffset);
								BlueArrowAABBOffset = RectOffset(RectRectOrientation(SDebugRenderPass::GetArrowAABB(), Quat(Vec3(0.0f, 0.0f, 0.0f), 0.0f)), Entity->Pos + Entity->TargetOffset);
							}
							
							tTest = FloatMax;
							if (IntersectRectRay(RedArrowAABBOffset, RayStartP, RayDir, tTest))
							{
								if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedArrowHit && !bGreenArrowHit && !bBlueArrowHit))
								{
									bRedArrowHit = bGreenArrowHit = bBlueArrowHit = false;
									bRedArrowHitTargetOffset = true;
									tHitObject = tTest;
								}
							}
							
							tTest = FloatMax;
							if (IntersectRectRay(GreenArrowAABBOffset, RayStartP, RayDir, tTest))
							{
								if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedArrowHit && !bGreenArrowHit && !bBlueArrowHit && !bRedArrowHitTargetOffset))
								{
									bRedArrowHit = bGreenArrowHit = bBlueArrowHit = false;
									bRedArrowHitTargetOffset = false;
									bGreenArrowHitTargetOffset = true;
									tHitObject = tTest;
								}
							}
							
							tTest = FloatMax;
							if (IntersectRectRay(BlueArrowAABBOffset, RayStartP, RayDir, tTest))
							{
								if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedArrowHit && !bGreenArrowHit && !bBlueArrowHit && !bRedArrowHitTargetOffset && !bGreenArrowHitTargetOffset))
								{
									bRedArrowHit = bGreenArrowHit = bBlueArrowHit = false;
									bRedArrowHitTargetOffset = bGreenArrowHitTargetOffset = false;
									bBlueArrowHitTargetOffset = true;
									tHitObject = tTest;
								}
							}
						}
					} break;
					
					case EditorHelpersMode_Rotation:
					{
						if (Entity)
						{
							const uint32_t CircleIndexCount = SDebugRenderPass::GetCircleIndexCount();
							const uint32_t* CircleIndices = SDebugRenderPass::GetCircleIndices();
							const SVertex* CircleVertices = SDebugRenderPass::GetCircleVertices();
							
							float Radius = Max(Entity->Dim.x, Entity->Dim.z);
							
							Assert(CircleIndexCount % 3 == 0);
							for (uint32_t I = 0; I < CircleIndexCount / 3; I++)
							{
								vec3 A = Radius * CircleVertices[CircleIndices[3*I]].Pos + Entity->Pos;
								vec3 B = Radius * CircleVertices[CircleIndices[3*I + 1]].Pos + Entity->Pos;
								vec3 C = Radius * CircleVertices[CircleIndices[3*I + 2]].Pos + Entity->Pos;
								
								float tTest = FloatMax;
								if (IntersectRayTriangle(RayStartP, RayDir, A, B, C, tTest))
								{
									if ((tTest < tHitObject) || (HitEntity == Entity))
									{
										bCircleHit = true;
										tHitObject = tTest;
									}
								}
							}
						}
					} break;
					
					case EditorHelpersMode_Scale:
					{
						if (Entity)
						{
							{
								// Red
								quat Orientation = EulerToQuat(Entity->Orientation.xyz) * Quat(Vec3(0.0f, 1.0f, 0.0f), 90.0f);
								vec3 Origin = RotateByQuaternion(Vec3(0.0f, 0.0f, 0.5f * DimHelperDim.z), Orientation) + Entity->Pos;
								vec3 OriginCube = RotateByQuaternion(Vec3(0.0f, 0.0f, DimHelperDim.z), Orientation) + Entity->Pos;
								
								vec3 Axes[3];
								Axes[0] = RotateByQuaternion(Vec3(1.0f, 0.0f, 0.0f), Orientation);
								Axes[1] = RotateByQuaternion(Vec3(0.0f, 1.0f, 0.0f), Orientation);
								Axes[2] = RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), Orientation);
								
								vec3 Extents[3];
								Extents[0] = DimHelperDim.x * Axes[0];
								Extents[1] = DimHelperDim.y * Axes[1];
								Extents[2] = DimHelperDim.z * Axes[2];
								
								vec3 ExtentsCube[3];
								ExtentsCube[0] = DimHelperCubeDim.x * Axes[0];
								ExtentsCube[1] = DimHelperCubeDim.y * Axes[1];
								ExtentsCube[2] = DimHelperCubeDim.z * Axes[2];
								
								float tTest = FloatMax;
								if (IntersectOBBRay(Origin, Extents, RayStartP, RayDir, tTest) || IntersectOBBRay(OriginCube, ExtentsCube, RayStartP, RayDir, tTest))
								{
									if ((tTest < tHitObject) || (HitEntity == Entity))
									{
										bRedDimHelperHit = true;
										tHitObject = tTest;
									}
								}
							}
							
							{
								// Green
								quat Orientation = EulerToQuat(Entity->Orientation.xyz) * Quat(Vec3(1.0f, 0.0f, 0.0f), -90.0f);
								vec3 Origin = Entity->Pos + Vec3(0.0f, 0.5f * DimHelperDim.y, 0.0f);
								vec3 OriginCube = Entity->Pos + Vec3(0.0f, DimHelperDim.y, 0.0f);
								
								vec3 Axes[3];
								Axes[0] = RotateByQuaternion(Vec3(1.0f, 0.0f, 0.0f), Orientation);
								Axes[1] = RotateByQuaternion(Vec3(0.0f, 1.0f, 0.0f), Orientation);
								Axes[2] = RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), Orientation);
								
								vec3 Extents[3];
								Extents[0] = DimHelperDim.x * Axes[0];
								Extents[1] = DimHelperDim.y * Axes[1];
								Extents[2] = DimHelperDim.z * Axes[2];
								
								vec3 ExtentsCube[3];
								ExtentsCube[0] = DimHelperCubeDim.x * Axes[0];
								ExtentsCube[1] = DimHelperCubeDim.y * Axes[1];
								ExtentsCube[2] = DimHelperCubeDim.z * Axes[2];
								
								float tTest = FloatMax;
								if (IntersectOBBRay(Origin, Extents, RayStartP, RayDir, tTest) || IntersectOBBRay(OriginCube, ExtentsCube, RayStartP, RayDir, tTest))
								{
									if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedDimHelperHit))
									{
										bRedDimHelperHit = false;
										bGreenDimHelperHit = true;
										tHitObject = tTest;
									}
								}
							}
							
							{
								// Blue
								quat Orientation = EulerToQuat(Entity->Orientation.xyz);
								vec3 Origin = RotateByQuaternion(Vec3(0.0f, 0.0f, 0.5f * DimHelperDim.z), Orientation) + Entity->Pos;
								vec3 OriginCube = RotateByQuaternion(Vec3(0.0f, 0.0f, DimHelperDim.z), Orientation) + Entity->Pos;
								
								vec3 Axes[3];
								Axes[0] = RotateByQuaternion(Vec3(1.0f, 0.0f, 0.0f), Orientation);
								Axes[1] = RotateByQuaternion(Vec3(0.0f, 1.0f, 0.0f), Orientation);
								Axes[2] = RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), Orientation);
								
								vec3 Extents[3];
								Extents[0] = DimHelperDim.x * Axes[0];
								Extents[1] = DimHelperDim.y * Axes[1];
								Extents[2] = DimHelperDim.z * Axes[2];
								
								vec3 ExtentsCube[3];
								ExtentsCube[0] = DimHelperCubeDim.x * Axes[0];
								ExtentsCube[1] = DimHelperCubeDim.y * Axes[1];
								ExtentsCube[2] = DimHelperCubeDim.z * Axes[2];
								
								float tTest = FloatMax;
								if (IntersectOBBRay(Origin, Extents, RayStartP, RayDir, tTest) || IntersectOBBRay(OriginCube, ExtentsCube, RayStartP, RayDir, tTest))
								{
									if ((tTest < tHitObject) || ((HitEntity == Entity) && !bRedDimHelperHit && !bGreenDimHelperHit))
									{
										bRedDimHelperHit = bGreenDimHelperHit = false;
										bBlueDimHelperHit = true;
										tHitObject = tTest;
									}
								}
							}
						}
					} break;
				}
			}
		}
		
		bool bNoHelpersHit = !bRedArrowHit && !bGreenArrowHit && !bBlueArrowHit && !bRedArrowHitTargetOffset && !bGreenArrowHitTargetOffset && !bBlueArrowHitTargetOffset && !bCircleHit && !bRedDimHelperHit && !bGreenDimHelperHit && !bBlueDimHelperHit;
		SRaytraceVoxelsResult RaytraceResult = RaytraceVoxels(Level, RayStartP, RayDir, 100.0f, true);
		if ((RaytraceResult.bHit && (RaytraceResult.Distance < tHitObject) && bNoHelpersHit) || (!HitEntity && !HitPointLight && bNoHelpersHit))
		{
			HitEntity = 0;
			HitEntityIndex = -1;
			HitPointLight = 0;
			bRedArrowHit = bGreenArrowHit = bBlueArrowHit = false;
			bRedArrowHitTargetOffset = bGreenArrowHitTargetOffset = bBlueArrowHitTargetOffset = false;
			bCircleHit = false;
			bRedDimHelperHit = bGreenDimHelperHit = bBlueDimHelperHit = false;
			
			uint32_t EmptyX = RaytraceResult.LastEmptyX;
			uint32_t EmptyY = RaytraceResult.LastEmptyY;
			uint32_t EmptyZ = RaytraceResult.LastEmptyZ;
			uint32_t EmptyID = EmptyZ*LevelDimX*LevelDimY + EmptyY*LevelDimX + EmptyX;
			
			uint32_t SolidX = RaytraceResult.X;
			uint32_t SolidY = RaytraceResult.Y;
			uint32_t SolidZ = RaytraceResult.Z;
			uint32_t SolidID = SolidZ*LevelDimX*LevelDimY + SolidY*LevelDimX + SolidX;
			
			if (EditorState->bBuildingMode)
			{
				if (RaytraceResult.bHit && !EditorState->bIsImguiWindowHovered && !GameInput->Buttons[Button_X].IsDown)
				{
					if (GameInput->Buttons[Button_MouseLeft].IsDown)
					{
						if (GameInput->Buttons[Button_Control].IsDown)
						{
							bool bFound = false;
							for (uint32_t I = 0; I < EditorState->BuildingVoxelsToAddCount; I++)
							{
								if (EmptyID == EditorState->BuildingVoxelsToAdd[I])
									bFound = true;
							}
							
							if (!bFound)
							{
								Assert(EditorState->BuildingVoxelsToAddCount < ArrayCount(EditorState->BuildingVoxelsToAdd));
								EditorState->BuildingVoxelsToAdd[EditorState->BuildingVoxelsToAddCount++] = EmptyID;
							}
						}
						else
						{
							EditorState->BuildingVoxelsToAddCount = 1;
							EditorState->BuildingVoxelsToAdd[0] = EmptyID;
						}
					}
				}
				
				if (WasReleased(GameInput->Buttons[Button_MouseLeft]))
				{
					for (uint32_t I = 0; I < EditorState->BuildingVoxelsToAddCount; I++)
					{
						uint32_t ID = EditorState->BuildingVoxelsToAdd[I];
						uint32_t X = ID % LevelDimX;
						uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
						uint32_t Z = ID / (LevelDimX*LevelDimY);
						
						AddVoxelToLevel(EngineState, X, Y, Z);
					}
					EditorState->BuildingVoxelsToAddCount = 0;
					
					EditorState->WallSetMode = WallSetMode_None;
				}
				
				if (WasReleased(GameInput->Buttons[Button_Control]))
				{
					EditorState->BuildingVoxelsToAddCount = 0;
				}
				
				if (WasReleased(GameInput->Buttons[Button_X]))
				{
					EditorState->BuildingVoxelsToAddCount = 0;
					EditorState->WallSetMode = WallSetMode_None;
				}
			}
			else
			{
				if (RaytraceResult.bHit && !EditorState->bIsImguiWindowHovered)
				{
					if (GameInput->Buttons[Button_MouseLeft].IsDown)
					{
						if (!GameInput->Buttons[Button_X].IsDown)
						{
							EditorState->WallSetMode = WallSetMode_None;
						}
						
						if (GameInput->Buttons[Button_Control].IsDown)
						{
							bool bFound = false;
							for (uint32_t I = 0; I < EditorState->SelectedVoxelsCount; I++)
							{
								if (SolidID == EditorState->SelectedVoxels[I])
									bFound = true;
							}
							
							if (!bFound)
							{
								Assert(EditorState->SelectedVoxelsCount < ArrayCount(EditorState->SelectedVoxels));
								EditorState->SelectedVoxels[EditorState->SelectedVoxelsCount++] = SolidID;
							}
						}
						else
						{
							EditorState->SelectedVoxelsCount = 1;
							EditorState->SelectedVoxels[0] = SolidID;
						}
					}
				}
				
				if (EditorState->SelectedVoxelsCount > 0)
				{
					uint32_t ID = EditorState->SelectedVoxels[0];
					uint32_t X = ID % LevelDimX;
					uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
					uint32_t Z = ID / (LevelDimX*LevelDimY);
					
					uint32_t* Voxel = &Level->Voxels.ColorActive[Z][Y][X];
					float Red = ((*Voxel >> 24) & 0xFF) / 255.0f;
					float Green = ((*Voxel >> 16) & 0xFF) / 255.0f;
					float Blue = ((*Voxel >> 8) & 0xFF) / 255.0f;
					EditorState->SelectedVoxelColorFloat = Vec3(Red, Green, Blue);
				}
				
				if (WasDown(GameInput->Buttons[Button_Delete]))
				{
					for (uint32_t I = 0; I < EditorState->SelectedVoxelsCount; I++)
					{
						uint32_t ID = EditorState->SelectedVoxels[I];
						uint32_t X = ID % LevelDimX;
						uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
						uint32_t Z = ID / (LevelDimX*LevelDimY);
						
						MarkVoxelForDeletion(EngineState, X, Y, Z);
					}
					EditorState->SelectedVoxelsCount = 0;
					
					if (!EditorState->bBuildingMode)
					{
						EditorState->WallSetMode = WallSetMode_None;
					}
				}
			}
			
			if (WasDown(GameInput->Buttons[Button_X]))
			{
				EditorState->WallSetMode = WallSetMode_None;
			}
			
			if (GameInput->Buttons[Button_X].IsDown)
			{
				if (GameInput->Buttons[Button_MouseLeft].IsDown)
				{
					uint32_t FirstID = EditorState->bBuildingMode ? EmptyID : SolidID;
					if (RaytraceResult.bHit && (EditorState->WallSetMode == WallSetMode_None))
					{
						float RayX = Absolute(Dot(Camera->Dir, Vec3(1.0f, 0.0f, 0.0f)));
						float RayY = Absolute(Dot(Camera->Dir, Vec3(0.0f, 1.0f, 0.0f)));
						float RayZ = Absolute(Dot(Camera->Dir, Vec3(0.0f, 0.0f, 1.0f)));
						
						if ((RayX > RayY) && (RayX > RayZ))
						{
							EditorState->WallSetMode = WallSetMode_ZY;
						}
						else if ((RayZ > RayY) && (RayZ > RayX))
						{
							EditorState->WallSetMode = WallSetMode_XY;
						}
						else 
						{
							EditorState->WallSetMode = WallSetMode_XZ;
						}
						
						EditorState->WallSetLastWorldPos = WorldPos.xyz;
						EditorState->WallSetFirstID = FirstID;
						EditorState->WallSetWidth = EditorState->WallSetHeight = 1;
						
						Assert(EditorState->BuildingVoxelsToAddCount < ArrayCount(EditorState->BuildingVoxelsToAdd));
						EditorState->BuildingVoxelsToAdd[EditorState->BuildingVoxelsToAddCount++] = FirstID;
					}
					
					if (EditorState->WallSetMode != WallSetMode_None)
					{
						vec3 WorldPosDelta = WorldPos.xyz - EditorState->WallSetLastWorldPos;
						
						switch (EditorState->WallSetMode)
						{
							case WallSetMode_XY:
							{
								if (Absolute(0.03f * WorldPosDelta.x) > VoxelDim)
								{
									EditorState->WallSetLastWorldPos = WorldPos.xyz;
									
									if (WorldPosDelta.x > 0.0f)
									{
										EditorState->WallSetWidth++;
										if (EditorState->WallSetWidth == 0)
										{
											EditorState->WallSetWidth = 1;
										}
									}
									else
									{
										EditorState->WallSetWidth--;
										if (EditorState->WallSetWidth == 0)
										{
											EditorState->WallSetWidth = -1;
										}
									}
								}
								
								if (Absolute(0.03f * WorldPosDelta.y) > VoxelDim)
								{
									EditorState->WallSetLastWorldPos = WorldPos.xyz;
									
									if (WorldPosDelta.y > 0.0f)
									{
										EditorState->WallSetHeight++;
										if (EditorState->WallSetHeight == 0)
										{
											EditorState->WallSetHeight = 1;
										}
									}
									else
									{
										EditorState->WallSetHeight--;
										if (EditorState->WallSetHeight == 0)
										{
											EditorState->WallSetHeight = -1;
										}
									}
								}
							} break;
							
							case WallSetMode_XZ:
							{
								if (Absolute(0.03f * WorldPosDelta.x) > VoxelDim)
								{
									EditorState->WallSetLastWorldPos = WorldPos.xyz;
									
									if (WorldPosDelta.x > 0.0f)
									{
										EditorState->WallSetWidth++;
										if (EditorState->WallSetWidth == 0)
										{
											EditorState->WallSetWidth = 1;
										}
									}
									else
									{
										EditorState->WallSetWidth--;
										if (EditorState->WallSetWidth == 0)
										{
											EditorState->WallSetWidth = -1;
										}
									}
								}
								
								if (Absolute(0.03f * WorldPosDelta.z) > VoxelDim)
								{
									EditorState->WallSetLastWorldPos = WorldPos.xyz;
									
									if (WorldPosDelta.z > 0.0f)
									{
										EditorState->WallSetHeight++;
										if (EditorState->WallSetHeight == 0)
										{
											EditorState->WallSetHeight = 1;
										}
									}
									else
									{
										EditorState->WallSetHeight--;
										if (EditorState->WallSetHeight == 0)
										{
											EditorState->WallSetHeight = -1;
										}
									}
								}
							} break;
							
							case WallSetMode_ZY:
							{
								if (Absolute(0.03f * WorldPosDelta.z) > VoxelDim)
								{
									EditorState->WallSetLastWorldPos = WorldPos.xyz;
									
									if (WorldPosDelta.z > 0.0f)
									{
										EditorState->WallSetWidth++;
										if (EditorState->WallSetWidth == 0)
										{
											EditorState->WallSetWidth = 1;
										}
									}
									else
									{
										EditorState->WallSetWidth--;
										if (EditorState->WallSetWidth == 0)
										{
											EditorState->WallSetWidth = -1;
										}
									}
								}
								
								if (Absolute(0.03f * WorldPosDelta.y) > VoxelDim)
								{
									EditorState->WallSetLastWorldPos = WorldPos.xyz;
									
									if (WorldPosDelta.y > 0.0f)
									{
										EditorState->WallSetHeight++;
										if (EditorState->WallSetHeight == 0)
										{
											EditorState->WallSetHeight = 1;
										}
									}
									else
									{
										EditorState->WallSetHeight--;
										if (EditorState->WallSetHeight == 0)
										{
											EditorState->WallSetHeight = -1;
										}
									}
								}
							} break;
						}
					}
				}
			}
		}
		else
		{
			if (!EditorState->bIsImguiWindowHovered && WasDown(GameInput->Buttons[Button_MouseLeft]))
			{
				if (!bRedArrowHit && !bGreenArrowHit && !bBlueArrowHit && !bRedArrowHitTargetOffset && !bGreenArrowHitTargetOffset && !bBlueArrowHitTargetOffset && !bCircleHit && !bRedDimHelperHit && !bGreenDimHelperHit && !bBlueDimHelperHit)
				{
					if (EditorState->bSelectDoor && (HitEntity->Type == Entity_Door))
					{
						Assert(EditorState->SelectedEntity && (EditorState->SelectedEntity->Type == Entity_Checkpoint));
						Assert(HitEntityIndex > 0);

						EditorState->SelectedEntity->DoorIndex = uint32_t(HitEntityIndex);
						EditorState->bSelectDoor = false;
					}
					else
					{
						EditorState->SelectedEntity = HitEntity;
						EditorState->SelectedPointLight = HitPointLight;
						EditorState->BuildingVoxelsToAddCount = 0;
						EditorState->SelectedVoxelsCount = 0;
						EditorState->bSelectDoor = false;
					}
				}
			}
		}
	}
	
	if ((EditorState->BuildingVoxelsToAddCount > 0) || (EditorState->SelectedVoxelsCount > 0))
	{
		EditorState->SelectedEntity = 0;
		EditorState->SelectedPointLight = 0;
		EditorState->SelectedArrow = SelectedArrow_None;
		EditorState->bCircleSelected = false;
		EditorState->SelectedDimHelper = SelectedDimHelper_None;
		EditorState->bSelectDoor = false;
	}
	
	// Draw editor voxel stuff
	if (EditorState->WallSetMode != WallSetMode_None)
	{
		int32_t FirstID = EditorState->WallSetFirstID;
		uint32_t FirstX = FirstID % LevelDimX;
		uint32_t FirstY = (FirstID % (LevelDimX*LevelDimY)) / LevelDimX;
		uint32_t FirstZ = FirstID / (LevelDimX*LevelDimY);
		
		EditorState->BuildingVoxelsToAddCount = EditorState->SelectedVoxelsCount = 0;
		for (int32_t Height = 0; Height < Absolute(EditorState->WallSetHeight); Height++)
		{
			for (int32_t Width = 0; Width < Absolute(EditorState->WallSetWidth); Width++)
			{
				int32_t X = FirstX;
				if ((EditorState->WallSetMode == WallSetMode_XY) || (EditorState->WallSetMode == WallSetMode_XZ))
				{
					X += Sign(EditorState->WallSetWidth) * Width;
				}
				
				int32_t Y = FirstY;
				if ((EditorState->WallSetMode == WallSetMode_XY) || (EditorState->WallSetMode == WallSetMode_ZY))
				{
					Y += Sign(EditorState->WallSetHeight) * Height;
				}
				
				int32_t Z = FirstZ;
				if ((EditorState->WallSetMode == WallSetMode_ZY) || (EditorState->WallSetMode == WallSetMode_XZ))
				{
					if (EditorState->WallSetMode == WallSetMode_ZY)
					{
						Z += Sign(EditorState->WallSetWidth) * Width;
					}
					else
					{
						Z += Sign(EditorState->WallSetHeight) * Height;
					}
				}
				
				if ((X >= 0) && (X < LevelDimX) && (Y >= 0) && (Y < LevelDimY) && (Z >= 0) && (Z < LevelDimZ))
				{
					bool bVoxelActive = IsVoxelActive(Level->Voxels, X, Y, Z);
					if (EditorState->bBuildingMode && !bVoxelActive)
					{
						uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
						EditorState->BuildingVoxelsToAdd[EditorState->BuildingVoxelsToAddCount++] = ID;
					}
					else if (!EditorState->bBuildingMode && bVoxelActive)
					{
						uint32_t ID = Z*LevelDimX*LevelDimY + Y*LevelDimX + X;
						EditorState->SelectedVoxels[EditorState->SelectedVoxelsCount++] = ID;
					}
				}
			}
		}
		
		vec3 Pos = VoxelDim * Vec3i(FirstX, FirstY, FirstZ) + 0.5f * Vec3(VoxelDim);
		switch (EditorState->WallSetMode)
		{
			case WallSetMode_XY:
			{
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(0.5f), Vec3(1.0f, 0.0f, 0.0f), Vec4(Vec3(0.0f, 1.0f, 0.0f), 90.0f));
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(0.5f), Vec3(0.0f, 1.0f, 0.0f), Vec4(Vec3(1.0f, 0.0f, 0.0f), -90.0f));
			} break;
			
			case WallSetMode_ZY:
			{
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(0.5f), Vec3(0.0f, 0.0f, 1.0f));
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(0.5f), Vec3(0.0f, 1.0f, 0.0f), Vec4(Vec3(1.0f, 0.0f, 0.0f), -90.0f));
			} break;
			
			case WallSetMode_XZ:
			{
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(0.5f), Vec3(1.0f, 0.0f, 0.0f), Vec4(Vec3(0.0f, 1.0f, 0.0f), 90.0f));
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(0.5f), Vec3(0.0f, 0.0f, 1.0f));
			} break;
		}
	}
	
	for (uint32_t I = 0; I < EditorState->BuildingVoxelsToAddCount; I++)
	{
		uint32_t ID = EditorState->BuildingVoxelsToAdd[I];
		uint32_t X = ID % LevelDimX;
		uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
		uint32_t Z = ID / (LevelDimX*LevelDimY);
		
		vec3 Pos = VoxelDim * Vec3i(X, Y, Z) + 0.5f * Vec3(VoxelDim);
		Renderer->DebugRenderPass.DrawDebugBox(Pos, Vec3(VoxelDim), Vec3(1.0f, 1.0f, 1.0f));
	}
	
	for (uint32_t I = 0; I < EditorState->SelectedVoxelsCount; I++)
	{
		uint32_t ID = EditorState->SelectedVoxels[I];
		uint32_t X = ID % LevelDimX;
		uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
		uint32_t Z = ID / (LevelDimX*LevelDimY);
		
		vec3 Pos = VoxelDim * Vec3i(X, Y, Z) + 0.5f * Vec3(VoxelDim);
		Renderer->DebugRenderPass.DrawDebugBox(Pos, Vec3(VoxelDim), Vec3(1.0f, 0.0f, 0.0f));
	}
	
	// Handle editor entity/pointlight stuff
	if (EditorState->SelectedEntity || EditorState->SelectedPointLight)
	{
		SEntity* Entity = EditorState->SelectedEntity;
		SPointLight* PointLight = EditorState->SelectedPointLight;
		
		switch (EditorState->EditorHelpersMode)
		{
			case EditorHelpersMode_Translation:
			{
				vec3 Pos = Entity ? Entity->Pos : PointLight->Pos;
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(1.0f), (bRedArrowHit || (EditorState->SelectedArrow == SelectedArrow_Red)) ? Vec3(1.0f, 0.8f, 0.8f) : Vec3(1.0f, 0.0f, 0.0f), Vec4(Vec3(0.0f, 1.0f, 0.0f), 90.0f));
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(1.0f), (bGreenArrowHit || (EditorState->SelectedArrow == SelectedArrow_Green)) ? Vec3(0.8f, 1.0f, 0.8f) : Vec3(0.0f, 1.0f, 0.0f), Vec4(Vec3(1.0f, 0.0f, 0.0f), -90.0f));
				Renderer->DebugRenderPass.DrawDebugArrow(Pos, Vec3(1.0f), (bBlueArrowHit || (EditorState->SelectedArrow == SelectedArrow_Blue)) ? Vec3(0.8f, 0.8f, 1.0f) : Vec3(0.0f, 0.0f, 1.0f));
				
				if (Entity && HasTargetPos(Entity))
				{
					if (Entity->Type == Entity_Turret)
					{
						vec3 Offset = RotateByQuaternion(Entity->TargetOffset, EulerToQuat(Entity->Orientation.xyz));
						
						vec4 RedRotation = QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz) * Quat(Vec3(0.0f, 1.0f, 0.0f), 90.0f));
						vec4 GreenRotation = QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz) * Quat(Vec3(1.0f, 0.0f, 0.0f), -90.0f));
						vec4 BlueRotation = QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz));
						
						Renderer->DebugRenderPass.DrawDebugArrow(Entity->Pos + Offset, Vec3(1.0f), (bRedArrowHitTargetOffset || (EditorState->SelectedArrow == SelectedArrow_RedTargetOffset)) ? Vec3(1.0f, 0.8f, 0.8f) : Vec3(1.0f, 0.0f, 0.0f), RedRotation);
						Renderer->DebugRenderPass.DrawDebugArrow(Entity->Pos + Offset, Vec3(1.0f), (bGreenArrowHitTargetOffset || (EditorState->SelectedArrow == SelectedArrow_GreenTargetOffset)) ? Vec3(0.8f, 1.0f, 0.8f) : Vec3(0.0f, 1.0f, 0.0f), GreenRotation);
						Renderer->DebugRenderPass.DrawDebugArrow(Entity->Pos + Offset, Vec3(1.0f), (bBlueArrowHitTargetOffset || (EditorState->SelectedArrow == SelectedArrow_BlueTargetOffset)) ? Vec3(0.8f, 0.8f, 1.0f) : Vec3(0.0f, 0.0f, 1.0f), BlueRotation);
					}
					else
					{
						Renderer->DebugRenderPass.DrawDebugArrow(Entity->Pos + Entity->TargetOffset, Vec3(1.0f), (bRedArrowHitTargetOffset || (EditorState->SelectedArrow == SelectedArrow_RedTargetOffset)) ? Vec3(1.0f, 0.8f, 0.8f) : Vec3(1.0f, 0.0f, 0.0f), Vec4(Vec3(0.0f, 1.0f, 0.0f), 90.0f));
						Renderer->DebugRenderPass.DrawDebugArrow(Entity->Pos + Entity->TargetOffset, Vec3(1.0f), (bGreenArrowHitTargetOffset || (EditorState->SelectedArrow == SelectedArrow_GreenTargetOffset)) ? Vec3(0.8f, 1.0f, 0.8f) : Vec3(0.0f, 1.0f, 0.0f), Vec4(Vec3(1.0f, 0.0f, 0.0f), -90.0f));
						Renderer->DebugRenderPass.DrawDebugArrow(Entity->Pos + Entity->TargetOffset, Vec3(1.0f), (bBlueArrowHitTargetOffset || (EditorState->SelectedArrow == SelectedArrow_BlueTargetOffset)) ? Vec3(0.8f, 0.8f, 1.0f) : Vec3(0.0f, 0.0f, 1.0f));
					}
				}
				
				if (WasDown(GameInput->Buttons[Button_MouseLeft]) && (bRedArrowHit || bGreenArrowHit || bBlueArrowHit || bRedArrowHitTargetOffset || bGreenArrowHitTargetOffset || bBlueArrowHitTargetOffset))
				{
					SaveLevelHistory(EditorState, Level);
					
					EditorState->SelectedArrow = bRedArrowHit ? SelectedArrow_Red : (bGreenArrowHit ? SelectedArrow_Green : SelectedArrow_Blue);
					if (bRedArrowHitTargetOffset || bGreenArrowHitTargetOffset || bBlueArrowHitTargetOffset)
					{
						EditorState->SelectedArrow = bRedArrowHitTargetOffset ? SelectedArrow_RedTargetOffset : (bGreenArrowHitTargetOffset ? SelectedArrow_GreenTargetOffset : SelectedArrow_BlueTargetOffset);
					}
					PlatformDisableCursor(GameInput);
					
					if (Entity && (Entity->Type != Entity_Hero) && GameInput->Buttons[Button_Alt].IsDown)
					{
						EditorState->SelectedEntity = AddEntityCopy(Level, Entity);
						Entity = EditorState->SelectedEntity;
					}
				}
				
				if (EditorState->SelectedArrow != SelectedArrow_None)
				{
					vec4 ScreenPos0 = Vec4(2.0f * (float(GameInput->PlatformMouseX) / Vulkan->Width), 2.0f * (float(GameInput->PlatformMouseY) / Vulkan->Height), 1.0f, 1.0f);
					vec4 WorldPos0 = Reproj * ScreenPos0;
					WorldPos0.xyz *= 1.0f / WorldPos0.w;
					
					vec4 ScreenPos1 = Vec4(2.0f * (float(GameInput->PlatformMouseX + GameInput->MouseDeltaX) / Vulkan->Width), 2.0f * (float(GameInput->PlatformMouseY - GameInput->MouseDeltaY) / Vulkan->Height), 1.0f, 1.0f);
					vec4 WorldPos1 = Reproj * ScreenPos1;
					WorldPos1.xyz *= 1.0f / WorldPos1.w;
					
					vec3 WorldPosDelta = WorldPos1.xyz - WorldPos0.xyz;
					switch (EditorState->SelectedArrow)
					{
						case SelectedArrow_Red:
						{
							if (Entity)
							{
								Entity->Pos.x += 0.01f * WorldPosDelta.x;
								Entity->BasePos = Entity->Pos;
							}
							else
							{
								Assert(PointLight);
								PointLight->Pos.x += 0.01f * WorldPosDelta.x;
							}
						} break;
						
						case SelectedArrow_Green:
						{
							if (Entity)
							{
								Entity->Pos.y += 0.01f * WorldPosDelta.y;
								Entity->BasePos = Entity->Pos;
							}
							else
							{
								Assert(PointLight);
								PointLight->Pos.y += 0.01f * WorldPosDelta.y;
							}
						} break;
						
						case SelectedArrow_Blue:
						{
							if (Entity)
							{
								Entity->Pos.z += 0.01f * WorldPosDelta.z;
								Entity->BasePos = Entity->Pos;
							}
							else
							{
								Assert(PointLight);
								PointLight->Pos.z += 0.01f * WorldPosDelta.z;
							}
						} break;
						
						case SelectedArrow_RedTargetOffset:
						{
							if (Entity->Type == Entity_Turret)
							{
								Entity->TargetOffset.x += 0.01f * Dot(WorldPosDelta, RotateByQuaternion(Vec3(1.0f, 0.0f, 0.0f), EulerToQuat(Entity->Orientation.xyz)));
							}
							else
							{
								Entity->TargetOffset.x += 0.01f * WorldPosDelta.x;
							}
						} break;
						
						case SelectedArrow_GreenTargetOffset:
						{
							Entity->TargetOffset.y += 0.01f * WorldPosDelta.y;
						} break;
						
						case SelectedArrow_BlueTargetOffset:
						{
							if (Entity->Type == Entity_Turret)
							{
								Entity->TargetOffset.z += 0.01f * Dot(WorldPosDelta, RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), EulerToQuat(Entity->Orientation.xyz)));
							}
							else
							{
								Entity->TargetOffset.z += 0.01f * WorldPosDelta.z;
							}
						} break;
					}
				}
				
				if (WasReleased(GameInput->Buttons[Button_MouseLeft]) && (EditorState->SelectedArrow != SelectedArrow_None))
				{
					EditorState->SelectedArrow = SelectedArrow_None;
					PlatformEnableCursor(GameInput);
				}
			} break;
			
			case EditorHelpersMode_Rotation:
			{
				if (Entity)
				{
					Renderer->DebugRenderPass.DrawDebugCircle(Entity->Pos, Max(Entity->Dim.x, Entity->Dim.z), (bCircleHit || EditorState->bCircleSelected) ? Vec3(0.8f, 1.0f, 0.8f) : Vec3(0.0f, 1.0f, 0.0f));
					
					if (WasDown(GameInput->Buttons[Button_MouseLeft]) && bCircleHit)
					{
						SaveLevelHistory(EditorState, Level);
						
						EditorState->bCircleSelected = true;
						PlatformDisableCursor(GameInput);
					}
					
					if (EditorState->bCircleSelected)
					{
						float RotationDelta = 0.2f * GameInput->MouseDeltaX;
						Entity->Orientation.y += RotationDelta;
					}
					
					if (WasReleased(GameInput->Buttons[Button_MouseLeft]) && EditorState->bCircleSelected)
					{
						EditorState->bCircleSelected = false;
						PlatformEnableCursor(GameInput);
					}
				}
			} break;
			
			case EditorHelpersMode_Scale:
			{
				if (Entity)
				{
					float RotationAngle = Entity->Orientation.y;
					
					vec4 RedRotation = QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz) * Quat(Vec3(0.0f, 1.0f, 0.0f), 90.0f));
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, DimHelperDim, (bRedDimHelperHit || (EditorState->SelectedDimHelper == SelectedDimHelper_Red)) ? Vec3(1.0f, 0.8f, 0.8f) : Vec3(1.0f, 0.0f, 0.0f), RedRotation, Vec3(0.0f, 0.0f, 0.5f * DimHelperDim.z), true);
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, DimHelperCubeDim, (bRedDimHelperHit || (EditorState->SelectedDimHelper == SelectedDimHelper_Red)) ? Vec3(1.0f, 0.8f, 0.8f) : Vec3(1.0f, 0.0f, 0.0f), RedRotation, Vec3(0.0f, 0.0f, DimHelperDim.z), true);
					
					vec4 GreenRotation = QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz) * Quat(Vec3(1.0f, 0.0f, 0.0f), -90.0f));
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, DimHelperDim, (bGreenDimHelperHit || (EditorState->SelectedDimHelper == SelectedDimHelper_Green)) ? Vec3(0.8f, 1.0f, 0.8f) : Vec3(0.0f, 1.0f, 0.0f), GreenRotation, Vec3(0.0f, 0.0f, 0.5f * DimHelperDim.z), true);
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, DimHelperCubeDim, (bGreenDimHelperHit || (EditorState->SelectedDimHelper == SelectedDimHelper_Green)) ? Vec3(0.8f, 1.0f, 0.8f) : Vec3(0.0f, 1.0f, 0.0f), GreenRotation, Vec3(0.0f, 0.0f, DimHelperDim.z), true);
					
					vec4 BlueRotation = QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz));
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, DimHelperDim, (bBlueDimHelperHit || (EditorState->SelectedDimHelper == SelectedDimHelper_Blue)) ? Vec3(0.8f, 0.8f, 1.0f) : Vec3(0.0f, 0.0f, 1.0f), BlueRotation, Vec3(0.0f, 0.0f, 0.5f * DimHelperDim.z), true);
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, DimHelperCubeDim, (bBlueDimHelperHit || (EditorState->SelectedDimHelper == SelectedDimHelper_Blue)) ? Vec3(0.8f, 0.8f, 1.0f) : Vec3(0.0f, 0.0f, 1.0f), BlueRotation, Vec3(0.0f, 0.0f, DimHelperDim.z), true);
					
					if (WasDown(GameInput->Buttons[Button_MouseLeft]) && (bRedDimHelperHit || bGreenDimHelperHit || bBlueDimHelperHit))
					{
						SaveLevelHistory(EditorState, Level);
						
						EditorState->SelectedDimHelper = bRedDimHelperHit ? SelectedDimHelper_Red : (bGreenDimHelperHit ? SelectedDimHelper_Green : SelectedDimHelper_Blue);
						PlatformDisableCursor(GameInput);
					}
					
					if (EditorState->SelectedDimHelper != SelectedDimHelper_None)
					{
						vec4 ScreenPos0 = Vec4(2.0f * (float(GameInput->PlatformMouseX) / Vulkan->Width), 2.0f * (float(GameInput->PlatformMouseY) / Vulkan->Height), 1.0f, 1.0f);
						vec4 WorldPos0 = Reproj * ScreenPos0;
						WorldPos0.xyz *= 1.0f / WorldPos0.w;
						
						vec4 ScreenPos1 = Vec4(2.0f * (float(GameInput->PlatformMouseX + GameInput->MouseDeltaX) / Vulkan->Width), 2.0f * (float(GameInput->PlatformMouseY - GameInput->MouseDeltaY) / Vulkan->Height), 1.0f, 1.0f);
						vec4 WorldPos1 = Reproj * ScreenPos1;
						WorldPos1.xyz *= 1.0f / WorldPos1.w;
						
						vec3 WorldPosDelta = WorldPos1.xyz - WorldPos0.xyz;
						switch (EditorState->SelectedDimHelper)
						{
							case SelectedDimHelper_Red:
							{
								float Delta = Dot(WorldPosDelta, RotateByQuaternion(Vec3(1.0f, 0.0f, 0.0f), EulerToQuat(Entity->Orientation.xyz)));
								Entity->Dim.x += 0.01f * Delta;
							} break;
							
							case SelectedDimHelper_Green:
							{
								float Delta = Dot(WorldPosDelta, Vec3(0.0f, 1.0f, 0.0f));
								Entity->Dim.y += 0.01f * Delta;
							} break;
							
							case SelectedDimHelper_Blue:
							{
								float Delta = Dot(WorldPosDelta, RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), EulerToQuat(Entity->Orientation.xyz)));
								Entity->Dim.z += 0.01f * Delta;
							} break;
						}
					}
					
					if (WasReleased(GameInput->Buttons[Button_MouseLeft]) && (EditorState->SelectedDimHelper != SelectedDimHelper_None))
					{
						EditorState->SelectedDimHelper = SelectedDimHelper_None;
						PlatformEnableCursor((SGameInput*) &GameInput);
					}
				}
			} break;
		}
		
		if (Entity)
		{
			Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, Entity->Dim, Entity->Color, QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz)));
			
			if (HasTargetPos(Entity))
			{
				if (Entity->Type == Entity_Turret)
				{
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos, Entity->FireballDim, Entity->Color, QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz)), Entity->TargetOffset);
				}
				else
				{
					Renderer->DebugRenderPass.DrawDebugBox(Entity->Pos + Entity->TargetOffset, Entity->Dim, Entity->Color, QuaternionToAxisAngle(EulerToQuat(Entity->Orientation.xyz)));
				}
			}
		}
		
		if (WasDown(GameInput->Buttons[Button_Delete]))
		{
			SaveLevelHistory(EditorState, Level);
			if (Entity)
			{
				Entity->bRemoved = true;
			}
			else
			{
				Assert(PointLight);
				*PointLight = Level->PointLights[--Level->PointLightCount];
			}
			
			EditorState->SelectedEntity = 0;
			EditorState->SelectedPointLight = 0;
		}
	}
	
	for (uint32_t I = 0; I < Level->PointLightCount; I++)
	{
		const SPointLight* PointLight = Level->PointLights + I;
		
		Renderer->DebugRenderPass.DrawDebugSphere(PointLight->Pos, 0.1f, Vec3(1.0f, 1.0f, 1.0f));
		if (PointLight == EditorState->SelectedPointLight)
		{
			Renderer->DebugRenderPass.DrawDebugSphere(PointLight->Pos, PointLight->Radius, PointLight->Color.rgb);
		}				
	}
	
	// Handle Ctrl+Z stuff
	if (GameInput->Buttons[Button_Control].IsDown && WasDown(GameInput->Buttons[Button_Z]))
	{
		if (EditorState->LevelHistoryTail != EditorState->LevelHistoryHead)
		{
			uint32_t LastIndex;
			if (EditorState->LevelHistoryHead > 0)
			{
				LastIndex = --EditorState->LevelHistoryHead;
			}
			else
			{
				LastIndex = ArrayCount(EditorState->LevelHistory) - 1;
				EditorState->LevelHistoryHead = LastIndex;
			}
			
			EngineState->Level = EditorState->LevelHistory[LastIndex];
			EngineState->bForceUpdateVoxels = true;
		}
	}
	
	if (EditorState->bImGuiChangeStarted)
	{
		EditorState->ImGuiChangeTimePassed += GameInput->dt;
		if (EditorState->ImGuiChangeTimePassed > 1.0f)
		{
			EditorState->bImGuiChangeStarted = false;
			EditorState->ImGuiChangeTimePassed = 0.0f;
		}
	}
}
#endif