// SDL3
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

// RSL Math
#define RSL_DEFAULT_ALIGNED_MATH false
#include <rsl/math>

// Vulkan
#include <volk.h>
#include <VkBootstrap.h>

// C++
#include <filesystem>
#include <fstream>
#include <string>

// Macros to retrieve CMake variables
#define Q(x) #x
#define QUOTE(x) Q(x)

std::filesystem::path GetBasePath() {
	const std::filesystem::path assetsDirName = std::string(QUOTE(MYPROJECT_NAME)) + "-assets";
	return SDL_GetBasePath() / assetsDirName;
}

#ifdef NDEBUG
constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

struct FrameData {
	VkCommandPool commandPool = nullptr;
	VkCommandBuffer mainCommandBuffer = nullptr;

	VkSemaphore swapchainSemaphore = nullptr;
	VkSemaphore renderSemaphore = nullptr;
	VkFence renderFence = nullptr;
};

constexpr unsigned int FRAME_OVERLAP = 2;

struct MyAppState {
	std::string name = QUOTE(MYPROJECT_NAME);
	SDL_Window* window = nullptr;

	VkInstance instance = nullptr;
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkSurfaceKHR surface = nullptr;

	VkSwapchainKHR swapchain = nullptr;
	VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent = {};

	int frameNumber = 0;
	FrameData frames[FRAME_OVERLAP];
	FrameData& GetCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; }
	VkQueue graphicsQueue = nullptr;
	uint32_t graphicsQueueFamilyIndex = 0;

	VkPipelineLayout trianglePipelineLayout;
	VkPipeline trianglePipeline;
};

std::optional<VkShaderModule> LoadShaderModule(const char* filePath, VkDevice device) {
	size_t fileSize;
	uint32_t* contents = static_cast<uint32_t*>(SDL_LoadFile(filePath, &fileSize));
	if (contents == nullptr) {
		SDL_Log("Couldn't load shader from disk! %s\n%s", filePath, SDL_GetError());
		return std::nullopt;
	}

	// create a new shader module, using the buffer we loaded
	const VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		// codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
		.codeSize = fileSize,
		.pCode = contents,
	};

	// check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return std::nullopt;
	}
	return shaderModule;
}

/// Returns true on success, false on failure
bool InitTrianglePipeline(const MyAppState* myAppState) {
	std::optional<VkShaderModule> triangleFragShader = LoadShaderModule("../../shaders/colored_triangle.frag.spv", myAppState->device);
	if (!triangleFragShader.has_value()) {
		SDL_Log("Error when building the triangle fragment shader module");
	} else {
		SDL_Log("Triangle fragment shader succesfully loaded");
	}

	std::optional<VkShaderModule> triangleVertexShader = LoadShaderModule("../../shaders/colored_triangle.vert.spv", myAppState->device);
	if (!triangleVertexShader.has_value()) {
		SDL_Log("Error when building the triangle vertex shader module");
	} else {
		SDL_Log("Triangle vertex shader succesfully loaded");
	}

	//build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	// VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineLayoutCreateInfo();
}

/// Returns true on success, false on failure
bool CreateSwapchain(MyAppState* myAppState, const uint32_t width, const uint32_t height) {
	myAppState->swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::SwapchainBuilder swapchainBuilder(myAppState->physicalDevice, myAppState->device, myAppState->surface);
	vkb::Result<vkb::Swapchain> swapchain_ret = swapchainBuilder
	                                            .set_desired_format(VkSurfaceFormatKHR{.format = myAppState->swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
	                                            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
	                                            .set_desired_extent(width, height)
	                                            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	                                            .build();

	if (!swapchain_ret.has_value()) {
		SDL_Log("Couldn't create swapchain: %s", swapchain_ret.error().message().c_str());
		return false;
	}

	//Yoink the swapchain from the result
	vkb::Swapchain vkbSwapchain = swapchain_ret.value();
	myAppState->swapchain = vkbSwapchain.swapchain;
	myAppState->swapchainExtent = vkbSwapchain.extent;
	myAppState->swapchainImages = vkbSwapchain.get_images().value();
	myAppState->swapchainImageViews = vkbSwapchain.get_image_views().value();

	return true;
}

bool InitSwapchain(MyAppState* myAppState) {
	int32_t width, height;
	SDL_GetWindowSize(myAppState->window, &width, &height);
	return CreateSwapchain(myAppState, width, height);
}

void DestroySwapchain(const MyAppState* myAppState) {
	vkDestroySwapchainKHR(myAppState->device, myAppState->swapchain, nullptr);

	//Destroy swapchain resources
	for (int i = 0; i < myAppState->swapchainImageViews.size(); i++) {
		vkDestroyImageView(myAppState->device, myAppState->swapchainImageViews[i], nullptr);
	}
}

VkCommandPoolCreateInfo CommandPoolCreateInfo(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags = 0) {
	return VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = flags,
		.queueFamilyIndex = queueFamilyIndex,
	};
}

VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool& commandPool, const uint32_t count = 1) {
	return VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = count,
	};
}

