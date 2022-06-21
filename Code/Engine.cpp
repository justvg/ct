#include "Engine.h"
#include "Audio.cpp"
#include "Assets.cpp"
#include "Particles.cpp"
#include "Entity.cpp"
#include "Game.cpp"
#include "Editor.cpp"
#include "Renderer.cpp"

void UpdateVoxels(SGameState* GameState, const SVulkanContext& Vulkan, SLevel* Level)
{
	SRenderer* Renderer = &GameState->Renderer;
	if (GameState->VoxelsToDeleteCount || GameState->VoxelsToChangeColorCount || GameState->VoxelsToAddCount)
	{
#ifndef ENGINE_RELEASE
		if (GameState->GameMode == GameMode_Editor)
		{
			if ((GameState->VoxelsToDeleteCount > 0) || (GameState->VoxelsToAddCount > 0))
			{
				SaveLevelHistory(&GameState->EditorState, Level);
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
        
		for (uint32_t I = 0; I < GameState->VoxelsToDeleteCount; I++)
		{
			uint32_t ID = GameState->VoxelsToDelete[I];
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
			uint32_t Z = ID / (LevelDimX*LevelDimY);
            
			// Remove game voxel
			SetVoxelColor(Level->Voxels, X, Y, Z, 0, 0, 0);
			SetVoxelActive(Level->Voxels, X, Y, Z, false);
            
			// Remove render voxel
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
            
			GameState->VoxelDraws[ID].FirstInstance = UINT32_MAX;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelDrawBuffer, ID*sizeof(SVoxelDraw), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &GameState->VoxelDraws[ID], sizeof(SVoxelDraw));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(SVoxelDraw);
            
			GameState->VoxelVisibilities[ID] = 1;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelVisibilityBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &GameState->VoxelVisibilities[ID], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		GameState->VoxelsToDeleteCount = 0;
        
		for (uint32_t I = 0; I < GameState->VoxelsToChangeColorCount; I++)
		{
			uint32_t ID = GameState->VoxelsToChangeColor[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimX*LevelDimY)) / LevelDimX;
			uint32_t Z = ID / (LevelDimY*LevelDimX);
            
			uint8_t Red = GameState->VoxelsToChangeColor[I].Red;
			uint8_t Green = GameState->VoxelsToChangeColor[I].Green;
			uint8_t Blue = GameState->VoxelsToChangeColor[I].Blue;
            
			// Change game voxel color
			SetVoxelColor(Level->Voxels, X, Y, Z, Red, Green, Blue);
            
			// Update render voxel 
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		GameState->VoxelsToChangeColorCount = 0;
        
		for (uint32_t I = 0; I < GameState->VoxelsToAddCount; I++)
		{
			uint32_t ID = GameState->VoxelsToAdd[I].ID;
			uint32_t X = ID % LevelDimX;
			uint32_t Y = (ID % (LevelDimY*LevelDimX)) / LevelDimX;
			uint32_t Z = ID / (LevelDimX*LevelDimY);
            
			uint8_t Red = GameState->VoxelsToAdd[I].Red;
			uint8_t Green = GameState->VoxelsToAdd[I].Green;
			uint8_t Blue = GameState->VoxelsToAdd[I].Blue;
            
			SetVoxelActive(Level->Voxels, X, Y, Z, true);
			SetVoxelColor(Level->Voxels, X, Y, Z, Red, Green, Blue);
            
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelsBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &Level->Voxels.ColorActive[Z][Y][X], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
            
			GameState->VoxelDraws[ID].FirstInstance = ID;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelDrawBuffer, ID*sizeof(SVoxelDraw), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &GameState->VoxelDraws[ID], sizeof(SVoxelDraw));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(SVoxelDraw);
            
			GameState->VoxelVisibilities[ID] = 1;
			UpdateBuffer(Vulkan.CommandBuffer, Renderer->VoxelVisibilityBuffer, ID*sizeof(uint32_t), Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], &GameState->VoxelVisibilities[ID], sizeof(uint32_t));
			Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += sizeof(uint32_t);
		}
		GameState->VoxelsToAddCount = 0;
        
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

uint32_t UpdateParticles(SGameState* GameState, const SVulkanContext& Vulkan, SLevel* Level, float dt)
{
	SRenderer* Renderer = &GameState->Renderer;

	uint32_t TotalParticleCount = SimulateParticles(Level->ParticleEmitters, Level->ParticleEmitterCount, dt);
	if (TotalParticleCount > 0)
	{
		GameState->ParticlesDrawCount = TotalParticleCount;
        
		uint32_t ParticleDrawIndex = 0;
		for (uint32_t I = 0; I < Level->ParticleEmitterCount; I++)
		{
			SParticleEmitter* Emitter = &Level->ParticleEmitters[I];
            
			for (uint32_t J = 0; J < Emitter->ParticleCount; J++)
			{
				SParticle* Particle = &Emitter->Particles[J];
				SParticleDraw* Draw = &GameState->ParticleDraws[ParticleDrawIndex++];
                
				Draw->Pos = Particle->Pos;
				Draw->Scale = Particle->Scale;
				Draw->Color = Particle->Color;
			}
		}
		Assert(ParticleDrawIndex == TotalParticleCount);
        
		VkBufferMemoryBarrier ParticleBufferBeginBarrier = CreateBufferMemoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, Renderer->ParticleDrawBuffer, Renderer->ParticleDrawBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &ParticleBufferBeginBarrier, 0, 0);
        
		UpdateBuffer(Vulkan.CommandBuffer, Renderer->ParticleDrawBuffer, 0, Renderer->StagingBuffers[Vulkan.FrameInFlight], Renderer->StagingBufferOffsets[Vulkan.FrameInFlight], GameState->ParticleDraws, GameState->ParticlesDrawCount*sizeof(SParticleDraw));
		Renderer->StagingBufferOffsets[Vulkan.FrameInFlight] += GameState->ParticlesDrawCount*sizeof(SParticleDraw);
        
		VkBufferMemoryBarrier ParticleBufferEndBarrier = CreateBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, Renderer->ParticleDrawBuffer, Renderer->ParticleDrawBuffer.Size);
		vkCmdPipelineBarrier(Vulkan.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, 0, 1, &ParticleBufferEndBarrier, 0, 0);
	}
    
	return TotalParticleCount;
}

