#pragma once

#define ArrayCount(Arr) (sizeof(Arr)/sizeof((Arr)[0]))
#define OffsetOf(Struct, Member) ((uint64_t)&(((Struct *)0)->Member))
#define GetAlignmentOf(Member) alignof(Member)

#ifndef ENGINE_RELEASE
#define Assert(Expr) if(!(Expr)) { *(int *)0 = 0; }
#else
#define Assert(Expr) 
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // NOTE(georgii): Included for malloc

constexpr uint64_t Kilobytes(uint64_t Value) { return Value*1024; }
constexpr uint64_t Megabytes(uint64_t Value) { return Kilobytes(Value*1024); }
constexpr uint64_t Gigabytes(uint64_t Value) { return Megabytes(Value*1024); }

const uint32_t MaxPath = 260;

void AlignAddress(void** Address, uint32_t Alignment)
{
	if (Alignment > 1)
	{
		uint32_t AlignmentMask = Alignment - 1;
		Assert((Alignment & AlignmentMask) == 0);

		uint64_t AddressUInt = uint64_t(*Address);
		if (AddressUInt & AlignmentMask)
		{
			AddressUInt += Alignment - (AddressUInt & AlignmentMask);
			*Address = (void*) AddressUInt;
		}
	}
}

void AlignAddress(uint8_t** Address, uint32_t Alignment)
{
	AlignAddress((void**)Address, Alignment);
}

struct SReadEntireFileResult
{
	void* Memory;
	size_t Size;
};

SReadEntireFileResult ReadEntireFile(const char* Path)
{
	SReadEntireFileResult Result = {};

	FILE* File = fopen(Path, "rb");
	Assert(File);
	if (File)
	{
		fseek(File, 0, SEEK_END);
		size_t FileSize = ftell(File);
		fseek(File, 0, SEEK_SET);

		uint8_t* Buffer = (uint8_t *)malloc(FileSize);
		Assert(Buffer);
		if (Buffer)
		{
			fread(Buffer, FileSize, 1, File);
			fclose(File);

			Result.Memory = Buffer;
			Result.Size = FileSize;
		}
	}

	return Result;
}

// NOTE(georgii): Adds null termination at the end
SReadEntireFileResult ReadEntireTextFile(const char* Path)
{
	SReadEntireFileResult Result = {};

	FILE* File = fopen(Path, "rb");
	Assert(File);
	if (File)
	{
		fseek(File, 0, SEEK_END);
		size_t FileSize = ftell(File);
		fseek(File, 0, SEEK_SET);

		uint8_t* Buffer = (uint8_t *)malloc(FileSize + 1);
		Assert(Buffer);
		if (Buffer)
		{
			fread(Buffer, FileSize, 1, File);
			fclose(File);

			Result.Memory = Buffer;
			Result.Size = FileSize;

			Buffer[FileSize] = 0;
		}
	}

	return Result;
}

void FreeEntireFile(SReadEntireFileResult* EntireFile)
{
	Assert(EntireFile->Memory);

	if (EntireFile->Memory)
	{
		free(EntireFile->Memory);
		EntireFile->Memory = 0;
	}
}

void WriteEntireFile(const char* Path, void* Data, size_t Size)
{
	FILE* File = fopen(Path, "wb");
	Assert(File);
	if (File)
	{
		fwrite(Data, Size, 1, File);
		fclose(File);
	}
}

enum EButtonType
{
	Button_W,
	Button_A,
	Button_S,
	Button_D,
	Button_Space,

	Button_MouseLeft,
	Button_MouseRight,

	Button_F1,
    Button_F2,

	Button_Control,
	Button_Alt,
	Button_Shift,
	Button_Delete,
	Button_Escape,
	Button_Enter,

	Button_Z,
	Button_R,
	Button_C,
	Button_X,
	Button_B,
	Button_Q,
	Button_E,
	Button_F,
	Button_G,
	Button_V,

	Button_ArrowUp,
	Button_ArrowDown,
	Button_ArrowLeft,
	Button_ArrowRight,

	Button_Count
};

struct SButton
{
	bool IsDown;
	uint32_t HalfTransitionCount;
};

bool WasDown(const SButton& Button)
{
	bool bResult = (Button.HalfTransitionCount > 1) || ((Button.HalfTransitionCount == 1) && Button.IsDown);
	return bResult;
}

bool WasReleased(const SButton& Button)
{
	bool bResult = (Button.HalfTransitionCount > 1) || ((Button.HalfTransitionCount == 1) && !Button.IsDown);
	return bResult;
}

struct SGameInput
{
	double MouseX, MouseY;
	double MouseLastX, MouseLastY;
	float MouseDeltaX, MouseDeltaY;
    float MouseWheelDelta;

	SButton Buttons[Button_Count];

	uint32_t FrameID;
	float dt;

	bool bShowMouse;
	double PlatformMouseX, PlatformMouseY;
};

struct SGameMemory
{
	size_t StorageSize;
	void* Storage;
};

struct SGameSoundBuffer
{
	uint32_t SamplesPerSec;
	uint32_t ChannelCount;
	uint32_t SampleCount;
	int16_t* Samples;
};

void PlatformOutputDebugString(const char* String);

#include "Math.h"
#include "DynamicArray.h"
#include "Logger.h"
#include "Vulkan.cpp"

struct SVulkanContext
{
	VkDevice Device;
	VmaAllocator MemoryAllocator;

	union
	{
		PFN_vkCmdDrawIndexedIndirectCount vkCmdDrawIndexedIndirectCount;
		PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR;
		PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD;
	};
	bool bMinMaxSampler;

	VkCommandPool CommandPool;
	VkCommandBuffer CommandBuffer;
	VkQueue GraphicsQueue;

	VkFormat SwapchainFormat;
	VkFormat DepthFormat;

	bool bSwapchainChanged;
	uint32_t InternalWidth, InternalHeight;
	uint32_t Width, Height;

	uint32_t FrameInFlight;

#ifndef ENGINE_RELEASE
	VkRenderPass ImguiRenderPass;
#endif
};

struct SWindowPlacementInfo
{
	void* InfoPointer;
	uint64_t InfoSizeInBytes;
};

//
// NOTE(georgii): Platform abstracted functions.
//

typedef void RunningThreadType(void* Data);
void PlatformCreateThread(RunningThreadType* RunningThreadFunction, void* Data);
void PlatformInterlockedExchange(uint32_t volatile* Target, uint32_t Value);
uint32_t PlatformInterlockedIncrement(uint32_t volatile* Target);

void PlatformDisableCursor(SGameInput* GameInput);
void PlatformEnableCursor(SGameInput* GameInput);
void PlatformToggleCursor(SGameInput* GameInput, bool bEnable, bool bShowCursor = true);
bool PlatformIsCursorEnabled();
bool PlatformIsCursorShowed();

void PlatformGetAllFilenamesFromDir(const char* WildcardPath, char* Filenames, uint32_t Stride, uint32_t& FileCount);

bool PlatformGetFullscreen();
void PlatformChangeFullscreen(bool bFullscreen);
SWindowPlacementInfo PlatformGetWindowPlacement();

bool PlatformGetVSync();
void PlatformChangeVSync(bool bEnabled);

void PlatformQuitGame();