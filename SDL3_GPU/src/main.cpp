#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

// Rythe Math
#define RSL_DEFAULT_ALIGNED_MATH false
#include <vector/vector.hpp>
#include <matrix/matrix.hpp>

#ifdef NDEBUG
constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

struct MyVertex {
	rsl::math::float3 pos;
	rsl::math::float2 tex;
};

struct MyAppState {
	std::string name = "Hello, SDL3's GPU API!";
	SDL_Window* window = nullptr;
	SDL_GPUDevice* device = nullptr;
	SDL_GPUGraphicsPipeline* pipeline = nullptr;
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
	SDL_GPUTexture* texture = nullptr;
	SDL_GPUSampler* sampler = nullptr;
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
	const std::string fullPath = std::string(SDL_GetBasePath()) + "assets/shaders/compiled/" + shaderFilename + ".spv";

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath.c_str(), &codeSize);
	if (code == nullptr) {
		SDL_Log("Couldn't load shader from disk! %s", fullPath.c_str());
		return nullptr;
	}

	const char* entrypoint = "main";
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
		SDL_Log("Couldn't create shader!");
		SDL_free(code);
		return nullptr;
	}

	SDL_free(code);
	return shader;
}

SDL_Surface* LoadImage(const std::string& imageFilename, const int desiredChannels) {
	const std::string fullPath = std::string(SDL_GetBasePath()) + "assets/images/" + imageFilename;

	SDL_Surface* result = SDL_LoadBMP(fullPath.c_str());
	if (result == nullptr) {
		SDL_Log("Couldn't load BMP: %s", SDL_GetError());
		return nullptr;
	}

	SDL_PixelFormat format;
	if (desiredChannels == 4) {
		format = SDL_PIXELFORMAT_ABGR8888;
	} else {
		SDL_assert(!"Unexpected desiredChannels");
		SDL_DestroySurface(result);
		return nullptr;
	}
	if (result->format != format) {
		SDL_Surface* next = SDL_ConvertSurface(result, format);
		SDL_DestroySurface(result);
		result = next;
	}

	return result;
}