VkImage GameUpdateAndRender(const SVulkanContext& Vulkan, SGameMemory* GameMemory, const SGameInput& GameInput, const SGameSoundBuffer& SoundBuffer)
{
	Assert(sizeof(SGameState) <= GameMemory->StorageSize);
    SGameState* GameState = (SGameState*) GameMemory->Storage;
    if (!GameState->bInitialized)
    {
		InitializeMemoryArena(&GameState->MemoryArena, (uint8_t*) GameMemory->Storage + sizeof(SGameState), GameMemory->StorageSize - sizeof(SGameState));

		CreateVoxelMesh(GameState->Geometry);
		AddGLTFMesh(GameState->Geometry, "Models\\cube.gltf");
		CreateQuadMesh(GameState->Geometry);
        
		InitializeRenderer(&GameState->Renderer, Vulkan, &GameState->Geometry);
            
		GameState->LoadedSounds[0] = LoadWAV("Sounds\\example.wav");
		GameState->LoadedSounds[1] = LoadWAV("Sounds\\shotgun.wav");
		GameState->AudioState.MasterVolume = Vec2(1.0f, 1.0f);
		// GameState->BackgroundSound = PlaySound(&GameState->AudioState, true, 0);
        
		GameState->Camera.OffsetFromPlayer = Vec3(0.0f, 0.8f*0.5f*1.5f, 0.0f);
        GameState->Camera.Near = 0.01f;
        GameState->Camera.Far = 300.0f;
        GameState->Camera.FoV = 70.0f;

		ReadEntireFileResult ConfigFile = ReadEntireTextFile("Game.cfg");
		if (ConfigFile.Memory && ConfigFile.Size)
		{
			char* FileText = (char*) ConfigFile.Memory;

			char* LineStart = FileText;
			uint32_t LineLength = 0;
			for (int I = 0; I < ConfigFile.Size + 1; I++)
			{
				if ((FileText[I] != '\r') && (FileText[I] != '\n') && (FileText[I] != 0))
				{
					LineLength++;
				}
				else
				{
					char* NameStart = (char*) LineStart;
					uint32_t NameLength = 0;
					for (uint32_t J = 0; J < LineLength; J++)
					{
						if (NameStart[J] != ' ')
						{
							NameLength++;
						}
						else
						{
							break;
						}
					}

					char *ValueStart = NameStart + NameLength + 1;
					uint32_t ValueLength = LineLength - NameLength - 1;
					if (CompareStrings(NameStart, NameLength, "HeroSpeed"))
					{
						GameState->HeroSpeed = StrToFloat(ValueStart, ValueLength);
					}
					else if (CompareStrings(NameStart, NameLength, "HeroDrag"))
					{
						GameState->HeroDrag = StrToFloat(ValueStart, ValueLength);
					}
					else if (CompareStrings(NameStart, NameLength, "HeroJumpPower"))
					{
						GameState->HeroJumpPower = StrToFloat(ValueStart, ValueLength);
					}
					else if (CompareStrings(NameStart, NameLength, "HeroLampDistance"))
					{
						GameState->HeroLampDistance = StrToFloat(ValueStart, ValueLength);
					}
                #ifndef ENGINE_RELEASE
                    else if (CompareStrings(NameStart, NameLength, "CameraSpeed"))
					{
		                GameState->EditorState.CameraSpeed = StrToFloat(ValueStart, ValueLength);
					}
                #endif

					if (FileText[I] == '\r')
					{
						I++;
					}
					LineStart = FileText + I + 1;
					LineLength = 0;
				}
			}

			free(ConfigFile.Memory);
		}
		else
		{
			// NOTE(georgii): Use these values if for some reason there is no config file.
			GameState->HeroSpeed = 11.0f;
			GameState->HeroDrag = 3.1f;
			GameState->HeroJumpPower = 4.5f;
			GameState->HeroLampDistance = 8.0f;

        #ifndef ENGINE_RELEASE
		    GameState->EditorState.CameraSpeed = 1.0f;
        #endif
		}

        GameState->DeathAnimationSpeed = 10.0f;
        
		LoadLevel(GameState, &GameState->LevelBaseState, "MainHub.ctl");
        
        GameState->bInitialized = true;
    }
    
	if (Vulkan.bSwapchainResized)
	{
		RendererSwapchainResized(&GameState->Renderer, Vulkan);
	}
	
	if (GameState->bReloadLevel || GameState->bReloadLevelEditor || GameState->bForceUpdateVoxels)
	{
		if (GameState->bReloadLevel)
		{
			GameState->Level = GameState->LevelBaseState;
		}
		if (GameState->bReloadLevelEditor)
		{
			GameState->Level = GameState->LevelGameStartState;
		}

		Assert(GameState->Level.Entities[0].Type == Entity_Hero);
		if (GameState->bDeathAnimation)
		{
			GameState->Level.Entities[0].Pos = GameState->LastCheckpointPos;
		}
		else
		{
			GameState->LastCheckpointPos = GameState->Level.Entities[0].Pos; 
		}

		memset(GameState->VoxelDraws, 0, sizeof(GameState->VoxelDraws));
		for (uint32_t Z = 0; Z < LevelDimZ; Z++)
		{
			for (uint32_t Y = 0; Y < LevelDimY; Y++)
			{
				for (uint32_t X = 0; X < LevelDimX; X++)
				{
					SVoxelDraw* VoxelDraw = &GameState->VoxelDraws[Z*LevelDimX*LevelDimY + Y*LevelDimX + X];
                    
					VoxelDraw->Pos = Vec3i(X, Y, Z) * VoxelDim;
					VoxelDraw->FirstInstance = IsVoxelActive(GameState->Level.Voxels, X, Y, Z) ? Z*LevelDimX*LevelDimY + Y*LevelDimX + X : UINT32_MAX;
                    
					GameState->VoxelVisibilities[Z*LevelDimX*LevelDimY + Y*LevelDimX + X] = 1;
				}
			}
		}
        
		VkCheck(vkDeviceWaitIdle(Vulkan.Device));
        
		SRenderer* Renderer = &GameState->Renderer;
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelDrawBuffer, Renderer->StagingBuffers[0], GameState->VoxelDraws, sizeof(GameState->VoxelDraws));
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelVisibilityBuffer, Renderer->StagingBuffers[0], GameState->VoxelVisibilities, sizeof(GameState->VoxelVisibilities));
		UploadBuffer(Vulkan.Device, Vulkan.CommandPool, Vulkan.CommandBuffer, Vulkan.GraphicsQueue, Renderer->VoxelsBuffer, Renderer->StagingBuffers[0], GameState->Level.Voxels.ColorActive, sizeof(GameState->Level.Voxels));
		
		GameState->bReloadLevel = GameState->bReloadLevelEditor = GameState->bForceUpdateVoxels = false;
	}
    
	SCamera& Camera = GameState->Camera;
	SLevel* Level = &GameState->Level;
    
	switch (GameState->GameMode)
	{
		case GameMode_Game:
		{
			UpdateGame(GameState, &GameInput);
		} break;
        
#ifndef ENGINE_RELEASE
		case GameMode_Editor:
		{
			UpdateEditor(GameState, (SGameInput*) &GameInput, &Vulkan, &GameState->Renderer);
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
	UpdateCameraRenderData(&GameState->Renderer, Camera, Vulkan.Width, Vulkan.Height, GameInput.FrameID, FrameInFlight);
    
	SLightBuffer* LightBufferGPU = (SLightBuffer*) GameState->Renderer.LightBuffers[Vulkan.FrameInFlight].Data;
	LightBufferGPU->AmbientColor = Vec4(Level->AmbientColor, 0.0f);
	LightBufferGPU->AmbientConstant = Vec4(Level->AmbientConstant, 0.0f);
    
	SPointLight* PointLightsGPU = (SPointLight*) GameState->Renderer.PointLightsBuffers[Vulkan.FrameInFlight].Data;
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
			if ((LengthSq(Entity->PointLight.Color.rgb) > 0.0f) && (Entity->PointLight.Radius > 0.0f))
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
    
	GameState->Renderer.StagingBufferOffsets[FrameInFlight] = 0;
    
	// NOTE(georgii): Delete all voxels that were marked for deletion. Add all voxels that were added. Chnage all voxels that were updated. Update appropriate GPU buffers.
	UpdateVoxels(GameState, Vulkan, Level);

	// Update particles
	uint32_t TotalParticleCount = UpdateParticles(GameState, Vulkan, Level, GameInput.dt);
    
	// Sound mixing
	STempMemoryArena SoundTempMemory = BeginTempMemoryArena(&GameState->MemoryArena);
	OutputPlayingSounds(&GameState->AudioState, SoundBuffer, GameState->LoadedSounds, &SoundTempMemory);
	EndTempMemoryArena(&SoundTempMemory);
    
	// Render
	STempMemoryArena RenderTempMemory = BeginTempMemoryArena(&GameState->MemoryArena);
	VkImage FinalImage = RenderScene(GameState, &GameState->Renderer, Vulkan, Level, PointLightCount, TotalParticleCount, GameInput.FrameID, &RenderTempMemory, GameInput.dt);
	EndTempMemoryArena(&RenderTempMemory);

	// Update text stuff
	for (uint32_t I = 0; I < GameState->TextsToRenderCount;)
	{
		SText* Text = GameState->TextsToRender + I;

		Text->CurrentTime += GameInput.dt;
		if (Text->CurrentTime > Text->Time)
		{
			*Text = GameState->TextsToRender[--GameState->TextsToRenderCount];
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
    
	return FinalImage;
}