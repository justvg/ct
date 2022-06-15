#pragma once

#ifndef ENGINE_RELEASE

#include "Intrinsics.h"

inline LARGE_INTEGER WinGetWallClock();
inline float WinGetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End);

// TODO(georgii): Currently this doesn't support recursion.
struct SCpuProfilerBlock
{
    const char* Name;
    uint64_t FirstCycleInFrame;

    uint64_t CycleStartValue;
    uint64_t CycleEndValue;

	LARGE_INTEGER TimeStartValue;
    LARGE_INTEGER TimeEndValue;

    uint64_t TotalCycles;
	float TotalTime;

    uint32_t HitCount;
};

struct SCpuProfiler
{
public:
    void BeginProfilerBlock(const char* Name);
    void EndProfilerBlock(const char* Name);

    void OutputInfo();
	void ClearInfo();

private:
    // NOTE(georgii): Must be a power of two!
    SCpuProfilerBlock BlocksHashTable[512];

private:
    SCpuProfilerBlock* GetBlockFromName(const char* Name);
};

void SCpuProfiler::BeginProfilerBlock(const char* Name)
{
    SCpuProfilerBlock* Block = GetBlockFromName(Name);

    Block->CycleStartValue = GetProcessorTimeStamp();
	Block->TimeStartValue = WinGetWallClock();
    if (Block->HitCount == 0)
    {
        Block->FirstCycleInFrame = Block->CycleStartValue;
    }
}

void SCpuProfiler::EndProfilerBlock(const char* Name)
{
    SCpuProfilerBlock* Block = GetBlockFromName(Name);
	
    Block->CycleEndValue = GetProcessorTimeStamp();
    Block->TotalCycles += Block->CycleEndValue - Block->CycleStartValue;

	Block->TimeEndValue = WinGetWallClock();
	Block->TotalTime += 1000.0f * WinGetSecondsElapsed(Block->TimeStartValue, Block->TimeEndValue);

    Block->HitCount++;
}

void SCpuProfiler::OutputInfo()
{
    SCpuProfilerBlock SortedBlocks[ArrayCount(BlocksHashTable)];
    memcpy(SortedBlocks, BlocksHashTable, sizeof(SortedBlocks));

	// Pack all entries at the start of the array
    uint32_t BlocksCount = 0;
    uint32_t LastFreeIndex = UINT32_MAX;
    for (uint32_t I = 0; I < ArrayCount(SortedBlocks); I++)
    {
        if (SortedBlocks[I].HitCount == 0)
        {
			if (I < LastFreeIndex)
            {
            	LastFreeIndex = I;
            }
        }
        else
        {
            if (LastFreeIndex < I)
            {
                SortedBlocks[LastFreeIndex] = SortedBlocks[I];
                LastFreeIndex++;
            }
			BlocksCount++;
        }
    }

	// Bubble sort 
	for (uint32_t I = 0; I < BlocksCount - 1; I++)
	{
		bool bSwapped  = false;
		for (uint32_t J = 0; J < BlocksCount - I - 1; J++)
		{
			if (SortedBlocks[J].FirstCycleInFrame > SortedBlocks[J + 1].FirstCycleInFrame)
			{
				SCpuProfilerBlock Temp = SortedBlocks[J];
				SortedBlocks[J] = SortedBlocks[J + 1];
				SortedBlocks[J + 1] = Temp;

				bSwapped = true;
			}
		}

		if (!bSwapped)
        {
			break;
        }
	}

	for (uint32_t I = 0; I < BlocksCount; I++)
	{
		SCpuProfilerBlock &Block = SortedBlocks[I];

		char Output[256];
		snprintf(Output, sizeof(Output), "%s: %.3fms/hit %lldcycl/hit %u\n", Block.Name, Block.TotalTime / Block.HitCount, Block.TotalCycles / Block.HitCount, Block.HitCount);
		PlatformOutputDebugString(Output);
	}

	PlatformOutputDebugString("\n\n");
}

void SCpuProfiler::ClearInfo()
{
    for (uint32_t I = 0; I < ArrayCount(BlocksHashTable); I++)
    {
        if (BlocksHashTable[I].HitCount != 0)
        {
			BlocksHashTable[I].FirstCycleInFrame = 0;
			BlocksHashTable[I].CycleStartValue = 0;
			BlocksHashTable[I].CycleEndValue = 0;
            BlocksHashTable[I].TimeStartValue = {};
            BlocksHashTable[I].TimeEndValue = {};
			BlocksHashTable[I].TotalCycles = 0;
			BlocksHashTable[I].TotalTime = 0.0f;
			BlocksHashTable[I].HitCount = 0;
        }
    }
}

SCpuProfilerBlock* SCpuProfiler::GetBlockFromName(const char* Name)
{
    uint32_t HashValue = 0;
    for (const char* C = Name; *C; C++)
    {
        HashValue += uint32_t(*C);
    }

    uint32_t Index = HashValue & (ArrayCount(BlocksHashTable) - 1);

    SCpuProfilerBlock *Block = 0;
    for (uint32_t I = 0; I < ArrayCount(BlocksHashTable); I++)
    {
        uint32_t BlockIndex = (Index + I) & (ArrayCount(BlocksHashTable) - 1);
        if ((BlocksHashTable[BlockIndex].Name == 0) || CompareStrings(BlocksHashTable[BlockIndex].Name, Name))
        {
            Block = &BlocksHashTable[BlockIndex];
			Block->Name = Name;
            break;
        }
    }
    Assert(Block);

    return Block;
}

static SCpuProfiler CpuProfiler = {};

#define BEGIN_PROFILER_BLOCK(BlockName) CpuProfiler.BeginProfilerBlock(BlockName)
#define END_PROFILER_BLOCK(BlockName) CpuProfiler.EndProfilerBlock(BlockName)
#define OUTPUT_PROFILER_INFO() CpuProfiler.OutputInfo()
#define CLEAR_PROFILER_INFO() CpuProfiler.ClearInfo()

#else

#define BEGIN_PROFILER_BLOCK(BlockName)
#define END_PROFILER_BLOCK(BlockName)
#define OUTPUT_PROFILER_INFO()
#define CLEAR_PROFILER_INFO()

#endif