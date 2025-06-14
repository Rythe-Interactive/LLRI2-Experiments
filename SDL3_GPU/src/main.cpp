// C++
#include <filesystem>
#include <string>

// SDL3
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// RSL
#define RSL_DEFAULT_ALIGNED_MATH false
#include <rsl/math>

// Assimp
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post-processing flags

// Macros to retrieve CMake variables
#define Q(x) #x
#define QUOTE(x) Q(x)

std::filesystem::path GetAssetsDir() {
	const std::filesystem::path assetsDirName = std::string(QUOTE(MYPROJECT_NAME)) + "-assets";
	return SDL_GetBasePath() / assetsDirName;
}

#ifdef NDEBUG
constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

struct MyVertex {
	math::float3 pos;
	math::float2 tex;
};

struct MyMesh {
	std::vector<MyVertex> vertices;
	std::vector<Uint16> indices;
	std::filesystem::path texture;

	[[nodiscard]] size_t vertices_size() const {
		return sizeof(MyVertex) * vertices.size();
	}

	[[nodiscard]] size_t indices_size() const {
		return sizeof(Uint16) * indices.size();
	}

	[[nodiscard]] size_t total_size() const {
		return vertices_size() + indices_size();
	}

	[[nodiscard]] SDL_GPUBufferCreateInfo vertex_buffer_create_info() const {
		return SDL_GPUBufferCreateInfo{
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = static_cast<Uint32>(vertices_size()),
		};
	}

	[[nodiscard]] SDL_GPUBufferCreateInfo index_buffer_create_info() const {
		return SDL_GPUBufferCreateInfo{
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = static_cast<Uint32>(indices_size()),
		};
	}

	[[nodiscard]] SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info() const {
		return SDL_GPUTransferBufferCreateInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = static_cast<Uint32>(total_size()),
		};
	}
};

struct MyAppState {
	std::string name = QUOTE(MYPROJECT_NAME);
	SDL_Window* window = nullptr;
	SDL_GPUDevice* device = nullptr;
	SDL_GPUGraphicsPipeline* pipeline = nullptr;
	SDL_GPUTexture* depthTexture = nullptr;
	math::uint2 depthTextureSize = {0, 0};
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
	SDL_GPUTexture* texture = nullptr;
	SDL_GPUSampler* sampler = nullptr;
	MyMesh* mesh = nullptr;
	Uint32 frameNumber = 0;
	std::array<Uint32, FRAME_NUMBERS> frameTimes = {};
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
	const std::filesystem::path fullPath = GetAssetsDir() / "shaders/compiled/" / (shaderFilename + ".spv");

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath.string().c_str(), &codeSize);
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

