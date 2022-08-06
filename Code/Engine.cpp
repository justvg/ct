#include "Engine.h"
#include "Game.h"
#include "Audio.cpp"
#include "Assets.cpp"
#include "Entity.cpp"
#include "Game.cpp"
#include "Editor.cpp"
#include "Renderer.cpp"

void UpdateVoxels(SEngineState* EngineState, const SVulkanContext& Vulkan, SLevel* Level)
{
	SRenderer* Renderer = &EngineState->Renderer;
	if (EngineState->VoxelsToDeleteCount || EngineState->VoxelsToChangeColorCount || EngineState->VoxelsToChangeReflectivityCount || EngineState->VoxelsToChangeRoughnessCount || EngineState->VoxelsToAddCount)
	{
#ifndef ENGINE_RELEASE
		if (EngineState->EngineMode == EngineMode_Editor)
		{
			if ((EngineState->VoxelsToDeleteCount > 0) || (EngineState->VoxelsToAddCount > 0))
			{
				SaveLevelHistory(&EngineState->EditorState, Level);
			}
		}
#endif
        
		VkBufferMemoryBarrier VoxelCullingBuffersBeginBarrier[] = 
		{
			CreateBufferMemoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, Renderer->VoxelDrawBuffer, Renderer->VoxelDrawBuffer.Size),
			CreateBufferMemoryBarrier(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, Renderer->VoxelVisibilityBuffer, Renderer->VoxelVisibilityBuffer.Size)
		};
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, ArrayCount(VoxelCullingBuffersBeginBarrier), VoxelCullingBuffersBeginBarrier, 0, 0);
        
		VkBufferMemoryBarrier VoxelBufferBeginBarrier = CreateBufferMemoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, Renderer->VoxelsBuffer, Renderer->VoxelsBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &VoxelBufferBeginBarrier , 0, 0);
        
		for (uint32_t I = 0; I < EngineState->VoxelsToDeleteCount; I++)
		{
			uint32_t ID = EngineState->VoxelsToDelete[I];
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
			uint32_t Z = ID / (LevelDimX*LevelDimY);
            
			// Remove game voxel
			SetVoxelColor(Level->Voxels, X, Y, Z, 0, 0, 0);
			SetVoxelReflectivity(Level->Voxels, X, Y, Z, 0);
			SetVoxelRoughness(Level->Voxels, X, Y, Z, 0);
			SetVoxelActive(Level->Voxels, X, Y, Z, false);
            
			// Remove render voxel
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorMatActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
            
			EngineState->VoxelDraws[ID].FirstInstance = UINT32_MAX;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelDrawBuffer, ID*sizeof(SVoxelDraw), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &EngineState->VoxelDraws[ID], sizeof(SVoxelDraw));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(SVoxelDraw);
            
			EngineState->VoxelVisibilities[ID] = 1;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelVisibilityBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &EngineState->VoxelVisibilities[ID], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		EngineState->VoxelsToDeleteCount = 0;
        
		for (uint32_t I = 0; I < EngineState->VoxelsToChangeColorCount; I++)
		{
			uint32_t ID = EngineState->VoxelsToChangeColor[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
			uint32_t Z = ID / (LevelDimY*LevelDimX);
            
			uint8_t Red = EngineState->VoxelsToChangeColor[I].Red;
			uint8_t Green = EngineState->VoxelsToChangeColor[I].Green;
			uint8_t Blue = EngineState->VoxelsToChangeColor[I].Blue;
            
			// Change game voxel color
			SetVoxelColor(Level->Voxels, X, Y, Z, Red, Green, Blue);
            
			// Update render voxel 
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorMatActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		EngineState->VoxelsToChangeColorCount = 0;

		for (uint32_t I = 0; I < EngineState->VoxelsToChangeReflectivityCount; I++)
		{
			uint32_t ID = EngineState->VoxelsToChangeReflectivity[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
			uint32_t Z = ID / (LevelDimY*LevelDimX);
            
			uint8_t Reflectivity = EngineState->VoxelsToChangeReflectivity[I].Reflectivity;
            
			// Change game voxel color
			SetVoxelReflectivity(Level->Voxels, X, Y, Z, Reflectivity);
            
			// Update render voxel 
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorMatActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		EngineState->VoxelsToChangeReflectivityCount = 0;

		for (uint32_t I = 0; I < EngineState->VoxelsToChangeRoughnessCount; I++)
		{
			uint32_t ID = EngineState->VoxelsToChangeRoughness[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
			uint32_t Z = ID / (LevelDimY*LevelDimX);
            
			uint8_t Roughness = EngineState->VoxelsToChangeRoughness[I].Roughness;
            
			// Change game voxel color
			SetVoxelRoughness(Level->Voxels, X, Y, Z, Roughness);
            
			// Update render voxel 
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorMatActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		EngineState->VoxelsToChangeRoughnessCount = 0;
        
		for (uint32_t I = 0; I < EngineState->VoxelsToAddCount; I++)
		{
			uint32_t ID = EngineState->VoxelsToAdd[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimY*LevelDimX)) / LevelDimX;
			uint32_t Z = ID / (LevelDimX*LevelDimY);
            
			uint8_t Red = EngineState->VoxelsToAdd[I].Red;
			uint8_t Green = EngineState->VoxelsToAdd[I].Green;
			uint8_t Blue = EngineState->VoxelsToAdd[I].Blue;
			
			uint8_t Reflectivity = EngineState->VoxelsToAdd[I].Reflectivity;
			uint8_t Roughness = EngineState->VoxelsToAdd[I].Roughness;
            
			SetVoxelActive(Level->Voxels, X, Y, Z, true);
			SetVoxelColor(Level->Voxels, X, Y, Z, Red, Green, Blue);
			SetVoxelReflectivity(Level->Voxels, X, Y, Z, Reflectivity);
			SetVoxelRoughness(Level->Voxels, X, Y, Z, Roughness);
            
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorMatActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
            
			EngineState->VoxelDraws[ID].FirstInstance = ID;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelDrawBuffer, ID*sizeof(SVoxelDraw), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &EngineState->VoxelDraws[ID], sizeof(SVoxelDraw));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(SVoxelDraw);
            
			EngineState->VoxelVisibilities[ID] = 1;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelVisibilityBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &EngineState->VoxelVisibilities[ID], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		EngineState->VoxelsToAddCount = 0;
        
		VkBufferMemoryBarrier VoxelBufferEndBarrier = CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, Renderer->VoxelsBuffer, Renderer->VoxelsBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 1, &VoxelBufferEndBarrier , 0, 0);
        
		VkBufferMemoryBarrier VoxelCullingBuffersEndBarrier[] = 
		{
			CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, Renderer->VoxelDrawBuffer, Renderer->VoxelDrawBuffer.Size),
			CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, Renderer->VoxelVisibilityBuffer, Renderer->VoxelVisibilityBuffer.Size)
		};
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, ArrayCount(VoxelCullingBuffersEndBarrier), VoxelCullingBuffersEndBarrier, 0, 0);
	}
}

