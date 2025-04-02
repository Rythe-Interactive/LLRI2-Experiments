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
	SDL_GPUGraphicsPipeline* pipeline = nullptr;
};

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const std::string& shaderFilename,
	const Uint32 samplerCount,
	const Uint32 uniformBufferCount,
	const Uint32 storageBufferCount,
	const Uint32 storageTextureCount
) {
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (shaderFilename.contains(".vert")) {
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	} else if (shaderFilename.contains(".frag")) {
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	} else {
		SDL_Log("Invalid shader stage!");
		return nullptr;
	}

	const SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	constexpr SDL_GPUShaderFormat desiredFormat = SDL_GPU_SHADERFORMAT_SPIRV;
	if (!(backendFormats & desiredFormat)) {
		SDL_Log("Device does not support SPIR-V shaders!");
		return nullptr;
	}
	const char* entrypoint = "main";
	const std::string fullPath = std::string(SDL_GetBasePath()) + "assets/shaders/compiled/" + shaderFilename + ".spv";

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath.c_str(), &codeSize);
	if (code == nullptr) {
		SDL_Log("Couldn't load shader from disk! %s", fullPath);
		return nullptr;
	}

	const SDL_GPUShaderCreateInfo shaderInfo = SDL_GPUShaderCreateInfo{
		.code_size = codeSize,
		.code = static_cast<Uint8*>(code),
		.entrypoint = entrypoint,
		.format = desiredFormat,
		.stage = stage,
		.num_samplers = samplerCount,
		.num_storage_textures = storageTextureCount,
		.num_storage_buffers = storageBufferCount,
		.num_uniform_buffers = uniformBufferCount,
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == nullptr) {
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return nullptr;
	}

	SDL_free(code);
	return shader;
}

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

	SDL_GPUShader* vertexShader = LoadShader(myAppState->device, "triangle.vert", 0, 0, 0, 0);
	if (vertexShader == nullptr) {
		SDL_Log("Couldn't create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragmentShader = LoadShader(myAppState->device, "triangle.frag", 0, 0, 0, 0);
	if (fragmentShader == nullptr) {
		SDL_Log("Couldn't create fragment shader!");
		return SDL_APP_FAILURE;
	}

	const SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = SDL_GPUGraphicsPipelineCreateInfo{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = SDL_GPURasterizerState{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
		},
		.target_info = SDL_GPUGraphicsPipelineTargetInfo{
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){
				SDL_GPUColorTargetDescription{
					.format = SDL_GetGPUSwapchainTextureFormat(myAppState->device, myAppState->window)
				}
			},
			.num_color_targets = 1,
		},
	};

	myAppState->pipeline = SDL_CreateGPUGraphicsPipeline(myAppState->device, &pipelineCreateInfo);
	if (myAppState->pipeline == nullptr) {
		SDL_Log("Couldn't create graphics pipeline!");
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(myAppState->device, vertexShader);
	SDL_ReleaseGPUShader(myAppState->device, fragmentShader);

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
		SDL_Log("Couldn't AcquireGPUCommandBuffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, myAppState->window, &swapchainTexture, nullptr, nullptr)) {
		SDL_Log("Couldn't WaitAndAcquireGPUSwapchainTexture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != nullptr) {
		const SDL_GPUColorTargetInfo colorTargetInfo = {
			.texture = swapchainTexture,
			.clear_color = SDL_FColor{0.3f, 0.4f, 0.5f, 1.0f},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);

		SDL_BindGPUGraphicsPipeline(renderPass, myAppState->pipeline);
		SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	SDL_ReleaseGPUGraphicsPipeline(myAppState->device, myAppState->pipeline);

	delete myAppState;
}
