#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

struct MyAppState {
	std::string name = "Hello, SDL3's GPU API!";
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	MyAppState* myAppState = new MyAppState();

	if (!SDL_CreateWindowAndRenderer(myAppState->name.c_str(), 1280, 720, 0, &myAppState->window, &myAppState->renderer)) {
		SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	*appstate = myAppState;

	return SDL_APP_CONTINUE;
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
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	int w = 0, h = 0;
	constexpr float scale = 4.0f;

	// Center the message and scale it up
	SDL_GetRenderOutputSize(myAppState->renderer, &w, &h);
	SDL_SetRenderScale(myAppState->renderer, scale, scale);
	const float x = (static_cast<float>(w) / scale - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * myAppState->name.length()) / 2;
	const float y = (static_cast<float>(h) / scale - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	// Draw the message
	SDL_SetRenderDrawColor(myAppState->renderer, 0, 0, 0, 255);
	SDL_RenderClear(myAppState->renderer);
	SDL_SetRenderDrawColor(myAppState->renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(myAppState->renderer, x, y, myAppState->name.c_str());
	SDL_RenderPresent(myAppState->renderer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);
	delete myAppState;
}