VkImage GameUpdateAndRender(SVulkanContext& Vulkan, SGameMemory* GameMemory, const SGameInput& GameInput, const SGameSoundBuffer& SoundBuffer)
{
	Assert((sizeof(SEngineState) + sizeof(SGameState)) <= GameMemory->StorageSize);

    SEngineState* EngineState = (SEngineState*) GameMemory->Storage;
    SGameState* GameState = (SGameState*) ((uint8_t*) GameMemory->Storage + sizeof(SEngineState));
    if (!EngineState->bInitialized)
    {
		InitializeMemoryArena(&EngineState->MemoryArena, (uint8_t*) GameMemory->Storage + sizeof(SEngineState) + sizeof(SGameState), GameMemory->StorageSize - sizeof(SEngineState) - sizeof(SGameState));

		CreateVoxelMesh(EngineState->Geometry);
		AddGLTFMesh(EngineState->Geometry, "Models\\cube.gltf");
		CreateQuadMesh(EngineState->Geometry);
		CreateSphereMesh(EngineState->Geometry);
        
		InitializeRenderer(&EngineState->Renderer, Vulkan, &EngineState->Geometry);
                    
		EngineState->Camera.OffsetFromPlayer = Vec3(0.0f, 0.8f*0.5f*1.5f, 0.0f);
        EngineState->Camera.Near = 0.01f;
        EngineState->Camera.Far = 300.0f;
        EngineState->Camera.FoV = 70.0f;

		EngineState->FullscreenResolutionPercent = 100;

		EngineState->bVignetteEnabled = true;

		SParsedConfigFile ConfigFile = ParseConfigFile("Engine.cfg");
		if (ConfigFile.ItemCount > 0)
		{
			for (uint32_t I = 0; I < ConfigFile.ItemCount; I++)
			{
				const SConfigFileItem& Item = ConfigFile.Items[I];

			#ifndef ENGINE_RELEASE
				if (CompareStrings(Item.Name, "CameraSpeed"))
				{
					EngineState->EditorState.CameraSpeed = Item.Value;
				}
			#endif
			}
		}
		else
		{
			// NOTE(georgii): Use these values if for some reason there is no config file.
		#ifndef ENGINE_RELEASE
		    EngineState->EditorState.CameraSpeed = 1.0f;
        #endif
		}

		EngineState->AudioState.MasterVolume = Vec2(1.0f, 1.0f);
		EngineState->LoadedSounds[Sound_Footstep0] = LoadWAV("Sounds\\Footsteps\\Footstep0.wav");
		EngineState->LoadedSounds[Sound_Footstep1] = LoadWAV("Sounds\\Footsteps\\Footstep1.wav");
		EngineState->LoadedSounds[Sound_Footstep2] = LoadWAV("Sounds\\Footsteps\\Footstep2.wav");
		EngineState->LoadedSounds[Sound_Footstep3] = LoadWAV("Sounds\\Footsteps\\Footstep3.wav");
		EngineState->LoadedSounds[Sound_Footstep4] = LoadWAV("Sounds\\Footsteps\\Footstep4.wav");
		EngineState->LoadedSounds[Sound_SuccessColor] = LoadWAV("Sounds\\Effects\\SuccessColor.wav");
		EngineState->LoadedSounds[Sound_NegativeColor] = LoadWAV("Sounds\\Effects\\NegativeColor.wav");
		EngineState->LoadedSounds[Sound_Portal] = LoadWAV("Sounds\\Effects\\Portal.wav");

        EngineState->bInitialized = true;
    }
    
	bool bSwapchainChanged = false;
	if (Vulkan.bSwapchainChanged || EngineState->bSwapchainChanged)
	{
		if (EngineState->bSwapchainChanged)
		{
			Vulkan.InternalWidth = uint32_t(Vulkan.Width * (EngineState->FullscreenResolutionPercent / 100.0f));
			Vulkan.InternalHeight = uint32_t(Vulkan.Height * (EngineState->FullscreenResolutionPercent / 100.0f));
		}

		RendererHandleChanges(&EngineState->Renderer, Vulkan);
		EngineState->bSwapchainChanged = false;

		bSwapchainChanged = true;
	}

	if (PlatformGetFullscreen())
	{
		EngineState->LastFullscreenInternalWidth = Vulkan.InternalWidth;
		EngineState->LastFullscreenInternalHeight = Vulkan.InternalHeight;
	}
	
	if (EngineState->bReloadLevel || EngineState->bReloadLevelEditor || EngineState->bForceUpdateVoxels)
	{
		if (EngineState->bReloadLevel)
		{
			EngineState->Level = EngineState->LevelBaseState;
		}
		if (EngineState->bReloadLevelEditor)
		{
			EngineState->Level = EngineState->LevelGameStartState;
#ifndef ENGINE_RELEASE
			EngineState->EditorState.LevelHistoryHead = EngineState->EditorState.LevelHistoryTail = 0;
			SaveLevelHistory(&EngineState->EditorState, &EngineState->LevelGameStartState);
#endif
		}

		memset(EngineState->VoxelDraws, 0, sizeof(EngineState->VoxelDraws));
		for (uint32_t Z = 0; Z < LevelDimZ; Z++)
		{
			for (uint32_t Y = 0; Y < LevelDimY; Y++)
			{
				for (uint32_t X = 0; X < LevelDimX; X++)
				{
					SVoxelDraw* VoxelDraw = &EngineState->VoxelDraws[Z*LevelDimX*LevelDimY + Y*LevelDimX + X];
                    
					VoxelDraw->Pos = Vec3i(X, Y, Z) * VoxelDim;
					VoxelDraw->FirstInstance = IsVoxelActive(EngineState->Level.Voxels, X, Y, Z) ? Z*LevelDimX*LevelDimY + Y*LevelDimX + X : UINT32_MAX;
                    
					EngineState->VoxelVisibilities[Z*LevelDimX*LevelDimY + Y*LevelDimX + X] = 1;
				}
			}
		}
        
		VkCheck(vkDeviceWaitIdle(Vulkan.Device));
        
		SRenderer* Renderer = &EngineState->Renderer;
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelDrawBuffer, Renderer->StagingBuffers[0], EngineState->VoxelDraws, sizeof(EngineState->VoxelDraws));
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelVisibilityBuffer, Renderer->StagingBuffers[0], EngineState->VoxelVisibilities, sizeof(EngineState->VoxelVisibilities));
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelsBuffer, Renderer->StagingBuffers[0], EngineState->Level.Voxels.ColorMatActive, sizeof(EngineState->Level.Voxels));
		
		GameState->bReload = EngineState->bReloadGame;

		EngineState->bReloadLevel = EngineState->bReloadLevelEditor = EngineState->bForceUpdateVoxels = false;
		EngineState->bReloadGame = false;

		EngineState->GameTime = 0.0f;
	}
    
	SCamera& Camera = EngineState->Camera;
	SLevel* Level = &EngineState->Level;
    
	switch (EngineState->EngineMode)
	{
		case EngineMode_Game:
		{
			UpdateGame(GameState, EngineState, &GameInput, &Vulkan);
		} break;
        
#ifndef ENGINE_RELEASE
		case EngineMode_Editor:
		{
			UpdateEditor(EngineState, (SGameInput*) &GameInput, &Vulkan, &EngineState->Renderer);
		} break;
#endif
	}
    
