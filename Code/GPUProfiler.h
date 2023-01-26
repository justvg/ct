#pragma once

#ifndef ENGINE_RELEASE

static const uint32_t FramesHistory = FramesInFlight;

// NOTE(georgii): Currently this doesn't support timings with the same name.
struct SGpuProfilerBlock
{
    const char* Name;

    uint32_t QueryStartIndex[FramesHistory];
    uint32_t QueryEndIndex[FramesHistory];

    uint32_t HitCount[FramesHistory];

    uint32_t HierarchyDepth[FramesHistory];
};

struct SGpuProfilerBlockOuput
{
    const char* Name;
    float Time;
    uint32_t Depth;
};
struct SGpuProfilerOutput
{
    uint32_t Count;
    SGpuProfilerBlockOuput* Blocks;
};

struct SGpuProfiler
{
public:
    void Initialize(VkInstance Instance, VkDevice Device, float TimePeriod);

    void BeginProfilerBlock(const char* Name, VkCommandBuffer CommandBuffer, uint32_t FrameInFlight);
    void EndProfilerBlock(const char* Name, VkCommandBuffer CommandBuffer, uint32_t FrameInFlight);

    SGpuProfilerOutput OutputInfo(VkDevice Device, uint32_t FrameInFlight);
	void ClearInfo(VkCommandBuffer CommandBuffer, uint32_t FrameInFlight);

private:
    bool bInitialized;

    uint32_t CurrentOpenedBlocks;
    uint32_t QueriesMaxCount;
    uint32_t QueriesCounts;
    VkQueryPool QueryPools[FramesHistory];

    float TimestampPeriod;

    // NOTE(georgii): Must be a power of two!
    SGpuProfilerBlock BlocksHashTable[512];
    SGpuProfilerBlockOuput OutputBlocks[ArrayCount(BlocksHashTable)];

	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
	

private:
    SGpuProfilerBlock* GetBlockFromName(const char* Name);
};

void SGpuProfiler::Initialize(VkInstance Instance, VkDevice Device, float TimePeriod)
{
    if (!bInitialized)
    {
        QueriesMaxCount = 128;
        for (uint32_t I = 0; I < ArrayCount(QueryPools); I++)
        {
            QueryPools[I] = CreateQueryPool(Device, QueriesMaxCount);
        }

        TimestampPeriod = TimePeriod;

		vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT) vkGetInstanceProcAddr(Instance, "vkCmdBeginDebugUtilsLabelEXT");
		vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT) vkGetInstanceProcAddr(Instance, "vkCmdEndDebugUtilsLabelEXT");

        bInitialized = true;
    }
}

void SGpuProfiler::BeginProfilerBlock(const char* Name, VkCommandBuffer CommandBuffer, uint32_t FrameInFlight)
{
    SGpuProfilerBlock* Block = GetBlockFromName(Name);

    Block->HierarchyDepth[FrameInFlight] = CurrentOpenedBlocks;

    Assert(QueriesCounts < QueriesMaxCount);
    Block->QueryStartIndex[FrameInFlight] = QueriesCounts;
    vkCmdWriteTimestamp(CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, QueryPools[FrameInFlight], QueriesCounts++);

    Block->HitCount[FrameInFlight]++;
    CurrentOpenedBlocks++;

	// RenderDoc marker
	VkDebugUtilsLabelEXT MarkerBegin = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, 0, Name };
	if (vkCmdBeginDebugUtilsLabelEXT)
	{
		vkCmdBeginDebugUtilsLabelEXT(CommandBuffer, &MarkerBegin);
	}
}

void SGpuProfiler::EndProfilerBlock(const char* Name, VkCommandBuffer CommandBuffer, uint32_t FrameInFlight)
{
    Assert(CurrentOpenedBlocks > 0);

    SGpuProfilerBlock* Block = GetBlockFromName(Name);

    Assert(QueriesCounts < QueriesMaxCount);
    Block->QueryEndIndex[FrameInFlight] = QueriesCounts;
    vkCmdWriteTimestamp(CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, QueryPools[FrameInFlight], QueriesCounts++);

    CurrentOpenedBlocks--;

	// RenderDoc marker
	if (vkCmdEndDebugUtilsLabelEXT)
	{
		vkCmdEndDebugUtilsLabelEXT(CommandBuffer);
	}
}