/// @param imagePath Path to image file, relative from the directory where the application was run from.
/// @param desiredChannels Colour channels of the image to load.
SDL_Surface* LoadImage(const std::filesystem::path& imagePath, const int desiredChannels) {
	const std::filesystem::path fullPath = GetAssetsDir() / imagePath;

	SDL_Surface* result = SDL_LoadBMP(fullPath.string().c_str());
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

std::optional<MyMesh> ImportMesh(const std::filesystem::path& meshPath) {
	const std::filesystem::path fullPath = GetAssetsDir() / meshPath;
	SDL_assert(is_regular_file(fullPath));
	Assimp::Importer importer;

	constexpr int flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_ValidateDataStructure | aiProcess_FindInvalidData;

	const aiScene* scene = importer.ReadFile(fullPath.string().c_str(), flags);

	if (nullptr == scene) {
		SDL_Log("[ERROR] Assimp: %s", importer.GetErrorString());
		return std::nullopt;
	}

	// Mesh
	SDL_assert(scene->HasMeshes());
	SDL_assert(scene->mNumMeshes == 1);
	const aiMesh* mesh = scene->mMeshes[0];
	SDL_assert(mesh->HasPositions() && mesh->HasFaces());

	// > Vertices
	SDL_Log("Assimp: Mesh %s has %d vertices", fullPath.c_str(), mesh->mNumVertices);
	std::vector<MyVertex> vertices(mesh->mNumVertices);
	for (int i = 0; i < mesh->mNumVertices; i++) {
		const aiVector3D& pos = mesh->mVertices[i];
		const aiVector3D& tex = mesh->mTextureCoords[0][i];
		SDL_Log("Assimp: Vertex %d: pos{x: %f, y: %f, z: %f} tex{x: %f, y: %f, z: %f}", i, pos.x, pos.y, pos.z, tex.x, tex.y, tex.z);
		vertices[i] = MyVertex{
			.pos = math::float3(pos.x, pos.y, pos.z),
			.tex = math::float2(tex.x, tex.y),
		};
	}
	// > Indices
	std::vector<Uint16> indices(mesh->mNumFaces * 3);
	SDL_Log("Assimp: Mesh %s has %d faces", fullPath.c_str(), mesh->mNumFaces);
	for (int i = 0; i < mesh->mNumFaces; i++) {
		const aiFace& face = mesh->mFaces[i];
		SDL_Log("Assimp: Face %d: ", i);
		for (int j = 0; j < face.mNumIndices; j++) {
			SDL_Log("%d ", face.mIndices[j]);
			indices[i * 3 + j] = face.mIndices[j];
		}
		SDL_Log("\n");
	}

	// Material texture path
	SDL_assert(scene->HasMaterials());
	SDL_assert(scene->mNumMaterials == 2); //default and my own
	const aiMaterial* material = scene->mMaterials[1]; // 1 is my material
	SDL_Log("Material %d: %s", 1, material->GetName().C_Str());
	SDL_assert(material->GetTextureCount(aiTextureType_DIFFUSE) == 1);

	aiString* assimpPath = new aiString();
	material->GetTexture(aiTextureType_DIFFUSE, 0, assimpPath);
	SDL_Log("Assimp path: %s", assimpPath->C_Str());
	std::filesystem::path texturePath = fullPath.parent_path() / assimpPath->C_Str();
	delete assimpPath;

	return MyMesh{
		.vertices = std::move(vertices),
		.indices = std::move(indices),
		.texture = std::move(texturePath),
	};
}

void CreateDepthTexture(const math::uint2& newSize, MyAppState* myAppState) {
	myAppState->depthTextureSize = newSize;
	const SDL_GPUTextureCreateInfo depthTextureCreateInfo = SDL_GPUTextureCreateInfo{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = newSize.x,
		.height = newSize.y,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	myAppState->depthTexture = SDL_CreateGPUTexture(myAppState->device, &depthTextureCreateInfo);
}

SDL_AppResult SDL_AppInit(void** appstate, [[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
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

	SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
	if (SDL_WindowSupportsGPUPresentMode(myAppState->device, myAppState->window, SDL_GPU_PRESENTMODE_IMMEDIATE)) {
		presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
	} else if (SDL_WindowSupportsGPUPresentMode(myAppState->device, myAppState->window, SDL_GPU_PRESENTMODE_MAILBOX)) {
		presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
	}
	if (!SDL_SetGPUSwapchainParameters(myAppState->device, myAppState->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode)) {
		SDL_Log("Couldn't set swapchain parameters: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	switch (presentMode) {
		case SDL_GPU_PRESENTMODE_VSYNC:
			SDL_Log("Using present mode: VSYNC");
			break;
		case SDL_GPU_PRESENTMODE_IMMEDIATE:
			SDL_Log("Using present mode: IMMEDIATE");
			break;
		case SDL_GPU_PRESENTMODE_MAILBOX:
			SDL_Log("Using present mode: MAILBOX");
			break;
	}

	// Load mesh
	// std::optional<MyMesh> oMesh = ImportMesh("models/container/blender_quad.obj");
	std::optional<MyMesh> oMesh = ImportMesh("models/suzanne/suzanne.obj");
	if (!oMesh.has_value()) {
		SDL_Log("Couldn't import mesh!");
		return SDL_APP_FAILURE;
	}
	myAppState->mesh = new MyMesh(std::move(oMesh.value()));

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
	SDL_Surface* imageData = LoadImage(myAppState->mesh->texture, 4);
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
	SDL_GPUColorTargetDescription colourTargetDescriptions[]{
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
		.depth_stencil_state = SDL_GPUDepthStencilState{
			.compare_op = SDL_GPU_COMPAREOP_LESS,
			.write_mask = 0xFF,
			.enable_depth_test = true,
			.enable_depth_write = true,
			.enable_stencil_test = false,
		},
		.target_info = SDL_GPUGraphicsPipelineTargetInfo{
			.color_target_descriptions = colourTargetDescriptions,
			.num_color_targets = 1,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
			.has_depth_stencil_target = true,
		},
	};

	myAppState->pipeline = SDL_CreateGPUGraphicsPipeline(myAppState->device, &pipelineCreateInfo);
	if (myAppState->pipeline == nullptr) {
		SDL_Log("Couldn't create graphics pipeline!");
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(myAppState->device, vertexShader);
	SDL_ReleaseGPUShader(myAppState->device, fragmentShader);

	// Depth Texture
	math::int2 screenSize;
	SDL_GetWindowSize(myAppState->window, &screenSize.x, &screenSize.y);
	CreateDepthTexture(screenSize, myAppState);

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
	SDL_GPUBufferCreateInfo vertexBufferCreateInfo = myAppState->mesh->vertex_buffer_create_info();
	myAppState->vertexBuffer = SDL_CreateGPUBuffer(myAppState->device, &vertexBufferCreateInfo);

	// > Index Buffer
	SDL_GPUBufferCreateInfo indexBufferCreateInfo = myAppState->mesh->index_buffer_create_info();
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
	SDL_GPUTransferBufferCreateInfo bufferTransferBufferCreateInfo = myAppState->mesh->transfer_buffer_create_info();
	SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(myAppState->device, &bufferTransferBufferCreateInfo); //(transfer buffer for the _buffers_ as opposed to the _texture_)

	// Request space from the GPU Driver to put our buffers data into
	rsl::byte* transferData = static_cast<rsl::byte*>(SDL_MapGPUTransferBuffer(myAppState->device, bufferTransferBuffer, false));

	// Copy the vertex data into the transfer buffer
	SDL_memcpy(transferData, myAppState->mesh->vertices.data(), myAppState->mesh->vertices_size());
	// Copy the index data into the transfer buffer
	SDL_memcpy(transferData + myAppState->mesh->vertices_size(), myAppState->mesh->indices.data(), myAppState->mesh->indices_size());

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
		.size = static_cast<Uint32>(myAppState->mesh->vertices_size())
	};
	SDL_UploadToGPUBuffer(copyPass, &vertexBufferLocation, &vertexBufferRegion, false);

	// > Upload the index buffer
	const SDL_GPUTransferBufferLocation indexBufferLocation = SDL_GPUTransferBufferLocation{
		.transfer_buffer = bufferTransferBuffer,
		.offset = static_cast<Uint32>(myAppState->mesh->vertices_size()),
	};
	const SDL_GPUBufferRegion indexBufferRegion = SDL_GPUBufferRegion{
		.buffer = myAppState->indexBuffer,
		.offset = 0,
		.size = static_cast<Uint32>(myAppState->mesh->indices_size())
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
SDL_AppResult SDL_AppEvent([[maybe_unused]] void* appstate, SDL_Event* event) {
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
	MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(myAppState->device);
	if (commandBuffer == nullptr) {
		SDL_Log("Couldn't AcquireGPUCommandBuffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	math::uint2 swapchainSize{};
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, myAppState->window, &swapchainTexture, &swapchainSize.x, &swapchainSize.y)) {
		SDL_Log("Couldn't WaitAndAcquireGPUSwapchainTexture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != nullptr) {
		const SDL_GPUColorTargetInfo colourTargetInfo = {
			.texture = swapchainTexture,
			.clear_color = SDL_FColor{0.3f, 0.4f, 0.5f, 1.0f},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
		};

		if (swapchainSize != myAppState->depthTextureSize) {
			SDL_Log("Resizing depth texture to %ux%u", swapchainSize.x, swapchainSize.y);
			SDL_ReleaseGPUTexture(myAppState->device, myAppState->depthTexture);
			CreateDepthTexture(swapchainSize, myAppState);
		}
		const SDL_GPUDepthStencilTargetInfo depthTargetInfo = {
			.texture = myAppState->depthTexture,
			.clear_depth = 1,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
			.stencil_store_op = SDL_GPU_STOREOP_STORE,
			.cycle = true,
			.clear_stencil = 0,
		};

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colourTargetInfo, 1, &depthTargetInfo);

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
		math::float4x4 model = math::float4x4(1.0f);
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &model, sizeof(model));

		// > View Matrix
		constexpr float radius = 10.0f;
		float camX = sinf(static_cast<float>(SDL_GetTicks()) / 1000.0f) * radius;
		float camZ = cosf(static_cast<float>(SDL_GetTicks()) / 1000.0f) * radius;
		math::float3 cameraPos = math::float3(camX, 3.0f, camZ);
		math::float3 cameraTarget = math::float3(0.0f, 0.0f, 0.0f);
		math::float3 up = math::float3(0.0f, 1.0f, 0.0f);
		math::float4x4 view = inverse(look_at(cameraPos, cameraTarget, up));
		SDL_PushGPUVertexUniformData(commandBuffer, 1, &view, sizeof(view));

		// > Projection Matrix
		math::int2 screenSize;
		SDL_GetWindowSize(myAppState->window, &screenSize.x, &screenSize.y);
		math::float4x4 proj = perspective(math::degrees(45.0f).radians(),
		                                  static_cast<float>(screenSize.x) / static_cast<float>(screenSize.y),
		                                  0.1f, 100.0f);
		SDL_PushGPUVertexUniformData(commandBuffer, 2, &proj, sizeof(proj));

		SDL_DrawGPUIndexedPrimitives(renderPass, myAppState->mesh->indices.size(), 1, 0, 0, 0);

		SDL_EndGPURenderPass(renderPass);
	}

	if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
		SDL_Log("Couldn't submit command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<Uint32, std::nano> frameTime = endTime - startTime;
	myAppState->frameTimes[myAppState->frameNumber] = frameTime.count();

	myAppState->frameNumber++;
	if (myAppState->frameNumber >= myAppState->frameTimes.size()) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, [[maybe_unused]] SDL_AppResult result) {
	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	SDL_ReleaseGPUGraphicsPipeline(myAppState->device, myAppState->pipeline);
	SDL_ReleaseGPUBuffer(myAppState->device, myAppState->vertexBuffer);
	SDL_ReleaseGPUBuffer(myAppState->device, myAppState->indexBuffer);
	SDL_ReleaseGPUTexture(myAppState->device, myAppState->texture);
	SDL_ReleaseGPUTexture(myAppState->device, myAppState->depthTexture);
	SDL_ReleaseGPUSampler(myAppState->device, myAppState->sampler);

	if (SDL_IOStream* fileStream = SDL_IOFromFile(QUOTE(MYPROJECT_NAME) "_frameTimes.txt", "w");
		fileStream == nullptr) {
		SDL_Log("Couldn't open file for writing: %s", SDL_GetError());
	} else {
		for (const Uint32 frameTime : myAppState->frameTimes) {
			std::string frameTimeStr = std::to_string(frameTime) + "\n";
			SDL_WriteIO(fileStream, frameTimeStr.c_str(), frameTimeStr.size());
		}
		if (!SDL_CloseIO(fileStream)) {
			SDL_Log("Couldn't close file: %s", SDL_GetError());
		}
	}

	Uint64 averageFrameTime = 0;
	for (const Uint32 frameTime : myAppState->frameTimes) {
		averageFrameTime += frameTime;
	}
	averageFrameTime /= myAppState->frameTimes.size();
	SDL_Log("Average frame time over %d frames was: %d ns", myAppState->frameNumber, averageFrameTime);

	delete myAppState;
}
