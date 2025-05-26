#include "vk_engine.hpp"

// SDL3
#include "SDL3/SDL_vulkan.h"

// RSL Math
#define RSL_DEFAULT_ALIGNED_MATH false
#include <rsl/math>

// Vulkan Helper Libraries
#include <VkBootstrap.h>
#include <volk.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

// Engine
#include "vk_images.hpp"
#include "vk_initializers.hpp"
#include "vk_macros.hpp"
#include "vk_pipelines.hpp"

#pragma region Vulkan Initialization

SDL_AppResult VulkanEngine::InitVulkan() {
	//Attempt to load Vulkan loader from the system
	VK_CHECK(volkInitialize(), "Couldn't initialize Volk");

	//Make the Vulkan instance, with some debug features
	vkb::InstanceBuilder builder;
	builder.set_app_name(name.c_str())
	       .set_app_version(1, 0, 0)
	       .require_api_version(1, 3, 0);
	if (debugMode) {
		builder.request_validation_layers(true)
		       .use_default_debug_messenger();
	}
	vkb::Result<vkb::Instance> resVkbInstance = builder.build();

	if (!resVkbInstance.has_value()) {
		SDL_Log("Couldn't create Vulkan instance: %s", resVkbInstance.error().message().c_str());
		return SDL_APP_FAILURE;
	}

	//Yoink the actual instance and debug messenger from the result
	const vkb::Instance vkbInstance = resVkbInstance.value();
	instance = vkbInstance.instance;
	if (debugMode) {
		debugMessenger = vkbInstance.debug_messenger;
	}

	//Load all required Vulkan entrypoints, including all extensions; you can use Vulkan from here on as usual.
	volkLoadInstance(instance);

	//Create a surface for the window
	if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
		SDL_Log("Couldn't create Vulkan surface: %s", SDL_GetError());
		return SDL_APP_FAILURE;
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
	vkb::Result<vkb::PhysicalDevice> resVkbPhysicalDevice = selector
	                                                        .set_minimum_version(1, 3)
	                                                        .set_required_features_13(features13)
	                                                        .set_required_features_12(features12)
	                                                        .set_surface(surface)
	                                                        .select();

	if (!resVkbPhysicalDevice.has_value()) {
		SDL_Log("Couldn't select physical device: %s", resVkbPhysicalDevice.error().message().c_str());
		return SDL_APP_FAILURE;
	}

	//Yoink the physical device from the result
	const vkb::PhysicalDevice& vkbPhysicalDevice = resVkbPhysicalDevice.value();

	//Use VkBootstrap to create the final Vulkan Device
	vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
	vkb::Result<vkb::Device> resVkbDevice = deviceBuilder
		.build();

	if (!resVkbDevice.has_value()) {
		SDL_Log("Couldn't create Vulkan device: %s", resVkbDevice.error().message().c_str());
		return SDL_APP_FAILURE;
	}

	//Yoink the device from the result
	vkb::Device vkbDevice = std::move(resVkbDevice.value());
	device = vkbDevice.device;
	physicalDevice = vkbPhysicalDevice.physical_device;
	volkLoadDevice(device);

	//Set up the Queue
	vkb::QueueType queueType = vkb::QueueType::graphics;
	vkb::Result<VkQueue> resVkbQueue = vkbDevice.get_queue(queueType);
	if (!resVkbQueue.has_value()) {
		SDL_Log("Couldn't get graphics queue: %s", resVkbQueue.error().message().c_str());
		return SDL_APP_FAILURE;
	}
	graphicsQueue = resVkbQueue.value();

	vkb::Result<uint32_t> queueIndex = vkbDevice.get_queue_index(queueType);
	if (!queueIndex.has_value()) {
		SDL_Log("Couldn't get graphics queue index: %s", queueIndex.error().message().c_str());
		return SDL_APP_FAILURE;
	}
	graphicsQueueFamilyIndex = queueIndex.value();

	// Set up VMA
	VmaAllocatorCreateInfo allocatorCreateInfo{
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance,
		.vulkanApiVersion = vkbInstance.api_version,
	};

	//Make VMA play nice with Volk
	VmaVulkanFunctions vmaVulkanFunctions{};
	vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vmaVulkanFunctions);
	allocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;

	VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator), "Couldn't create VMA allocator");

	mainDeletionQueue.PushFunction([&] { vmaDestroyAllocator(vmaAllocator); });

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitCommands() {
	const VkCommandPoolCreateInfo commandPoolCreateInfo = vk_init::CommandPoolCreateInfo(graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (FrameData& frame : frames) {
		VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &frame.commandPool), "Couldn't create command pool");

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = vk_init::CommandBufferAllocateInfo(frame.commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &frame.mainCommandBuffer), "Couldn't allocate command buffer");
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitSyncStructures() {
	const VkFenceCreateInfo fenceCreateInfo = vk_init::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	const VkSemaphoreCreateInfo semaphoreCreateInfo = vk_init::SemaphoreCreateInfo();

	for (FrameData& frame : frames) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frame.renderFence), "Couldn't create fence");

		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frame.swapchainSemaphore), "Couldn't create swapchain semaphore");
	}

	readyForPresentSemaphores.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &readyForPresentSemaphores[i]), "Couldn't create semaphore");
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::CreateSwapchain(const uint32_t width, const uint32_t height) {
	swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	const VkSurfaceFormatKHR desiredFormat{
		.format = swapchainImageFormat,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	};

	vkb::SwapchainBuilder swapchainBuilder(physicalDevice, device, surface);
	vkb::Result<vkb::Swapchain> retVkbSwapchain = swapchainBuilder
	                                              // .use_default_format_selection()
	                                              .set_desired_format(desiredFormat)
	                                              .set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
	                                              .set_desired_extent(width, height)
	                                              .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	                                              .build();

	if (!retVkbSwapchain.has_value()) {
		SDL_Log("Couldn't create swapchain: %s", retVkbSwapchain.error().message().c_str());
		return SDL_APP_FAILURE;
	}

	//Yoink the swapchain from the result
	vkb::Swapchain vkbSwapchain = retVkbSwapchain.value();
	swapchain = vkbSwapchain.swapchain;
	swapchainExtent = vkbSwapchain.extent;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitSwapchain() {
	int32_t width, height;
	if (!SDL_GetWindowSize(window, &width, &height)) {
		SDL_Log("Couldn't get window size: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	const uint32_t uWidth = static_cast<uint32_t>(width);
	const uint32_t uHeight = static_cast<uint32_t>(height);

	//draw image size will match the window
	if (const SDL_AppResult res = CreateSwapchain(uWidth, uHeight); res != SDL_APP_CONTINUE) {
		return res;
	}

	const VkExtent3D drawImageExtent = {uWidth, uHeight, 1};

	//hardcoding the draw format to 32-bit float
	drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const VkImageCreateInfo drawImageCreateInfo = vk_init::ImageCreateInfo(drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	constexpr VmaAllocationCreateInfo drawImageAllocationInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags{VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
	};
	//allocate and create the image
	VK_CHECK(vmaCreateImage(vmaAllocator, &drawImageCreateInfo, &drawImageAllocationInfo, &drawImage.image, &drawImage.allocation, nullptr), "Couldn't create image");

	//build an image-view for the draw image to use for rendering
	const VkImageViewCreateInfo drawImageViewCreateInfo = vk_init::ImageViewCreateInfo(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(device, &drawImageViewCreateInfo, nullptr, &drawImage.imageView), "Couldn't create image view");

	//add to deletion queues
	mainDeletionQueue.PushFunction([&] {
		vkDestroyImageView(device, drawImage.imageView, nullptr);
		vmaDestroyImage(vmaAllocator, drawImage.image, drawImage.allocation);
	});

	return SDL_APP_CONTINUE;
}

void VulkanEngine::DestroySwapchain() const {
	vkDestroySwapchainKHR(device, swapchain, nullptr);

	//Destroy swapchain resources
	for (const VkImageView& imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	for (const VkSemaphore& semaphore : readyForPresentSemaphores) {
		vkDestroySemaphore(device, semaphore, nullptr);
	}
}

SDL_AppResult VulkanEngine::InitDescriptors() {
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector sizes =
	{
		DescriptorAllocator::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
	};

	globalDescriptorAllocator.InitPool(device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		const std::optional<VkDescriptorSetLayout> buildResult = builder.Build(device, VK_SHADER_STAGE_COMPUTE_BIT);
		if (!buildResult.has_value()) {
			SDL_Log("Couldn't create descriptor set layout for draw image");
			return SDL_APP_FAILURE;
		}
		drawImageDescriptorLayout = buildResult.value();
	}

	//allocate a descriptor set for our draw image
	const std::optional<VkDescriptorSet> allocationResult = globalDescriptorAllocator.Allocate(device, drawImageDescriptorLayout);
	if (!allocationResult.has_value()) {
		SDL_Log("Couldn't allocate descriptor set for draw image");
		return SDL_APP_FAILURE;
	}
	drawImageDescriptors = allocationResult.value();

	VkDescriptorImageInfo imgInfo{
		.imageView = drawImage.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	const VkWriteDescriptorSet drawImageWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = drawImageDescriptors,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imgInfo,
	};

	vkUpdateDescriptorSets(device, 1, &drawImageWrite, 0, nullptr);

	//make sure both the descriptor allocator and the new layout get cleaned up properly
	mainDeletionQueue.PushFunction([&] {
		globalDescriptorAllocator.DestroyPool(device);
		vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
	});

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitPipelines() {
	if (const SDL_AppResult res = InitBackgroundPipelines(); res != SDL_APP_CONTINUE) {
		return res;
	}
	if (const SDL_AppResult res = InitTrianglePipelines(); res != SDL_APP_CONTINUE) {
		return res;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitBackgroundPipelines() {
	const VkPipelineLayoutCreateInfo computeLayout{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &drawImageDescriptorLayout,
	};

	VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &gradientPipelineLayout), "Couldn't create pipeline layout");

	const std::filesystem::path fullPath = GetAssetsDir() / "shaders/compiled/" / "gradient.comp.spv";

	const std::optional<VkShaderModule> computeDrawShaderResult = vk_util::LoadShaderModule(fullPath.string().c_str(), device);
	if (!computeDrawShaderResult.has_value()) {
		SDL_Log("Couldn't load compute shader module");
		return SDL_APP_FAILURE;
	}
	const VkShaderModule& computeDrawShader = computeDrawShaderResult.value();

	const VkPipelineShaderStageCreateInfo stageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = computeDrawShader,
		.pName = "main",
	};

	const VkComputePipelineCreateInfo computePipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.stage = stageCreateInfo,
		.layout = gradientPipelineLayout,
	};

	VK_CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &gradientPipeline), "Couldn't create compute pipeline");

	vkDestroyShaderModule(device, computeDrawShader, nullptr);

	mainDeletionQueue.PushFunction([&] {
		vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
		vkDestroyPipeline(device, gradientPipeline, nullptr);
	});

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitTrianglePipelines() {
	VkShaderModule triangleFragShader;
	{
		const std::filesystem::path fullPath = GetAssetsDir() / "shaders/compiled/" / "triangle.frag.spv";
		if (const std::optional<VkShaderModule> triangleFragShaderResult = vk_util::LoadShaderModule(fullPath.string().c_str(), device); !triangleFragShaderResult.has_value()) {
			SDL_Log("Couldn't load triangle fragment shader module");
			return SDL_APP_FAILURE;
		} else {
			triangleFragShader = triangleFragShaderResult.value();
		}
	}

	VkShaderModule triangleVertShader;
	{
		const std::filesystem::path fullPath = GetAssetsDir() / "shaders/compiled/" / "triangle.vert.spv";
		if (const std::optional<VkShaderModule> triangleVertShaderResult = vk_util::LoadShaderModule(fullPath.string().c_str(), device); !triangleVertShaderResult.has_value()) {
			SDL_Log("Couldn't load triangle vertex shader module");
			return SDL_APP_FAILURE;
		} else {
			triangleVertShader = triangleVertShaderResult.value();
		}
	}

	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk_init::PipelineLayoutCreateInfo();
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &trianglePipelineLayout), "Couldn't create triangle pipeline layout");
	PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder.pipelineLayout = trianglePipelineLayout;
	pipelineBuilder.SetShaders(triangleVertShader, triangleFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultiSamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.DisableDepthTest();

	//connect the image format we will draw into, from draw image
	pipelineBuilder.SetColourAttachmentFormat(drawImage.imageFormat);
	pipelineBuilder.SetDepthFormat(VK_FORMAT_UNDEFINED);

	//finally build the pipeline
	const std::optional<VkPipeline> pipelineResult = pipelineBuilder.BuildPipeline(device);
	if (!pipelineResult.has_value()) {
		SDL_Log("Couldn't build triangle pipeline");
		return SDL_APP_FAILURE;
	}
	trianglePipeline = pipelineResult.value();

	//clean structures
	vkDestroyShaderModule(device, triangleFragShader, nullptr);
	vkDestroyShaderModule(device, triangleVertShader, nullptr);

	mainDeletionQueue.PushFunction([&] {
		vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
		vkDestroyPipeline(device, trianglePipeline, nullptr);
	});

	return SDL_APP_CONTINUE;
}

#pragma endregion

VulkanEngine::VulkanEngine(std::string name, const bool debugMode)
	: name(std::move(name)),
	  debugMode(debugMode) {
}

std::filesystem::path VulkanEngine::GetAssetsDir() const {
	const std::filesystem::path assetsDirName = name + "-assets";
	return SDL_GetBasePath() / assetsDirName;
}

SDL_AppResult VulkanEngine::Init(const int width, const int height) {
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
	window = SDL_CreateWindow(name.c_str(), width, height, windowFlags);

	if (window == nullptr) {
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (const SDL_AppResult res = InitVulkan(); res != SDL_APP_CONTINUE) {
		return res;
	}

	if (const SDL_AppResult res = InitSwapchain(); res != SDL_APP_CONTINUE) {
		return res;
	}

	if (const SDL_AppResult res = InitCommands(); res != SDL_APP_CONTINUE) {
		return res;
	}

	if (const SDL_AppResult res = InitSyncStructures(); res != SDL_APP_CONTINUE) {
		return res;
	}

	if (const SDL_AppResult res = InitDescriptors(); res != SDL_APP_CONTINUE) {
		return res;
	}

	if (const SDL_AppResult res = InitPipelines(); res != SDL_APP_CONTINUE) {
		return res;
	}

	return SDL_APP_CONTINUE;
}

void VulkanEngine::DrawBackground(const VkCommandBuffer& commandBuffer, const VkImage& image) const {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout, 0, 1, &drawImageDescriptors, 0, nullptr);

	vkCmdDispatch(commandBuffer, std::ceil(drawExtent.width / 16.0), std::ceil(drawExtent.height / 16.0), 1);
}

void VulkanEngine::DrawGeometry(const VkCommandBuffer& commandBuffer) const {
	//begin a render pass  connected to our draw image
	const VkRenderingAttachmentInfo colorAttachment = vk_init::AttachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	const VkRenderingInfo renderInfo = vk_init::RenderingInfo(drawExtent, &colorAttachment, nullptr);
	vkCmdBeginRendering(commandBuffer, &renderInfo);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);

	//set dynamic viewport and scissor
	const VkViewport viewport{
		.x = 0,
		.y = 0,
		.width = static_cast<float>(drawExtent.width),
		.height = static_cast<float>(drawExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	const VkRect2D scissor = {
		.offset = VkOffset2D{
			.x = 0,
			.y = 0
		},
		.extent = VkExtent2D{
			.width = drawExtent.width,
			.height = drawExtent.height,
		},
	};

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRendering(commandBuffer);
}


SDL_AppResult VulkanEngine::Draw() {
	constexpr uint64_t secondInNanoseconds = 1'000'000'000;

	VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, secondInNanoseconds), "Couldn't wait for fence");

	GetCurrentFrame().frameDeletionQueue.Flush();

	VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence), "Couldn't reset fence");

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, secondInNanoseconds, GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex), "Couldn't acquire next image");

	const VkCommandBuffer& commandBuffer = GetCurrentFrame().mainCommandBuffer;

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0), "Couldn't reset command buffer");

	const VkCommandBufferBeginInfo commandBufferBeginInfo = vk_init::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	drawExtent.width = drawImage.imageExtent.width;
	drawExtent.height = drawImage.imageExtent.height;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo), "Couldn't begin command buffer");

	// transition our main draw image into general layout so we can write into it.
	// we will overwrite it all so we don't care about what was the older layout
	vk_util::TransitionImage(commandBuffer, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(commandBuffer, drawImage.image);

	vk_util::TransitionImage(commandBuffer, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	DrawGeometry(commandBuffer);

	//transition the draw image and the swapchain image into their correct transfer layouts
	vk_util::TransitionImage(commandBuffer, drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vk_util::TransitionImage(commandBuffer, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vk_util::CopyImageToImage(commandBuffer, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

	// set swapchain image layout to Present so we can show it on the screen
	vk_util::TransitionImage(commandBuffer, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(commandBuffer), "Couldn't end command buffer");

	const VkCommandBufferSubmitInfo commandBufferSubmitInfo = vk_init::CommandBufferSubmitInfo(commandBuffer);

	const VkSemaphoreSubmitInfo waitInfo = vk_init::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchainSemaphore);
	const VkSemaphoreSubmitInfo signalInfo = vk_init::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, readyForPresentSemaphores[swapchainImageIndex]);

	const VkSubmitInfo2 submit = vk_init::SubmitInfo(&commandBufferSubmitInfo, &signalInfo, &waitInfo);
	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, GetCurrentFrame().renderFence), "Couldn't submit command buffer");

	const VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &readyForPresentSemaphores[swapchainImageIndex],
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &swapchainImageIndex,
	};

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo), "Couldn't present image");

	frameNumber++;

	return SDL_APP_CONTINUE;
}

void VulkanEngine::Cleanup(const SDL_AppResult result) {
	if (result == SDL_APP_SUCCESS) {
		vkDeviceWaitIdle(device);

		for (FrameData& frame : frames) {
			vkDestroyCommandPool(device, frame.commandPool, nullptr);

			vkDestroyFence(device, frame.renderFence, nullptr);
			vkDestroySemaphore(device, frame.swapchainSemaphore, nullptr);

			frame.frameDeletionQueue.Flush();
		}

		mainDeletionQueue.Flush();

		DestroySwapchain();

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);

		if (debugMode) {
			vkb::destroy_debug_utils_messenger(instance, debugMessenger);
		}
		vkDestroyInstance(instance, nullptr);

		SDL_DestroyWindow(window);
	}
}