VkFenceCreateInfo FenceCreateInfo(const VkFenceCreateFlags flags = 0) {
	return VkFenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = flags,
	};
}

VkSemaphoreCreateInfo SemaphoreCreateInfo(const VkSemaphoreCreateFlags flags = 0) {
	return VkSemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.flags = flags,
	};
}

bool InitSyncStructures(MyAppState* myAppState) {
	const VkFenceCreateInfo fenceCreateInfo = FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	const VkSemaphoreCreateInfo semaphoreCreateInfo = SemaphoreCreateInfo();

	for (auto& frame : myAppState->frames) {
		if (vkCreateFence(myAppState->device, &fenceCreateInfo, nullptr, &frame.renderFence) != VK_SUCCESS) {
			SDL_Log("Couldn't create fence: %s", SDL_GetError());
			return false;
		}

		if (vkCreateSemaphore(myAppState->device, &semaphoreCreateInfo, nullptr, &frame.swapchainSemaphore) != VK_SUCCESS) {
			SDL_Log("Couldn't create swapchain semaphore: %s", SDL_GetError());
			return false;
		}
		if (vkCreateSemaphore(myAppState->device, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore) != VK_SUCCESS) {
			SDL_Log("Couldn't create render semaphore: %s", SDL_GetError());
			return false;
		}
	}

	return true;
}

