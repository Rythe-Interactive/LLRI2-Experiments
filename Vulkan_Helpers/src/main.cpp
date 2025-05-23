// C++
#include <string>

// SDL3
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Engine
#include "vk_engine.hpp"

// Macros to retrieve CMake variables
#define Q(x) #x
#define QUOTE(x) Q(x)

#ifdef NDEBUG
constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

// ReSharper disable twice CppParameterNeverUsed
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	const std::string name = QUOTE(MYPROJECT_NAME);
	VulkanEngine* vulkanEngine = new VulkanEngine(name, debug_mode);
	*appstate = vulkanEngine;

	return vulkanEngine->Init(1280, 720);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	if (event->type == SDL_EVENT_KEY_DOWN ||
		event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS; // end the program, reporting success to the OS.
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	VulkanEngine* vulkanEngine = static_cast<VulkanEngine*>(appstate);

	vulkanEngine->Draw();

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, const SDL_AppResult result) {
	switch (result) {
		case SDL_APP_CONTINUE:
			SDL_Log("Exiting with result SDL_APP_CONTINUE");
			break;
		case SDL_APP_SUCCESS:
			SDL_Log("Exiting with result SDL_APP_SUCCESS");
			break;
		case SDL_APP_FAILURE:
			SDL_Log("Exiting with result SDL_APP_FAILURE");
			break;
		default:
			SDL_Log("Exiting with result unknown..?");
			break;
	}

	VulkanEngine* vulkanEngine = static_cast<VulkanEngine*>(appstate);
	vulkanEngine->Cleanup(result);

	delete vulkanEngine;
}
