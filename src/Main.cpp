#include <SDL2/SDL.h>
#include "CAEngine/CAEngine.h"

int main(int argc, char* argv[]) {
	CAEngine::startup();

	CAEngine::run();

	CAEngine::shutdown();

	return 0;
}