/// Returns true on success, false on failure
bool InitCommands(MyAppState* myAppState) {
	const VkCommandPoolCreateInfo commandPoolCreateInfo = CommandPoolCreateInfo(myAppState->graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (auto& frame : myAppState->frames) {
		if (vkCreateCommandPool(myAppState->device, &commandPoolCreateInfo, nullptr, &frame.commandPool) != VK_SUCCESS) {
			SDL_Log("Couldn't create command pool: %s", SDL_GetError());
			return false;
		}

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = CommandBufferAllocateInfo(frame.commandPool, 1);

		if (vkAllocateCommandBuffers(myAppState->device, &commandBufferAllocateInfo, &frame.mainCommandBuffer) != VK_SUCCESS) {
			SDL_Log("Couldn't allocate command buffer: %s", SDL_GetError());
			return false;
		}
	}

	return true;
}

/// Returns true on success, false on failure
bool InitVulkan(MyAppState* myAppState) {
	//TODO: Maybe try SDL_Vulkan_LoadLibrary instead of Volk?
	//Attempt to load Vulkan loader from the system
	if (const VkResult result = volkInitialize(); result != VK_SUCCESS) {
		SDL_Log("Couldn't initialize Volk: %d", result);
		return false;
	}

	//Make the Vulkan instance, with some debug features
	vkb::InstanceBuilder builder;
	builder.set_app_name(myAppState->name.c_str())
	       .set_app_version(1, 0, 0)
	       .require_api_version(1, 3, 0);
	if constexpr (debug_mode) {
		builder.request_validation_layers(true)
		       .use_default_debug_messenger();
	}
	vkb::Result<vkb::Instance> inst_ret = builder.build();

	if (!inst_ret.has_value()) {
		SDL_Log("Couldn't create Vulkan instance: %s", inst_ret.error().message().c_str());
		return false;
	}

	//Yoink the actual instance and debug messenger from the result
	const vkb::Instance vkbInstance = inst_ret.value();
	myAppState->instance = vkbInstance.instance;
	if constexpr (debug_mode) {
		myAppState->debugMessenger = vkbInstance.debug_messenger;
	}

	//Load all required Vulkan entrypoints, including all extensions; you can use Vulkan from here on as usual.
	volkLoadInstance(myAppState->instance);

	//Create a surface for the window
	if (!SDL_Vulkan_CreateSurface(myAppState->window, myAppState->instance, nullptr, &myAppState->surface)) {
		SDL_Log("Couldn't create Vulkan surface: %s", SDL_GetError());
	}

	VkPhysicalDeviceVulkan13Features features13{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = true,
		.dynamicRendering = true,
	};
	VkPhysicalDeviceVulkan12Features features12{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = true,
		.bufferDeviceAddress = true,
	};

	//Use VkBootstrap to select a physical device
	vkb::PhysicalDeviceSelector selector(vkbInstance);
	vkb::Result<vkb::PhysicalDevice> physDev_ret = selector
	                                               .set_minimum_version(1, 3)
	                                               .set_required_features_13(features13)
	                                               .set_required_features_12(features12)
	                                               .set_surface(myAppState->surface)
	                                               .select();

	if (!physDev_ret.has_value()) {
		SDL_Log("Couldn't select physical device: %s", physDev_ret.error().message().c_str());
		return false;
	}

	//Yoink the physical device from the result
	const vkb::PhysicalDevice& vkbPhysicalDevice = physDev_ret.value();

	//Use VkBootstrap to create the final Vulkan Device
	vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
	vkb::Result<vkb::Device> vkbDevice = deviceBuilder
		.build();

	if (!vkbDevice.has_value()) {
		SDL_Log("Couldn't create Vulkan device: %s", vkbDevice.error().message().c_str());
		return false;
	}

	//Yoink the device from the result
	myAppState->device = vkbDevice.value().device;
	myAppState->physicalDevice = vkbPhysicalDevice.physical_device;

	//Set up the Queue
	vkb::QueueType queueType = vkb::QueueType::graphics;
	vkb::Result<VkQueue> vkQueue = vkbDevice->get_queue(queueType);
	if (!vkQueue.has_value()) {
		SDL_Log("Couldn't get graphics queue: %s", vkQueue.error().message().c_str());
		return false;
	}
	myAppState->graphicsQueue = vkQueue.value();

	vkb::Result<uint32_t> queueIndex = vkbDevice->get_queue_index(queueType);
	if (!queueIndex.has_value()) {
		SDL_Log("Couldn't get graphics queue index: %s", queueIndex.error().message().c_str());
		return false;
	}
	myAppState->graphicsQueueFamilyIndex = queueIndex.value();

	return true;
}

// ReSharper disable twice CppParameterNeverUsed
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	MyAppState* myAppState = new MyAppState();
	*appstate = myAppState;

	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
	myAppState->window = SDL_CreateWindow(myAppState->name.c_str(), 1280, 720, windowFlags);

	if (myAppState->window == nullptr) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!InitVulkan(myAppState)) {
		//SDL Log is handled in the function itself
		return SDL_APP_FAILURE;
	}

	if (!InitSwapchain(myAppState)) {
		//SDL Log is handled in the function itself
		return SDL_APP_FAILURE;
	}

	if (!InitCommands(myAppState)) {
		//SDL Log is handled in the function itself
		return SDL_APP_FAILURE;
	}

	if (!InitSyncStructures(myAppState)) {
		//SDL Log is handled in the function itself
		return SDL_APP_FAILURE;
	}

	// if (!InitPipelines()) {
	//
	// }

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

VkCommandBufferBeginInfo CommandBufferBeginInfo(const VkCommandBufferUsageFlags flags = 0) {
	return VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = flags,
		.pInheritanceInfo = nullptr,
	};
}

VkImageSubresourceRange ImageSubresourceRange(const VkImageAspectFlags aspectMask) {
	return VkImageSubresourceRange{
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS,
	};
}

void TransitionImage(const VkCommandBuffer& commandBuffer, const VkImage& image, const VkImageLayout currentLayout, const VkImageLayout newLayout) {
	const VkImageAspectFlags aspectMask = newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		                                      ? VK_IMAGE_ASPECT_DEPTH_BIT
		                                      : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageMemoryBarrier2 imageBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
		.oldLayout = currentLayout,
		.newLayout = newLayout,
		.image = image,
		.subresourceRange = ImageSubresourceRange(aspectMask),
	};

	const VkDependencyInfo depInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier,
	};

	vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

