#include <filesystem>
#include <string>
#include <vector>

#define Assert(Expr) if(!(Expr)) { *(int *)0 = 0; }
#define ArrayCount(Arr) (sizeof(Arr)/sizeof((Arr)[0]))

typedef uint32_t uint;
#include "shaders\\VoxelDimension.h"
#include "Level.h"

int main() {
    std::vector<std::string> filePaths;

    const std::string directory = "Levels\\";
    for (const auto &filePath : std::filesystem::directory_iterator(directory)) {
        if (filePath.path().extension() == ".ctl") {
            filePaths.push_back(filePath.path().string());
        }
    }

	SLevel* level = (SLevel*) malloc(sizeof(SLevel));
	if (level) {
		for (const std::string &filePath : filePaths) {
			memset(level, 0, sizeof(SLevel));

			if (LoadLevel(*level, filePath.c_str())) {
				SaveLevel(*level, filePath.c_str());
				printf("Resaved %s file.\n", filePath.c_str());
			} else {
				printf("Couldn't load and resave %s file.\n", filePath.c_str());
			}
		}
    }
	free(level);

    return 0;
}