// ReSharper disable twice CppParameterNeverUsed
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	MyAppState* myAppState = new MyAppState();
	*appstate = myAppState;

	// Window
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
	myAppState->window = SDL_CreateWindow(myAppState->name.c_str(), 1280, 720, flags);
	if (myAppState->window == nullptr) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// GPU Device
	myAppState->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug_mode, "vulkan");
	if (myAppState->device == nullptr) {
		SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// Bind GPU Device to Window
	if (!SDL_ClaimWindowForGPUDevice(myAppState->device, myAppState->window)) {
		SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// Shaders
	// > Vertex Shader
	SDL_GPUShader* vertexShader = LoadShader(myAppState->device, "triangle.vert", 0, 3, 0, 0);
	if (vertexShader == nullptr) {
		SDL_Log("Couldn't create vertex shader!");
		return SDL_APP_FAILURE;
	}

	// > Fragment Shader
	SDL_GPUShader* fragmentShader = LoadShader(myAppState->device, "triangle.frag", 1, 0, 0, 0);
	if (fragmentShader == nullptr) {
		SDL_Log("Couldn't create fragment shader!");
		return SDL_APP_FAILURE;
	}

	// Texture Image
	SDL_Surface* imageData = LoadImage("container.bmp", 4);
	if (imageData == nullptr) {
		SDL_Log("Couldn't load image data!");
		return SDL_APP_FAILURE;
	}

	// Pipeline
	SDL_GPUVertexBufferDescription vertexBufferDescriptions[]{
		SDL_GPUVertexBufferDescription{
			.slot = 0,
			.pitch = sizeof(MyVertex),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			.instance_step_rate = 0,
		},
	};
	SDL_GPUVertexAttribute vertexAttributes[]{
		// position
		SDL_GPUVertexAttribute{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = 0,
		},
		// texture coordinate
		SDL_GPUVertexAttribute{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = sizeof(float) * 3,
		},
	};
	SDL_GPUColorTargetDescription colorTargetDescriptions[]{
		SDL_GPUColorTargetDescription{
			.format = SDL_GetGPUSwapchainTextureFormat(myAppState->device, myAppState->window)
		}
	};
	const SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = SDL_GPUGraphicsPipelineCreateInfo{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.vertex_input_state = SDL_GPUVertexInputState{
			.vertex_buffer_descriptions = vertexBufferDescriptions,
			.num_vertex_buffers = 1,
			.vertex_attributes = vertexAttributes,
			.num_vertex_attributes = 2,
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = SDL_GPURasterizerState{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
		},
		.target_info = SDL_GPUGraphicsPipelineTargetInfo{
			.color_target_descriptions = colorTargetDescriptions,
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

	// (Texture) Sampler
	SDL_GPUSamplerCreateInfo samplerCreateInfo = SDL_GPUSamplerCreateInfo{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	};
	myAppState->sampler = SDL_CreateGPUSampler(myAppState->device, &samplerCreateInfo);

	// GPU Resources
	// > Vertex Buffer
	constexpr SDL_GPUBufferCreateInfo vertexBufferCreateInfo = SDL_GPUBufferCreateInfo{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(MyVertex) * 4,
	};
	myAppState->vertexBuffer = SDL_CreateGPUBuffer(myAppState->device, &vertexBufferCreateInfo);

	// > Index Buffer
	constexpr SDL_GPUBufferCreateInfo indexBufferCreateInfo = SDL_GPUBufferCreateInfo{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(Uint16) * 6,
	};
	myAppState->indexBuffer = SDL_CreateGPUBuffer(myAppState->device, &indexBufferCreateInfo);

	// > Texture
	unsigned int texWidth = imageData->w;
	unsigned int texHeight = imageData->h;
	SDL_GPUTextureCreateInfo textureCreateInfo = SDL_GPUTextureCreateInfo{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = texWidth,
		.height = texHeight,
		.layer_count_or_depth = 1,
		.num_levels = 1,
	};
	myAppState->texture = SDL_CreateGPUTexture(myAppState->device, &textureCreateInfo);

	// Transfer Buffer for the vertex and index buffers
	constexpr SDL_GPUTransferBufferCreateInfo bufferTransferBufferCreateInfo = SDL_GPUTransferBufferCreateInfo{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(MyVertex) * 4 + sizeof(Uint16) * 6,
	};
	SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(myAppState->device, &bufferTransferBufferCreateInfo); //(transfer buffer for the _buffers_ as opposed to the _texture_)

	// Request space from the GPU Driver to put our buffers data into
	MyVertex* transferData = static_cast<MyVertex*>(SDL_MapGPUTransferBuffer(myAppState->device, bufferTransferBuffer, false));

	// Copy the vertex data into the transfer buffer
	constexpr float radius = 0.5f;
	//positions, texture coords, colours from LearnOpenGL
	//@formatter:off
	transferData[0] = MyVertex{{ radius,  radius, 0.0f}, {1.0f, 1.0f},}; // top right
	transferData[1] = MyVertex{{ radius, -radius, 0.0f}, {1.0f, 0.0f},}; // bottom right
	transferData[2] = MyVertex{{-radius, -radius, 0.0f}, {0.0f, 0.0f},}; // bottom left
	transferData[3] = MyVertex{{-radius,  radius, 0.0f}, {0.0f, 1.0f},}; // top left
	//@formatter:on

	// Copy the index data into the transfer buffer
	Uint16* indexData = reinterpret_cast<Uint16*>(&transferData[4]);
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 3;
	indexData[3] = 1;
	indexData[4] = 2;
	indexData[5] = 3;

	// Release the space we requested from the GPU Driver again
	SDL_UnmapGPUTransferBuffer(myAppState->device, bufferTransferBuffer);

	// Transfer Buffer for the Texture
	SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo = SDL_GPUTransferBufferCreateInfo{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = texWidth * texHeight * 4,
	};
	SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(myAppState->device, &textureTransferBufferCreateInfo); //(transfer buffer for the _texture_ as opposed to the _buffers_)

	// Request space from the GPU Driver to put our texture data into
	Uint8* textureTransferPtr = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(myAppState->device, textureTransferBuffer, false));
	// Copy the image data into the transfer buffer
	SDL_memcpy(textureTransferPtr, imageData->pixels, texWidth * texHeight * 4);
	// Release the space we requested from the GPU Driver again
	SDL_UnmapGPUTransferBuffer(myAppState->device, textureTransferBuffer);

	// Command Buffer to copy the data to the GPU
	SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(myAppState->device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

	// > Upload the vertex buffer
	const SDL_GPUTransferBufferLocation vertexBufferLocation = SDL_GPUTransferBufferLocation{
		.transfer_buffer = bufferTransferBuffer,
		.offset = 0,
	};
	const SDL_GPUBufferRegion vertexBufferRegion = SDL_GPUBufferRegion{
		.buffer = myAppState->vertexBuffer,
		.offset = 0,
		.size = sizeof(MyVertex) * 4,
	};
	SDL_UploadToGPUBuffer(copyPass, &vertexBufferLocation, &vertexBufferRegion, false);

	// > Upload the index buffer
	const SDL_GPUTransferBufferLocation indexBufferLocation = SDL_GPUTransferBufferLocation{
		.transfer_buffer = bufferTransferBuffer,
		.offset = sizeof(MyVertex) * 4,
	};
	const SDL_GPUBufferRegion indexBufferRegion = SDL_GPUBufferRegion{
		.buffer = myAppState->indexBuffer,
		.offset = 0,
		.size = sizeof(Uint16) * 6,
	};
	SDL_UploadToGPUBuffer(copyPass, &indexBufferLocation, &indexBufferRegion, false);

	// > Upload the texture
	const SDL_GPUTextureTransferInfo textureTransferInfo = SDL_GPUTextureTransferInfo{
		.transfer_buffer = textureTransferBuffer,
		.offset = 0,
	};
	const SDL_GPUTextureRegion textureRegion = SDL_GPUTextureRegion{
		.texture = myAppState->texture,
		.w = texWidth,
		.h = texHeight,
		.d = 1,
	};
	SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);

	SDL_EndGPUCopyPass(copyPass);
	if (!SDL_SubmitGPUCommandBuffer(uploadCommandBuffer)) {
		SDL_Log("Couldn't submit upload command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_DestroySurface(imageData);
	SDL_ReleaseGPUTransferBuffer(myAppState->device, bufferTransferBuffer);
	SDL_ReleaseGPUTransferBuffer(myAppState->device, textureTransferBuffer);


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

		const SDL_GPUTextureSamplerBinding textureSamplerBinding = {
			.texture = myAppState->texture,
			.sampler = myAppState->sampler,
		};
		SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

		//Uniforms
		// > Model Matrix
		rsl::math::float4x4 model = rsl::math::float4x4(1.0f);
		model = rotate(model, rsl::math::deg2rad(-55.0f), rsl::math::float3(-1.0f, 0.0f, 0.0f));
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &model, sizeof(model));

		// > View Matrix
		rsl::math::float4x4 view = rsl::math::float4x4(1.0f);
		view = translate(view, rsl::math::float3(0.0f, 0.0f, 3.0f));
		SDL_PushGPUVertexUniformData(commandBuffer, 1, &view, sizeof(view));

		// > Projection Matrix
		rsl::math::int2 screenSize;
		SDL_GetWindowSize(myAppState->window, &screenSize.x, &screenSize.y);
		rsl::math::float4x4 proj = rsl::math::perspective(rsl::math::deg2rad(45.0f),
		                                                  static_cast<float>(screenSize.x) / static_cast<float>(screenSize.y),
		                                                  0.1f, 100.0f);
		SDL_PushGPUVertexUniformData(commandBuffer, 2, &proj, sizeof(proj));

		SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

		SDL_EndGPURenderPass(renderPass);
	}

	if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
		SDL_Log("Couldn't submit command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	SDL_ReleaseGPUGraphicsPipeline(myAppState->device, myAppState->pipeline);
	SDL_ReleaseGPUBuffer(myAppState->device, myAppState->vertexBuffer);
	SDL_ReleaseGPUBuffer(myAppState->device, myAppState->indexBuffer);
	SDL_ReleaseGPUTexture(myAppState->device, myAppState->texture);
	SDL_ReleaseGPUSampler(myAppState->device, myAppState->sampler);

	delete myAppState;
}