VkSemaphoreSubmitInfo SemaphoreSubmitInfo(const VkPipelineStageFlags2 stageMask, const VkSemaphore& semaphore) {
	return VkSemaphoreSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = semaphore,
		.value = 1,
		.stageMask = stageMask,
		.deviceIndex = 0,
	};
}

VkCommandBufferSubmitInfo CommandBufferSubmitInfo(const VkCommandBuffer& commandBuffer) {
	return VkCommandBufferSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = commandBuffer,
		.deviceMask = 0,
	};
}

VkSubmitInfo2 SubmitInfo(const VkCommandBufferSubmitInfo* commandBuffer, const VkSemaphoreSubmitInfo* signalSemaphoreInfo, const VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
	return VkSubmitInfo2{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0u : 1u,
		.pWaitSemaphoreInfos = waitSemaphoreInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = commandBuffer,
		.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0u : 1u,
		.pSignalSemaphoreInfos = signalSemaphoreInfo
	};
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	constexpr uint64_t secondInNanoseconds = 1'000'000'000;

	if (vkWaitForFences(myAppState->device, 1, &myAppState->GetCurrentFrame().renderFence, true, secondInNanoseconds) != VK_SUCCESS) {
		SDL_Log("Couldn't wait for fence: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	if (vkResetFences(myAppState->device, 1, &myAppState->GetCurrentFrame().renderFence) != VK_SUCCESS) {
		SDL_Log("Couldn't reset fence: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	uint32_t swapchainImageIndex;
	if (vkAcquireNextImageKHR(myAppState->device, myAppState->swapchain, secondInNanoseconds, myAppState->GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex) != VK_SUCCESS) {
		SDL_Log("Couldn't acquire next image: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	const VkCommandBuffer& commandBuffer = myAppState->GetCurrentFrame().mainCommandBuffer;

	if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS) {
		SDL_Log("Couldn't reset command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	const VkCommandBufferBeginInfo commandBufferBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
		SDL_Log("Couldn't begin command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	TransitionImage(commandBuffer, myAppState->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	const float flash = std::abs(math::sin(static_cast<float>(myAppState->frameNumber) / 120.f));
	const VkClearColorValue clearValue = {
		.float32 = {0.0f, 0.0f, flash, 1.0f},
	};

	const VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	vkCmdClearColorImage(commandBuffer, myAppState->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	TransitionImage(commandBuffer, myAppState->swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalise
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		SDL_Log("Couldn't end command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	const VkCommandBufferSubmitInfo commandBufferSubmitInfo = CommandBufferSubmitInfo(commandBuffer);

	const VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, myAppState->GetCurrentFrame().swapchainSemaphore);
	const VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, myAppState->GetCurrentFrame().renderSemaphore);

	const VkSubmitInfo2 submit = SubmitInfo(&commandBufferSubmitInfo, &signalInfo, &waitInfo);

	if (vkQueueSubmit2(myAppState->graphicsQueue, 1, &submit, myAppState->GetCurrentFrame().renderFence) != VK_SUCCESS) {
		SDL_Log("Couldn't submit command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	const VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &myAppState->GetCurrentFrame().renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &myAppState->swapchain,
		.pImageIndices = &swapchainImageIndex,
	};

	if (vkQueuePresentKHR(myAppState->graphicsQueue, &presentInfo) != VK_SUCCESS) {
		SDL_Log("Couldn't present image: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	myAppState->frameNumber++;

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, const SDL_AppResult result) {
	SDL_Log("Exiting with result %d", result);

	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	if (result == SDL_APP_SUCCESS) {
		vkDeviceWaitIdle(myAppState->device);

		for (auto& frame : myAppState->frames) {
			vkDestroyCommandPool(myAppState->device, frame.commandPool, nullptr);

			vkDestroyFence(myAppState->device, frame.renderFence, nullptr);
			vkDestroySemaphore(myAppState->device, frame.swapchainSemaphore, nullptr);
			vkDestroySemaphore(myAppState->device, frame.renderSemaphore, nullptr);
		}

		DestroySwapchain(myAppState);

		vkDestroySurfaceKHR(myAppState->instance, myAppState->surface, nullptr);

		vkDestroyDevice(myAppState->device, nullptr);
		if constexpr (debug_mode) {
			vkb::destroy_debug_utils_messenger(myAppState->instance, myAppState->debugMessenger);
		}
		vkDestroyInstance(myAppState->instance, nullptr);
	}

	delete myAppState;
}