SGpuProfilerOutput SGpuProfiler::OutputInfo(VkDevice Device, uint32_t FrameInFlight)
{
    SGpuProfilerBlock SortedBlocks[ArrayCount(BlocksHashTable)];
    memcpy(SortedBlocks, BlocksHashTable, sizeof(SortedBlocks));

    uint32_t LastFrameInFlight = (FrameInFlight + (FramesHistory - 1)) % FramesHistory;

	// Pack all entries at the start of the array
    uint32_t BlocksCount = 0;
    uint32_t LastFreeIndex = UINT32_MAX;
    for (uint32_t I = 0; I < ArrayCount(SortedBlocks); I++)
    {
        if (SortedBlocks[I].HitCount[LastFrameInFlight] == 0)
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

    SGpuProfilerOutput ProfilerOutput = { BlocksCount, OutputBlocks };
    if (BlocksCount > 0)
    {
	    // Bubble sort 
	    for (uint32_t I = 0; I < BlocksCount - 1; I++)
	    {
		    bool bSwapped  = false;
		    for (uint32_t J = 0; J < BlocksCount - I - 1; J++)
		    {
			    if (SortedBlocks[J].QueryStartIndex[LastFrameInFlight] > SortedBlocks[J + 1].QueryStartIndex[LastFrameInFlight])
			    {
				    SGpuProfilerBlock Temp = SortedBlocks[J];
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

        uint64_t TimestampResults[128]; 
		Assert(ArrayCount(TimestampResults) >= 2*BlocksCount);
        VkCheck(vkGetQueryPoolResults(Device, QueryPools[LastFrameInFlight], 0, 2*BlocksCount, sizeof(TimestampResults), TimestampResults, sizeof(TimestampResults[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

	    for (uint32_t I = 0; I < BlocksCount; I++)
	    {
		    SGpuProfilerBlock &Block = SortedBlocks[I];
            Assert(Block.QueryStartIndex[LastFrameInFlight] < Block.QueryEndIndex[LastFrameInFlight]);

            double BeginTime = double(TimestampResults[Block.QueryStartIndex[LastFrameInFlight]]) * (TimestampPeriod * 1e-6);
            double EndTime = double(TimestampResults[Block.QueryEndIndex[LastFrameInFlight]]) * (TimestampPeriod * 1e-6);
            double Time = EndTime - BeginTime;

            OutputBlocks[I].Name = Block.Name;
            OutputBlocks[I].Time = float(Time);
            OutputBlocks[I].Depth = Block.HierarchyDepth[LastFrameInFlight];
	    }
    }

    return ProfilerOutput;
}

void SGpuProfiler::ClearInfo(VkCommandBuffer CommandBuffer, uint32_t FrameInFlight)
{
    vkCmdResetQueryPool(CommandBuffer, QueryPools[FrameInFlight], 0, QueriesMaxCount);
    QueriesCounts = 0;
    CurrentOpenedBlocks = 0;
    
    for (uint32_t I = 0; I < ArrayCount(BlocksHashTable); I++)
    {
        if (BlocksHashTable[I].HitCount[FrameInFlight] != 0)
        {
            BlocksHashTable[I].QueryStartIndex[FrameInFlight] = 0;
            BlocksHashTable[I].QueryEndIndex[FrameInFlight] = 0;
            BlocksHashTable[I].HitCount[FrameInFlight] = 0;
            BlocksHashTable[I].HierarchyDepth[FrameInFlight] = 0;
        }
    }
}

SGpuProfilerBlock* SGpuProfiler::GetBlockFromName(const char* Name)
{
    uint32_t HashValue = 0;
    for (const char* C = Name; *C; C++)
    {
        HashValue += uint32_t(*C);
    }

    uint32_t Index = HashValue & (ArrayCount(BlocksHashTable) - 1);

    SGpuProfilerBlock *Block = 0;
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

static SGpuProfiler GpuProfiler = {};

#define INIT_GPU_PROFILER(Instance, Device, TimePeriod) GpuProfiler.Initialize(Instance, Device, TimePeriod)
#define BEGIN_GPU_PROFILER_BLOCK(BlockName, CommandBuffer, FrameInFlight) GpuProfiler.BeginProfilerBlock(BlockName, CommandBuffer, FrameInFlight)
#define END_GPU_PROFILER_BLOCK(BlockName, CommandBuffer, FrameInFlight) GpuProfiler.EndProfilerBlock(BlockName, CommandBuffer, FrameInFlight)
#define OUTPUT_GPU_PROFILER_INFO(Device, FrameInFlight) GpuProfiler.OutputInfo(Device, FrameInFlight)
#define CLEAR_GPU_PROFILER_INFO(CommandBuffer, FrameInFlight) GpuProfiler.ClearInfo(CommandBuffer, FrameInFlight)

#else

#define INIT_GPU_PROFILER(Instance, Device, TimePeriod)
#define BEGIN_GPU_PROFILER_BLOCK(BlockName, CommandBuffer, FrameInFlight)
#define END_GPU_PROFILER_BLOCK(BlockName, CommandBuffer, FrameInFlight)
#define OUTPUT_GPU_PROFILER_INFO(Device, FrameInFlight)
#define CLEAR_GPU_PROFILER_INFO(CommandBuffer, FrameInFlight)

#endif