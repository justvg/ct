#pragma once

struct SMemoryArena
{
	void* Memory;
	uint64_t Size;
	uint64_t Used;

	uint32_t TempCount;
};

void InitializeMemoryArena(SMemoryArena* MemoryArena, void* Memory, uint64_t Size)
{
	MemoryArena->Memory = Memory;
	MemoryArena->Size = Size;
	MemoryArena->Used = 0;
}

void* PushMemory(SMemoryArena* MemoryArena, uint64_t Size)
{
	Assert(MemoryArena->Size - MemoryArena->Used >= Size);

	void* Result = (uint8_t*) MemoryArena->Memory + MemoryArena->Used;
	MemoryArena->Used += Size;

	return Result;
}

struct STempMemoryArena
{
	SMemoryArena* Arena;
	uint64_t UsedSaved;
};

STempMemoryArena BeginTempMemoryArena(SMemoryArena* MemoryArena)
{
	STempMemoryArena TempMemoryArena;

	TempMemoryArena.Arena = MemoryArena;
	TempMemoryArena.UsedSaved = MemoryArena->Used;
	MemoryArena->TempCount++;

	return TempMemoryArena;
}

void EndTempMemoryArena(STempMemoryArena* TempMemoryArena)
{
	Assert(TempMemoryArena->UsedSaved <= TempMemoryArena->Arena->Used);
	TempMemoryArena->Arena->Used = TempMemoryArena->UsedSaved;

	Assert(TempMemoryArena->Arena->TempCount > 0);
	TempMemoryArena->Arena->TempCount--;
}