#ifndef ENGINE_RELEASE
	if (WasDown(GameInput.Buttons[Button_F1]))
    {
		PlatformToggleCursor((SGameInput*) &GameInput, !PlatformIsCursorEnabled() || EngineState->bMenuOpened, !PlatformIsCursorShowed());
    }
#endif
    
	const uint32_t FrameInFlight = Vulkan.FrameInFlight;
	UpdateCameraRenderData(&EngineState->Renderer, Camera, Vulkan.Width, Vulkan.Height, GameInput.FrameID, FrameInFlight);
    
	SLightBuffer* LightBufferGPU = (SLightBuffer*) EngineState->Renderer.LightBuffers[Vulkan.FrameInFlight].Data;
	LightBufferGPU->AmbientColor = Vec4(Level->AmbientColor, 0.0f);
	LightBufferGPU->AmbientConstant = Vec4(Level->AmbientConstant, 0.0f);
    
	const SCameraBuffer& CameraBufferData = EngineState->Renderer.CameraBufferData;
	SPointLight* PointLightsGPU = (SPointLight*) EngineState->Renderer.PointLightsBuffers[Vulkan.FrameInFlight].Data;
	uint32_t PointLightCount = 0;
	for (uint32_t I = 0; I < Level->PointLightCount; I++)
	{
		PointLightsGPU[PointLightCount++] = Level->PointLights[I];
	}
    
	for (uint32_t I = 0; I < Level->EntityCount; I++)
	{
		const SEntity* Entity = Level->Entities + I;
        
		if (Entity->Type != Entity_MessageToggler)
		{
			if (Entity->PointLight.Radius > 0.0f)
			{
				Assert(PointLightCount < ArrayCount(Level->PointLights));
				
				PointLightsGPU[PointLightCount].Pos = Entity->Pos + Entity->PointLight.Pos;
				PointLightsGPU[PointLightCount].Radius = Entity->PointLight.Radius;
				PointLightsGPU[PointLightCount].Color = Entity->PointLight.Color;
				
				if ((Entity->Type == Entity_Torch) || (Entity->Type == Entity_Container))
				{
					PointLightsGPU[PointLightCount].Color.rgb = Entity->Color;
				}
				
				if (LengthSq(PointLightsGPU[PointLightCount].Color.rgb) > 0.0f)
				{
					PointLightCount++;
				}
			}
		}
	}

	uint32_t PointLightInFrustumCount = 0;
	bool PointLightsCulling[ArrayCount(Level->PointLights)];
	for (uint32_t I = 0; I < PointLightCount; I++)
	{
		bool bInsideFrustum = true;
		for (uint32_t J = 0; (J < ArrayCount(CameraBufferData.Frustums)) && bInsideFrustum; J++)
		{
			bInsideFrustum = Dot(Vec4(PointLightsGPU[I].Pos, -1.0f), CameraBufferData.Frustums[J]) >= -PointLightsGPU[I].Radius;
		}

		PointLightsCulling[I] = bInsideFrustum;
		if (bInsideFrustum)
		{
			PointLightInFrustumCount++;
		}
	}

	// Bubble sort
	for (uint32_t I = 0; I < PointLightCount; I++)
	{
		bool bSwapped = false;
		for (uint32_t J = 0; J < PointLightCount - 1 - I; J++)
		{
			if (!PointLightsCulling[J] && PointLightsCulling[J + 1])
			{
				bool TempBool = PointLightsCulling[J];
				PointLightsCulling[J] = PointLightsCulling[J + 1];
				PointLightsCulling[J + 1] = TempBool;

				SPointLight TempLight = PointLightsGPU[J];
				PointLightsGPU[J] = PointLightsGPU[J + 1];
				PointLightsGPU[J + 1] = TempLight;

				bSwapped = true;
			}
		}

		if (!bSwapped)
		{
			break;
		}
	}


	VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkCheck(vkBeginCommandBuffer(Vulkan.CommandBuffer, &CommandBufferBeginInfo));
    
	CLEAR_GPU_PROFILER_INFO(Vulkan.CommandBuffer, FrameInFlight);
    
	EngineState->Renderer.StagingBufferOffsets[FrameInFlight] = 0;
    
	// NOTE(georgii): Delete all voxels that were marked for deletion. Add all voxels that were added. Chnage all voxels that were updated. Update appropriate GPU buffers.
	UpdateVoxels(EngineState, Vulkan, Level);

	// Sound mixing
	STempMemoryArena SoundTempMemory = BeginTempMemoryArena(&EngineState->MemoryArena);
	OutputPlayingSounds(&EngineState->AudioState, SoundBuffer, EngineState->LoadedSounds, &SoundTempMemory);
	EndTempMemoryArena(&SoundTempMemory);
    
	// Render
	vec2 MousePos = Vec2(float(GameInput.PlatformMouseX), float(GameInput.PlatformMouseY));

	STempMemoryArena RenderTempMemory = BeginTempMemoryArena(&EngineState->MemoryArena);
	VkImage FinalImage = RenderScene(EngineState, &EngineState->Renderer, Vulkan, Level, PointLightInFrustumCount, PointLightCount, GameInput.FrameID, &RenderTempMemory, EngineState->GameTime, bSwapchainChanged, MousePos);
	EndTempMemoryArena(&RenderTempMemory);

	// Update text stuff
	for (uint32_t I = 0; I < EngineState->TextsToRenderCount;)
	{
		SText* Text = EngineState->TextsToRender + I;

		if ((EngineState->bMenuOpened && Text->bMenuText) || (!EngineState->bMenuOpened && !Text->bMenuText))
		{
			Text->CurrentTime += GameInput.dt;
			if (Text->CurrentTime > Text->Time)
			{
				*Text = EngineState->TextsToRender[--EngineState->TextsToRenderCount];
			}
			else
			{
				I++;
			}
		}
		else
		{
			I++;
		}
	}

	// Delete all entities marked for deletion and fix all indexes.
	for (uint32_t EntityIndex = 0; EntityIndex < Level->EntityCount;)
	{
		SEntity* Entity = Level->Entities + EntityIndex;

		if (Entity->bRemoved)
		{
			if (Entity->Type == Entity_Door)
			{
				FixDoorIndex(Level, EntityIndex, 0);
			}

			if (Level->Entities[Level->EntityCount - 1].Type == Entity_Door)
			{
				FixDoorIndex(Level, Level->EntityCount - 1, EntityIndex);
			}

			Level->Entities[EntityIndex] = Level->Entities[--Level->EntityCount];
		}
		else
		{
			EntityIndex++;
		}
	}

	if (!EngineState->bMenuOpened)
	{
		EngineState->GameTime += GameInput.dt;
	}
    
	return FinalImage;
}