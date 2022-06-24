#include "Engine.h"
#include "Game.h"
#include "Audio.cpp"
#include "Assets.cpp"
#include "Particles.cpp"
#include "Entity.cpp"
#include "Game.cpp"
#include "Editor.cpp"
#include "Renderer.cpp"

void UpdateVoxels(SEngineState* EngineState, const SVulkanContext& Vulkan, SLevel* Level)
{
	SRenderer* Renderer = &EngineState->Renderer;
	if (EngineState->VoxelsToDeleteCount || EngineState->VoxelsToChangeColorCount || EngineState->VoxelsToAddCount)
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
			SetVoxelActive(Level->Voxels, X, Y, Z, false);
            
			// Remove render voxel
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorActive[Z][Y][X], sizeof(uint32_t));
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
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		EngineState->VoxelsToChangeColorCount = 0;
        
		for (uint32_t I = 0; I < EngineState->VoxelsToAddCount; I++)
		{
			uint32_t ID = EngineState->VoxelsToAdd[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimY*LevelDimX)) / LevelDimX;
			uint32_t Z = ID / (LevelDimX*LevelDimY);
            
			uint8_t Red = EngineState->VoxelsToAdd[I].Red;
			uint8_t Green = EngineState->VoxelsToAdd[I].Green;
			uint8_t Blue = EngineState->VoxelsToAdd[I].Blue;
            
			SetVoxelActive(Level->Voxels, X, Y, Z, true);
			SetVoxelColor(Level->Voxels, X, Y, Z, Red, Green, Blue);
            
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorActive[Z][Y][X], sizeof(uint32_t));
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

uint32_t UpdateParticles(SEngineState* EngineState, const SVulkanContext& Vulkan, SLevel* Level, float dt)
{
	SRenderer* Renderer = &EngineState->Renderer;

	uint32_t TotalParticleCount = SimulateParticles(Level->ParticleEmitters, Level->ParticleEmitterCount, dt);
	if (TotalParticleCount > 0)
	{
		EngineState->ParticlesDrawCount = TotalParticleCount;
        
		uint32_t ParticleDrawIndex = 0;
		for (uint32_t I = 0; I < Level->ParticleEmitterCount; I++)
		{
			SParticleEmitter* Emitter = &Level->ParticleEmitters[I];
            
			for (uint32_t J = 0; J < Emitter->ParticleCount; J++)
			{
				SParticle* Particle = &Emitter->Particles[J];
				SParticleDraw* Draw = &EngineState->ParticleDraws[ParticleDrawIndex++];
                
				Draw->Pos = Particle->Pos;
				Draw->Scale = Particle->Scale;
				Draw->Color = Particle->Color;
			}
		}
		Assert(ParticleDrawIndex == TotalParticleCount);
        
		VkBufferMemoryBarrier ParticleBufferBeginBarrier = CreateBufferMemoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, Renderer->ParticleDrawBuffer, Renderer->ParticleDrawBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &ParticleBufferBeginBarrier, 0, 0);
        
		UpdateBuffer(Vulkan.CommandBuffer, Renderer->ParticleDrawBuffer, 0, Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], EngineState->ParticleDraws, EngineState->ParticlesDrawCount*sizeof(SParticleDraw));
		Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += EngineState->ParticlesDrawCount*sizeof(SParticleDraw);
        
		VkBufferMemoryBarrier ParticleBufferEndBarrier = CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, Renderer->ParticleDrawBuffer, Renderer->ParticleDrawBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, 0, 1, &ParticleBufferEndBarrier, 0, 0);
	}
    
	return TotalParticleCount;
}

