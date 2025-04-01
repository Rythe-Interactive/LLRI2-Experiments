#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

#ifdef NDEBUG
	constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

struct MyAppState {
	std::string name = "Hello, SDL3's GPU API!";
	SDL_Window* window = nullptr;
	SDL_GPUDevice* device = nullptr;
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	MyAppState* myAppState = new MyAppState();

	myAppState->window = SDL_CreateWindow(myAppState->name.c_str(), 1280, 720, 0);
	if (myAppState->window == nullptr) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	myAppState->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug_mode, "vulkan");
	if (myAppState->device == nullptr) {
		SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(myAppState->device, myAppState->window)) {
		SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
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

	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(myAppState->device);
	if (commandBuffer == nullptr) {
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, myAppState->window, &swapchainTexture, nullptr, nullptr)) {
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != nullptr) {
		SDL_GPUColorTargetInfo colorTargetInfo = {nullptr};
		colorTargetInfo.texture = swapchainTexture;
		colorTargetInfo.clear_color = SDL_FColor{0.3f, 0.4f, 0.5f, 1.0f};
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);
		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);
	delete myAppState;
}
