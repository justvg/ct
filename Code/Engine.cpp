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
                            
		EngineState->Camera.OffsetFromPlayer = Vec3(0.0f, 0.8f*0.5f*1.5f, 0.0f);
        EngineState->Camera.Near = 0.01f;
        EngineState->Camera.Far = 300.0f;
        EngineState->Camera.FoV = 70.0f;

		EngineState->FullscreenResolutionPercent = 100;

		EngineState->bVignetteEnabled = true;

		EngineState->AudioState.MasterVolume = 50;
		EngineState->AudioState.MusicVolume = 50;
		EngineState->AudioState.EffectsVolume = 50;
		EngineState->LoadedSounds[Sound_Footstep0] = LoadWAV("Sounds\\Footsteps\\Footstep0.wav");
		EngineState->LoadedSounds[Sound_Footstep1] = LoadWAV("Sounds\\Footsteps\\Footstep1.wav");
		EngineState->LoadedSounds[Sound_Footstep2] = LoadWAV("Sounds\\Footsteps\\Footstep2.wav");
		EngineState->LoadedSounds[Sound_Footstep3] = LoadWAV("Sounds\\Footsteps\\Footstep3.wav");
		EngineState->LoadedSounds[Sound_Footstep4] = LoadWAV("Sounds\\Footsteps\\Footstep4.wav");
		EngineState->LoadedSounds[Sound_SuccessColor] = LoadWAV("Sounds\\Effects\\SuccessColor.wav");
		EngineState->LoadedSounds[Sound_NegativeColor] = LoadWAV("Sounds\\Effects\\NegativeColor.wav");
		EngineState->LoadedSounds[Sound_Portal] = LoadWAV("Sounds\\Effects\\Portal.wav");
		EngineState->LoadedSounds[Sound_PortalAmbient] = LoadWAV("Sounds\\Effects\\PortalAmbient.wav");
		EngineState->LoadedSounds[Sound_Turret0] = LoadWAV("Sounds\\Effects\\Turret0.wav");
		EngineState->LoadedSounds[Sound_Turret1] = LoadWAV("Sounds\\Effects\\Turret1.wav");
		EngineState->LoadedSounds[Sound_Turret2] = LoadWAV("Sounds\\Effects\\Turret2.wav");
		EngineState->LoadedSounds[Sound_ColorFieldAmbient] = LoadWAV("Sounds\\Effects\\ColorFieldAmbient.wav");

		EngineState->LoadedSounds[Sound_PortalSoundtrack] = LoadWAV("Sounds\\Music\\PortalSoundtrack.wav");

	#ifndef ENGINE_RELEASE
		// Load and set editor config
		SParsedConfigFile ConfigFile = ParseConfigFile("Editor.cfg");
		if (ConfigFile.ItemCount > 0)
		{
			for (uint32_t I = 0; I < ConfigFile.ItemCount; I++)
			{
				const SConfigFileItem& Item = ConfigFile.Items[I];

				if (CompareStrings(Item.Name, "CameraSpeed"))
				{
					EngineState->EditorState.CameraSpeed = Item.Value;
				}
			}
		}
		else
		{
			// NOTE(georgii): Use these values if for some reason there is no config file.
		    EngineState->EditorState.CameraSpeed = 1.0f;
		}
	#endif

	#if ENGINE_RELEASE
		// Load saved and set engine data
		SGeneralSaveData GeneralSaveData = LoadGeneralSaveData();
		if (GeneralSaveData.bValid)
		{
			PlatformChangeVSync(GeneralSaveData.bVSync);
			EngineState->bVignetteEnabled = GeneralSaveData.bVignetteEnabled;
			EngineState->Renderer.AOQuality = EAOQuality(GeneralSaveData.AOQuality);
			EngineState->FullscreenResolutionPercent = GeneralSaveData.FullscreenResolutionPercent;
			EngineState->AudioState.MasterVolume = GeneralSaveData.MasterVolume;
			EngineState->AudioState.MusicVolume = GeneralSaveData.MusicVolume;
			EngineState->AudioState.EffectsVolume = GeneralSaveData.EffectsVolume;

			if (PlatformGetFullscreen())
			{
				Vulkan.InternalWidth = uint32_t(Vulkan.Width * (EngineState->FullscreenResolutionPercent / 100.0f));
				Vulkan.InternalHeight = uint32_t(Vulkan.Height * (EngineState->FullscreenResolutionPercent / 100.0f));
			}

			FreeGeneralSaveData(&GeneralSaveData);
		}
	#endif

		InitializeRenderer(&EngineState->Renderer, Vulkan, &EngineState->Geometry);

		PlatformCreateThread(WriteDataToDiskThreadFunction, &EngineState->WriteDiskThread);

        EngineState->bInitialized = true;
    }
    
	bool bSwapchainChanged = false;
	if (Vulkan.bSwapchainChanged || EngineState->bSwapchainChanged)
	{
		if (EngineState->bSwapchainChanged)
		{
			if (PlatformGetFullscreen())
			{
				Vulkan.InternalWidth = uint32_t(Vulkan.Width * (EngineState->FullscreenResolutionPercent / 100.0f));
				Vulkan.InternalHeight = uint32_t(Vulkan.Height * (EngineState->FullscreenResolutionPercent / 100.0f));
			}
		}

		RendererHandleChanges(&EngineState->Renderer, Vulkan);
		EngineState->bSwapchainChanged = false;

		bSwapchainChanged = true;
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
	SLight* LightsGPU = (SLight*) EngineState->Renderer.LightsBuffers[Vulkan.FrameInFlight].Data;
	uint32_t LightCount = 0;
	for (uint32_t I = 0; I < Level->LightCount; I++)
	{
		LightsGPU[LightCount] = Level->Lights[I];
		LightsGPU[LightCount].Direction = RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), EulerToQuat(Level->Lights[I].Rotation));

		LightCount++;
	}
    
	for (uint32_t I = 0; I < Level->EntityCount; I++)
	{
		const SEntity* Entity = Level->Entities + I;
		
		if (Entity->Light.Radius > 0.0f)
		{
			Assert(LightCount < ArrayCount(Level->Lights));
			
			LightsGPU[LightCount] = Entity->Light;
			LightsGPU[LightCount].Pos = Entity->Pos + Entity->Light.Pos;

			if (Entity->Light.Type == Light_Spot)
			{
				LightsGPU[LightCount].Direction = RotateByQuaternion(Vec3(0.0f, 0.0f, 1.0f), EulerToQuat(Entity->Light.Rotation));
			}
			
			if ((Entity->Type == Entity_Torch) || (Entity->Type == Entity_Container))
			{
				LightsGPU[LightCount].Color = Entity->Color;
			}
			
			if (LengthSq(LightsGPU[LightCount].Color) > 0.0f)
			{
				LightCount++;
			}
		}
	}

	uint32_t LightInFrustumCount = 0;
	bool LightsCulling[ArrayCount(Level->Lights)];
	for (uint32_t I = 0; I < LightCount; I++)
	{
		bool bInsideFrustum = true;
		for (uint32_t J = 0; (J < ArrayCount(CameraBufferData.Frustums)) && bInsideFrustum; J++)
		{
			bInsideFrustum = Dot(Vec4(LightsGPU[I].Pos, -1.0f), CameraBufferData.Frustums[J]) >= -LightsGPU[I].Radius;
		}

		LightsCulling[I] = bInsideFrustum;
		if (bInsideFrustum)
		{
			LightInFrustumCount++;
		}
	}

	// Bubble sort
	for (uint32_t I = 0; I < LightCount; I++)
	{
		bool bSwapped = false;
		for (uint32_t J = 0; J < LightCount - 1 - I; J++)
		{
			if (!LightsCulling[J] && LightsCulling[J + 1])
			{
				bool TempBool = LightsCulling[J];
				LightsCulling[J] = LightsCulling[J + 1];
				LightsCulling[J + 1] = TempBool;

				SLight TempLight = LightsGPU[J];
				LightsGPU[J] = LightsGPU[J + 1];
				LightsGPU[J + 1] = TempLight;

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
	OutputPlayingSounds(&EngineState->AudioState, SoundBuffer, EngineState->LoadedSounds, &SoundTempMemory, Camera.Pos, Level);
	EndTempMemoryArena(&SoundTempMemory);
    
	// Render
// #ifndef ENGINE_RELEASE
	char FrameTimeText[64];
	const float FrameMS = uint32_t(10000.0f * GameInput.dt) / 10.0f;
	snprintf(FrameTimeText, sizeof(FrameTimeText), "%.1fms/f %dFPS\n\n", FrameMS, (uint32_t)(1000.0f / FrameMS));
	AddTextOneFrame(EngineState, FrameTimeText, Vec2(-1.0f, -0.95f), 0.1f, Vec4(1.0f), TextAlignment_Left);
// #endif

	vec2 MousePos = Vec2(float(GameInput.PlatformMouseX), float(GameInput.PlatformMouseY));

	STempMemoryArena RenderTempMemory = BeginTempMemoryArena(&EngineState->MemoryArena);
	VkImage FinalImage = RenderScene(EngineState, &EngineState->Renderer, Vulkan, Level, LightInFrustumCount, LightCount, GameInput.FrameID, &RenderTempMemory, EngineState->GameTime, bSwapchainChanged, MousePos);
	EndTempMemoryArena(&RenderTempMemory);

	// Update text stuff
	for (uint32_t I = 0; I < EngineState->TextsToRenderCount;)
	{
		SText* Text = EngineState->TextsToRender + I;

		if ((EngineState->bMenuOpened && Text->bMenuText) || (!EngineState->bMenuOpened && !Text->bMenuText) || (Text->Time == 0.0f))
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

#if ENGINE_RELEASE
	SaveGeneralSaveData(EngineState);
#endif
    
	return FinalImage;
}

void EngineFinish(SGameMemory* GameMemory)
{
	Assert((sizeof(SEngineState) + sizeof(SGameState)) <= GameMemory->StorageSize);

    SEngineState* EngineState = (SEngineState*) GameMemory->Storage;
    if (EngineState->bInitialized)
    {
		// NOTE(georgii): Wait until the thread is finished with its job
		while (EngineState->WriteDiskThread.EntryToRead != EngineState->WriteDiskThread.EntryToWrite) { }
	}
}