VkImage GameUpdateAndRender(const SVulkanContext& Vulkan, SGameMemory* GameMemory, const SGameInput& GameInput, const SGameSoundBuffer& SoundBuffer)
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
        
		InitializeRenderer(&EngineState->Renderer, Vulkan, &EngineState->Geometry);
            
		EngineState->LoadedSounds[0] = LoadWAV("Sounds\\example.wav");
		EngineState->LoadedSounds[1] = LoadWAV("Sounds\\shotgun.wav");
		EngineState->AudioState.MasterVolume = Vec2(1.0f, 1.0f);
        
		EngineState->Camera.OffsetFromPlayer = Vec3(0.0f, 0.8f*0.5f*1.5f, 0.0f);
        EngineState->Camera.Near = 0.01f;
        EngineState->Camera.Far = 300.0f;
        EngineState->Camera.FoV = 70.0f;

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

        EngineState->bInitialized = true;
    }
    
	bool bSwapchainChanged = false;
	if (Vulkan.bSwapchainChanged || EngineState->bSampleCountMSAAChanged)
	{
		if (EngineState->bSampleCountMSAAChanged)
		{
			((SVulkanContext&) Vulkan).SampleCountMSAA = VkSampleCountFlagBits(EngineState->NewSampleCountMSAA);
		}

		RendererHandleChanges(&EngineState->Renderer, Vulkan, EngineState->bSampleCountMSAAChanged);
		EngineState->bSampleCountMSAAChanged = false;

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
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelsBuffer, Renderer->StagingBuffers[0], EngineState->Level.Voxels.ColorActive, sizeof(EngineState->Level.Voxels));
		
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
		PlatformToggleCursorOnOff((SGameInput*) &GameInput);
    }
#endif
    
	const uint32_t FrameInFlight = Vulkan.FrameInFlight;
	UpdateCameraRenderData(&EngineState->Renderer, Camera, Vulkan.Width, Vulkan.Height, GameInput.FrameID, FrameInFlight);
    
	SLightBuffer* LightBufferGPU = (SLightBuffer*) EngineState->Renderer.LightBuffers[Vulkan.FrameInFlight].Data;
	LightBufferGPU->AmbientColor = Vec4(Level->AmbientColor, 0.0f);
	LightBufferGPU->AmbientConstant = Vec4(Level->AmbientConstant, 0.0f);
    
	SPointLight* PointLightsGPU = (SPointLight*) EngineState->Renderer.PointLightsBuffers[Vulkan.FrameInFlight].Data;
	for (uint32_t I = 0; I < Level->PointLightCount; I++)
	{
		PointLightsGPU[I] = Level->PointLights[I];
	}
    
	uint32_t PointLightCount = Level->PointLightCount;
	for (uint32_t I = 0; I < Level->EntityCount; I++)
	{
		const SEntity* Entity = Level->Entities + I;
        
		if (Entity->Type != Entity_MessageToggler)
		{
			if (((LengthSq(Entity->PointLight.Color.rgb) > 0.0f) && (Entity->PointLight.Radius > 0.0f)))
			{
				Assert(PointLightCount < ArrayCount(Level->PointLights));
				
				PointLightsGPU[PointLightCount].Pos = Entity->Pos + Entity->PointLight.Pos;
				PointLightsGPU[PointLightCount].Radius = Entity->PointLight.Radius;
				PointLightsGPU[PointLightCount].Color = Entity->PointLight.Color;
				
				if ((Entity->Type == Entity_Torch) || (Entity->Type == Entity_Container))
				{
					PointLightsGPU[PointLightCount].Color.rgb = 8.0f * Entity->Color;
				}
				
				PointLightCount++;
			}
		}
	}
    
	VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkCheck(vkBeginCommandBuffer(Vulkan.CommandBuffer, &CommandBufferBeginInfo));
    
	CLEAR_GPU_PROFILER_INFO(Vulkan.CommandBuffer, FrameInFlight);
    
	EngineState->Renderer.StagingBufferOffsets[FrameInFlight] = 0;
    
	// NOTE(georgii): Delete all voxels that were marked for deletion. Add all voxels that were added. Chnage all voxels that were updated. Update appropriate GPU buffers.
	UpdateVoxels(EngineState, Vulkan, Level);

	// Update particles
	uint32_t TotalParticleCount = UpdateParticles(EngineState, Vulkan, Level, GameInput.dt);
    
	// Sound mixing
	STempMemoryArena SoundTempMemory = BeginTempMemoryArena(&EngineState->MemoryArena);
	OutputPlayingSounds(&EngineState->AudioState, SoundBuffer, EngineState->LoadedSounds, &SoundTempMemory);
	EndTempMemoryArena(&SoundTempMemory);
    
	// Render
	STempMemoryArena RenderTempMemory = BeginTempMemoryArena(&EngineState->MemoryArena);
	VkImage FinalImage = RenderScene(EngineState, &EngineState->Renderer, Vulkan, Level, PointLightCount, TotalParticleCount, GameInput.FrameID, &RenderTempMemory, EngineState->GameTime, bSwapchainChanged);
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

	EngineState->GameTime += GameInput.dt;
    
	return FinalImage;
}