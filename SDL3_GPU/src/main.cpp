#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

#ifdef NDEBUG
	constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

typedef struct PositionColorVertex {
	float x, y, z;
	Uint8 r, g, b, a;
} PositionColorVertex;

struct MyAppState {
	std::string name = "Hello, SDL3's GPU API!";
	SDL_Window* window = nullptr;
	SDL_GPUDevice* device = nullptr;
	SDL_GPUGraphicsPipeline* pipeline = nullptr;
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
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
		SDL_Log("Couldn't load shader from disk! %s", fullPath.c_str());
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

// ReSharper disable twice CppParameterNeverUsed
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
		.vertex_input_state = SDL_GPUVertexInputState{
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){
				SDL_GPUVertexBufferDescription{
					.slot = 0,
					.pitch = sizeof(PositionColorVertex),
					.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
					.instance_step_rate = 0,
				},
			},
			.num_vertex_buffers = 1,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){
				SDL_GPUVertexAttribute{
					.location = 0,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = 0,
				},
				SDL_GPUVertexAttribute{
					.location = 1,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
					.offset = sizeof(float) * 3,
				},
			},
			.num_vertex_attributes = 2,
		},
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

	//GPU Resources

	//>Vertex Buffer
	constexpr SDL_GPUBufferCreateInfo vertexBufferCreateInfo = SDL_GPUBufferCreateInfo{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(PositionColorVertex) * 4,
	};

	myAppState->vertexBuffer = SDL_CreateGPUBuffer(myAppState->device, &vertexBufferCreateInfo);

	//>Index Buffer
	constexpr SDL_GPUBufferCreateInfo indexBufferCreateInfo = SDL_GPUBufferCreateInfo{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(Uint16) * 6,
	};

	myAppState->indexBuffer = SDL_CreateGPUBuffer(myAppState->device, &indexBufferCreateInfo);

	//Transfer
	constexpr SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = SDL_GPUTransferBufferCreateInfo{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(PositionColorVertex) * 4 + sizeof(Uint16) * 6,
	};

	SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(myAppState->device, &transferBufferCreateInfo);

	PositionColorVertex* transferData = static_cast<PositionColorVertex*>(SDL_MapGPUTransferBuffer(myAppState->device, bufferTransferBuffer, false));

	constexpr float radius = 0.8f;
	transferData[0] = PositionColorVertex{-radius, radius, 0, 255, 0, 0, 255};
	transferData[1] = PositionColorVertex{radius, radius, 0, 0, 255, 0, 255};
	transferData[2] = PositionColorVertex{radius, -radius, 0, 0, 0, 255, 255};
	transferData[3] = PositionColorVertex{-radius, -radius, 0, 255, 255, 255, 255};

	Uint16* indexData = reinterpret_cast<Uint16*>(&transferData[4]);
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 0;
	indexData[4] = 2;
	indexData[5] = 3;

	SDL_UnmapGPUTransferBuffer(myAppState->device, bufferTransferBuffer);

	// Upload the transfer data to the vertex buffer
	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(myAppState->device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

	const SDL_GPUTransferBufferLocation vertexBufferLocation = SDL_GPUTransferBufferLocation{
		.transfer_buffer = bufferTransferBuffer,
		.offset = 0,
	};

	const SDL_GPUBufferRegion vertexBufferRegion = SDL_GPUBufferRegion{
		.buffer = myAppState->vertexBuffer,
		.offset = 0,
		.size = sizeof(PositionColorVertex) * 4,
	};

	SDL_UploadToGPUBuffer(copyPass, &vertexBufferLocation, &vertexBufferRegion, false);

	const SDL_GPUTransferBufferLocation indexBufferLocation = SDL_GPUTransferBufferLocation{
		.transfer_buffer = bufferTransferBuffer,
		.offset = sizeof(PositionColorVertex) * 4,
	};

	const SDL_GPUBufferRegion indexBufferRegion = SDL_GPUBufferRegion{
		.buffer = myAppState->indexBuffer,
		.offset = 0,
		.size = sizeof(Uint16) * 6,
	};

	SDL_UploadToGPUBuffer(copyPass, &indexBufferLocation, &indexBufferRegion, false);

	SDL_EndGPUCopyPass(copyPass);
	if (SDL_SubmitGPUCommandBuffer(uploadCmdBuf)) {
		SDL_Log("Couldn't submit command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_ReleaseGPUTransferBuffer(myAppState->device, bufferTransferBuffer);


	return SDL_APP_CONTINUE;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	switch (event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS; // end the program, reporting success to the OS.
		case SDL_EVENT_KEY_DOWN:
			if (event->key.key != SDLK_ESCAPE && event->key.key != SDLK_Q) break;
			return SDL_APP_SUCCESS; // end the program, reporting success to the OS.
		default:
			break;
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

		const SDL_GPUBufferBinding vertexBufferBinding = {
			.buffer = myAppState->vertexBuffer,
			.offset = 0,
		};
		SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

		const SDL_GPUBufferBinding indexBufferBinding = {
			.buffer = myAppState->indexBuffer,
			.offset = 0,
		};
		SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);


		SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

		SDL_EndGPURenderPass(renderPass);
	}

	if (SDL_SubmitGPUCommandBuffer(commandBuffer)) {
		SDL_Log("Couldn't submit command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	SDL_ReleaseGPUGraphicsPipeline(myAppState->device, myAppState->pipeline);
	SDL_ReleaseGPUBuffer(myAppState->device, myAppState->vertexBuffer);

	delete myAppState;
}
