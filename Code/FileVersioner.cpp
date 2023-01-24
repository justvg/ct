#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

#define Assert(Expr) if(!(Expr)) { *(int *)0 = 0; }
#define ArrayCount(Arr) (sizeof(Arr)/sizeof((Arr)[0]))
#define OffsetOf(Struct, Member) ((uint64_t)&(((Struct *)0)->Member))
#define GetAlignmentOf(Member) alignof(Member)

typedef uint32_t uint;
#include "shaders\\VoxelDimension.h"
#include "Level.h"

void AlignAddress(void** address, uint32_t alignment)
{
	if (alignment > 1)
	{
		uint32_t alignmentMask = alignment - 1;
		Assert((alignment & alignmentMask) == 0);

		uint64_t addressUInt = uint64_t(*address);
		if (addressUInt & alignmentMask)
		{
			addressUInt += alignment - (addressUInt & alignmentMask);
			*address = (void*) addressUInt;
		}
	}
}

void AlignAddress(uint8_t** address, uint32_t alignment)
{
	AlignAddress((void**)address, alignment);
}

struct readEntireFileResult_t {
	void* memory;
	size_t size;
};

readEntireFileResult_t ReadEntireFile(const char* path) {
	readEntireFileResult_t result = {};

	FILE* file = fopen(path, "rb");
	Assert(file);
	if (file) {
		fseek(file, 0, SEEK_END);
		size_t fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		uint8_t* buffer = (uint8_t *)malloc(fileSize);
		Assert(buffer);
		if (buffer) {
			fread(buffer, fileSize, 1, file);
			fclose(file);

			result.memory = buffer;
			result.size = fileSize;
		}
	}

	return result;
}

void FreeEntireFile(readEntireFileResult_t* file)
{
	Assert(file->memory);

	if (file->memory)
	{
		free(file->memory);
		file->memory = 0;
	}
}

uint8_t* ReadAndMovePtr(void* destination, const void* source, size_t size) {
    memcpy(destination, source, size);
    return (uint8_t*) source + size;
}

int main() {
    std::vector<std::string> filePaths;

    const std::string directory = "Levels\\";
    for (const auto &filePath : std::filesystem::directory_iterator(directory)) {
        if (filePath.path().extension() == ".ctl") {
            filePaths.push_back(filePath.path().string());
        }
    }

    for (const std::string &filePath : filePaths) {
        readEntireFileResult_t file = ReadEntireFile(filePath.c_str());
        SLevel* level = (SLevel*) malloc(sizeof(SLevel));
        if (file.memory && file.size > 0 && level) {
            memset(level, 0, sizeof(SLevel));

            uint8_t* memoryPtr = file.memory;

            // read file version
            uint32_t fileVersion;
            memoryPtr = ReadAndMovePtr(&fileVersion, memoryPtr, sizeof(fileVersion));

            // read voxels
            memoryPtr = ReadAndMovePtr(&level->Voxels, memoryPtr, sizeof(level->Voxels));

            // load entities
			// AlignAddress(&LevelMemory, GetAlignmentOf(uint32_t));
			// memcpy(&Level->EntityCount, LevelMemory, sizeof(uint32_t));
			// LevelMemory += sizeof(uint32_t);

            free(level);
            FreeEntireFile(&file);
        }
    }

    return 0;
}