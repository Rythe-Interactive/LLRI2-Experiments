// Impl
#include "vk_engine.hpp"

// Vulkan Helper Libraries
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

// Engine
#include "vk_images.hpp"
#include "vk_initializers.hpp"
#include "vk_loader.hpp"
#include "vk_macros.hpp"
#include "vk_pipelines.hpp"

// ReSharper disable CppDFAConstantParameter
// ReSharper disable CppDFAConstantConditions
// ReSharper disable CppDFAUnreachableCode
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

	VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &immediateSubmitCommandPool), "Couldn't create immediate submit command pool");

	const VkCommandBufferAllocateInfo immediateCommandBufferAllocateInfo = vk_init::CommandBufferAllocateInfo(immediateSubmitCommandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(device, &immediateCommandBufferAllocateInfo, &immediateSubmitCommandBuffer), "Couldn't allocate immediate command buffer");
	mainDeletionQueue.PushFunction([&] { vkDestroyCommandPool(device, immediateSubmitCommandPool, nullptr); });

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitSyncStructures() {
	const VkFenceCreateInfo fenceCreateInfo = vk_init::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	const VkSemaphoreCreateInfo semaphoreCreateInfo = vk_init::SemaphoreCreateInfo();

	for (FrameData& frame : frames) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frame.renderFence), "Couldn't create fence");

		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frame.swapchainSemaphore), "Couldn't create swapchain semaphore");
	}

	VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immediateSubmitFence), "Couldn't create immediate submit fence");
	mainDeletionQueue.PushFunction([&] { vkDestroyFence(device, immediateSubmitFence, nullptr); });

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

	readyForPresentSemaphores.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &readyForPresentSemaphores[i]), "Couldn't create ready for present semaphore");
	}

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

	// Depth Image
	depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	depthImage.imageExtent = drawImageExtent;

	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	const VkImageCreateInfo depthImageCreateInfo = vk_init::ImageCreateInfo(depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	VK_CHECK(vmaCreateImage(vmaAllocator, &depthImageCreateInfo, &drawImageAllocationInfo, &depthImage.image, &depthImage.allocation, nullptr), "Couldn't create depth image");

	//build an image-view for the depth image to use for rendering
	const VkImageViewCreateInfo depthImageViewCreateInfo = vk_init::ImageViewCreateInfo(depthImage.imageFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImage.imageView), "Couldn't create depth image view");

	//add to deletion queues
	mainDeletionQueue.PushFunction([&] {
		vkDestroyImageView(device, drawImage.imageView, nullptr);
		vmaDestroyImage(vmaAllocator, drawImage.image, drawImage.allocation);

		vkDestroyImageView(device, depthImage.imageView, nullptr);
		vmaDestroyImage(vmaAllocator, depthImage.image, depthImage.allocation);
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

SDL_AppResult VulkanEngine::ResizeSwapchain() {
	vkDeviceWaitIdle(device);

	DestroySwapchain();

	int32_t width, height;
	if (!SDL_GetWindowSize(window, &width, &height)) {
		SDL_Log("Couldn't get window size: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	const uint32_t uWidth = static_cast<uint32_t>(width);
	const uint32_t uHeight = static_cast<uint32_t>(height);

	if (const SDL_AppResult res = CreateSwapchain(uWidth, uHeight); res != SDL_APP_CONTINUE) {
		return res;
	}
	resizeRequested = false;

	return SDL_APP_CONTINUE;
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
			SDL_Log("Couldn't create descriptor set layout for compute draw image");
			return SDL_APP_FAILURE;
		}
		drawImageDescriptorLayout = buildResult.value();
	}
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		const std::optional<VkDescriptorSetLayout> buildResult = builder.Build(device, VK_SHADER_STAGE_FRAGMENT_BIT);
		if (!buildResult.has_value()) {
			SDL_Log("Couldn't create descriptor set layout for single image");
			return SDL_APP_FAILURE;
		}
		singleImageDescriptorLayout = buildResult.value();
	}
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		const std::optional<VkDescriptorSetLayout> buildResult = builder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		if (!buildResult.has_value()) {
			SDL_Log("Couldn't create descriptor set layout for GPU scene data");
			return SDL_APP_FAILURE;
		}
		gpuSceneDataDescriptorLayout = buildResult.value();
	}

	//allocate a descriptor set for our draw image
	const std::optional<VkDescriptorSet> allocationResult = globalDescriptorAllocator.Allocate(device, drawImageDescriptorLayout);
	if (!allocationResult.has_value()) {
		SDL_Log("Couldn't allocate descriptor set for draw image");
		return SDL_APP_FAILURE;
	}
	drawImageDescriptors = allocationResult.value();

	{
		DescriptorWriter writer;
		writer.WriteImage(0, drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.UpdateSet(device, drawImageDescriptors);
	}

	//make sure both the descriptor allocator and the new layout get cleaned up properly
	mainDeletionQueue.PushFunction([&] {
		globalDescriptorAllocator.DestroyPool(device);

		vkDestroyDescriptorSetLayout(device, gpuSceneDataDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, singleImageDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
	});

	for (FrameData& frame : frames) {
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
		};

		frame.frameDescriptors = DescriptorAllocatorGrowable{};
		frame.frameDescriptors.InitPools(device, 1000, frameSizes);

		mainDeletionQueue.PushFunction([&, frame = &frame] { frame->frameDescriptors.DestroyPools(device); });
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitPipelines() {
	if (const SDL_AppResult res = InitBackgroundPipelines(); res != SDL_APP_CONTINUE) {
		return res;
	}
	if (const SDL_AppResult res = InitMeshPipeline(); res != SDL_APP_CONTINUE) {
		return res;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitBackgroundPipelines() {
	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(ComputePushConstants),
	};

	const VkPipelineLayoutCreateInfo computeLayout{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &drawImageDescriptorLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};

	VkPipelineLayout computePipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &computePipelineLayout), "Couldn't create pipeline layout");

	const std::filesystem::path compiledShadersPath = GetAssetsDir() / "shaders/compiled/";

	VkShaderModule gradientShader;
	{
		const std::filesystem::path gradientShaderPath = compiledShadersPath / "gradient_colour.comp.spv";
		const std::optional<VkShaderModule> gradientShaderResult = vk_util::LoadShaderModule(gradientShaderPath.string().c_str(), device);
		if (!gradientShaderResult.has_value()) {
			SDL_Log("Couldn't load compute shader module: %s", gradientShaderPath.string().c_str());
			return SDL_APP_FAILURE;
		}
		gradientShader = gradientShaderResult.value();
	}
	VkShaderModule skyShader;
	{
		const std::filesystem::path skyShaderPath = compiledShadersPath / "sky.comp.spv";
		const std::optional<VkShaderModule> skyShaderResult = vk_util::LoadShaderModule(skyShaderPath.string().c_str(), device);
		if (!skyShaderResult.has_value()) {
			SDL_Log("Couldn't load compute shader module: %s", skyShaderPath.string().c_str());
			return SDL_APP_FAILURE;
		}
		skyShader = skyShaderResult.value();
	}

	const VkPipelineShaderStageCreateInfo stageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = gradientShader,
		.pName = "main",
	};

	VkComputePipelineCreateInfo computePipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.stage = stageCreateInfo,
		.layout = computePipelineLayout,
	};

	ComputeEffect gradientEffect{
		.name = "gradient",
		.layout = computePipelineLayout,
		.data = ComputePushConstants{
			//default colours
			.data1 = math::float4{1.0f, 0.0f, 0.0f, 1.0f}, // Red
			.data2 = math::float4{0.0f, 0.0f, 1.0f, 1.0f}, // Blue
		}
	};

	VK_CHECK(vkCreateComputePipelines(device, nullptr, 1, &computePipelineCreateInfo, nullptr, &gradientEffect.pipeline), "Couldn't create compute pipeline: gradient");

	//reuse the structs we've already made, but with the other shader
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect skyEffect{
		.name = "sky",
		.layout = computePipelineLayout,
		.data = ComputePushConstants{
			//default colours
			.data1 = math::float4{0.1f, 0.2f, 0.4f, 0.97f}, // Light blue
		}
	};
	VK_CHECK(vkCreateComputePipelines(device, nullptr, 1, &computePipelineCreateInfo, nullptr, &skyEffect.pipeline), "Couldn't create compute pipeline: sky");

	//add the effects to the background effects vector
	backgroundEffects.push_back(gradientEffect);
	backgroundEffects.push_back(skyEffect);

	//destroy the shader modules we created
	vkDestroyShaderModule(device, gradientShader, nullptr);
	vkDestroyShaderModule(device, skyShader, nullptr);

	mainDeletionQueue.PushFunction([=, this] {
		vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
		vkDestroyPipeline(device, skyEffect.pipeline, nullptr);
		vkDestroyPipeline(device, gradientEffect.pipeline, nullptr);
	});

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitMeshPipeline() {
	VkShaderModule meshFragShader;
	{
		const std::filesystem::path fullPath = GetAssetsDir() / "shaders/compiled/" / "tex_image.frag.spv";
		if (const std::optional<VkShaderModule> meshFragShaderResult = vk_util::LoadShaderModule(fullPath.string().c_str(), device); !meshFragShaderResult.has_value()) {
			SDL_Log("Couldn't load mesh fragment shader module");
			return SDL_APP_FAILURE;
		} else {
			meshFragShader = meshFragShaderResult.value();
		}
	}

	VkShaderModule meshVertShader;
	{
		const std::filesystem::path fullPath = GetAssetsDir() / "shaders/compiled/" / "triangle.vert.spv";
		if (const std::optional<VkShaderModule> meshVertShaderResult = vk_util::LoadShaderModule(fullPath.string().c_str(), device); !meshVertShaderResult.has_value()) {
			SDL_Log("Couldn't load mesh vertex shader module");
			return SDL_APP_FAILURE;
		} else {
			meshVertShader = meshVertShaderResult.value();
		}
	}

	VkPushConstantRange bufferRange{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(GPUDrawPushConstants),
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk_init::PipelineLayoutCreateInfo(&bufferRange, &singleImageDescriptorLayout);
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &meshPipelineLayout), "Couldn't create mesh pipeline layout");

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.pipelineLayout = meshPipelineLayout;
	pipelineBuilder.SetShaders(meshVertShader, meshFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultiSamplingNone();
	// pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableBlendingAdditive();
	// pipelineBuilder.DisableDepthTest();
	pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.SetColourAttachmentFormat(drawImage.imageFormat);
	pipelineBuilder.SetDepthFormat(depthImage.imageFormat);

	//finally build the pipeline
	const std::optional<VkPipeline> pipelineResult = pipelineBuilder.BuildPipeline(device);
	if (!pipelineResult.has_value()) {
		SDL_Log("Couldn't build mesh pipeline");
		return SDL_APP_FAILURE;
	}
	meshPipeline = pipelineResult.value();

	//clean structures
	vkDestroyShaderModule(device, meshFragShader, nullptr);
	vkDestroyShaderModule(device, meshVertShader, nullptr);

	mainDeletionQueue.PushFunction([&] {
		vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
		vkDestroyPipeline(device, meshPipeline, nullptr);
	});

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer commandBuffer)>&& function) const {
	VK_CHECK(vkResetFences(device, 1, &immediateSubmitFence), "Couldn't reset immediate submit fence");
	VK_CHECK(vkResetCommandBuffer(immediateSubmitCommandBuffer, 0), "Couldn't reset immediate submit command buffer");

	const VkCommandBuffer& commandBuffer = immediateSubmitCommandBuffer;

	const VkCommandBufferBeginInfo cmdBeginInfo = vk_init::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo), "Couldn't begin immediate command buffer");

	function(commandBuffer);

	VK_CHECK(vkEndCommandBuffer(commandBuffer), "Couldn't end immediate command buffer");

	const VkCommandBufferSubmitInfo commandBufferSubmitInfo = vk_init::CommandBufferSubmitInfo(commandBuffer);
	const VkSubmitInfo2 submit = vk_init::SubmitInfo(&commandBufferSubmitInfo, nullptr, nullptr);

	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immediateSubmitFence), "Couldn't submit immediate command buffer");

	VK_CHECK(vkWaitForFences(device, 1, &immediateSubmitFence, true, secondInNanoseconds), "Couldn't wait for immediate submit fence");

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::InitImgui() {
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversized, but it's copied from imgui demo
	//  itself.
	std::array poolSizes = {
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
	};

	const VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &imguiPool), "Couldn't create imgui descriptor pool");

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL3_InitForVulkan(window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo imguiVulkanInitInfo = {
		.Instance = instance,
		.PhysicalDevice = physicalDevice,
		.Device = device,
		.Queue = graphicsQueue,
		.DescriptorPool = imguiPool,
		.MinImageCount = 3,
		.ImageCount = 3,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.UseDynamicRendering = true,
		//dynamic rendering parameters for imgui to use
		.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &swapchainImageFormat,
		},
	};

	ImGui_ImplVulkan_Init(&imguiVulkanInitInfo);

	ImGui_ImplVulkan_CreateFontsTexture();

	// destroy the imgui created structures
	mainDeletionQueue.PushFunction([=, this] {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(device, imguiPool, nullptr);
	});

	return SDL_APP_CONTINUE;
}

// From https://github.com/g-truc/glm/blob/2d4c4b4dd31fde06cfffad7915c2b3006402322f/glm/detail/func_packing.inl#L67C2-L83C3
uint32_t packUnorm4x8(math::float4 const& v) {
	union {
		unsigned char in[4];
		uint out;
	} u;

	math::float4 result = round(clamp(v, 0.0f, 1.0f) * 255.0f);

	u.in[0] = result[0];
	u.in[1] = result[1];
	u.in[2] = result[2];
	u.in[3] = result[3];

	return u.out;
}

SDL_AppResult VulkanEngine::InitDefaultData() {
	const std::filesystem::path fullPath = GetAssetsDir() / "models/suzanne/suzanne.obj";
	// const std::filesystem::path fullPath = GetAssetsDir() / "models/container/blender_quad.obj";
	std::optional<std::vector<std::shared_ptr<MeshAsset>>> meshResult = ImportMesh(this, fullPath);
	if (!meshResult.has_value()) {
		SDL_Log("Couldn't import mesh!");
		return SDL_APP_FAILURE;
	}
	meshes = std::move(meshResult.value());

	//3 default textures, white, grey, black. 1 pixel each
	constexpr VkExtent3D pixelSize{1, 1, 1};

	// white
	const uint32_t white = packUnorm4x8(math::float4(1, 1, 1, 1));
	const std::optional<AllocatedImage> whiteImageResult = CreateImage(&white, pixelSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!whiteImageResult.has_value()) {
		SDL_Log("Couldn't create white image");
		return SDL_APP_FAILURE;
	}
	whiteImage = whiteImageResult.value();

	// grey
	const uint32_t grey = packUnorm4x8(math::float4(0.66f, 0.66f, 0.66f, 1));
	const std::optional<AllocatedImage> greyImageResult = CreateImage(&grey, pixelSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!greyImageResult.has_value()) {
		SDL_Log("Couldn't create grey image");
		return SDL_APP_FAILURE;
	}
	greyImage = greyImageResult.value();

	// black
	const uint32_t black = packUnorm4x8(math::float4(0, 0, 0, 0));
	const std::optional<AllocatedImage> blackImageResult = CreateImage(&black, pixelSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!blackImageResult.has_value()) {
		SDL_Log("Couldn't create black image");
		return SDL_APP_FAILURE;
	}
	blackImage = blackImageResult.value();

	//checkerboard image
	uint32_t magenta = packUnorm4x8(math::float4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16> pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y * 16 + x] = x % 2 ^ y % 2 ? magenta : black;
		}
	}
	std::optional<AllocatedImage> errorCheckerboardImageResult = CreateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!errorCheckerboardImageResult.has_value()) {
		SDL_Log("Couldn't create error checkerboard image");
		return SDL_APP_FAILURE;
	}
	errorCheckerboardImage = errorCheckerboardImageResult.value();

	VkSamplerCreateInfo sampler = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,};

	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(device, &sampler, nullptr, &defaultSamplerNearest);

	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(device, &sampler, nullptr, &defaultSamplerLinear);
	mainDeletionQueue.PushFunction([&] {
		vkDestroySampler(device, defaultSamplerNearest, nullptr);
		vkDestroySampler(device, defaultSamplerLinear, nullptr);

		DestroyImage(whiteImage);
		DestroyImage(greyImage);
		DestroyImage(blackImage);
		DestroyImage(errorCheckerboardImage);
	});

	return SDL_APP_CONTINUE;
}

std::optional<AllocatedBuffer> VulkanEngine::CreateBuffer(const size_t allocSize, const VkBufferUsageFlags bufferUsage, const VmaMemoryUsage memoryUsage) const {
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = allocSize,
		.usage = bufferUsage,
	};

	const VmaAllocationCreateInfo vmaAllocInfo = {
		.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = memoryUsage,
	};

	AllocatedBuffer newBuffer{};
	VK_CHECK_EMPTY_OPTIONAL(vmaCreateBuffer(vmaAllocator, &bufferInfo, &vmaAllocInfo, &newBuffer.internalBuffer, &newBuffer.allocation, &newBuffer.allocationInfo), "Failed to create buffer");

	return newBuffer;
}

void VulkanEngine::DestroyBuffer(const AllocatedBuffer& buffer) const {
	vmaDestroyBuffer(vmaAllocator, buffer.internalBuffer, buffer.allocation);
}

std::optional<GPUMeshBuffers> VulkanEngine::UploadMesh(std::span<Uint16> indices, std::span<MyVertex> vertices) const {
	const size_t vertexBufferSize = vertices.size() * sizeof(MyVertex);
	const size_t indexBufferSize = indices.size() * sizeof(Uint16);

	//create vertex buffer
	std::optional<AllocatedBuffer> vertexBufferResult = CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	if (!vertexBufferResult.has_value()) {
		SDL_Log("Failed to create vertex buffer");
		return std::nullopt;
	}
	AllocatedBuffer vertexBuffer = vertexBufferResult.value();

	std::optional<AllocatedBuffer> indexBufferResult = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	if (!indexBufferResult.has_value()) {
		SDL_Log("Failed to create index buffer");
		return std::nullopt;
	}
	AllocatedBuffer indexBuffer = indexBufferResult.value();

	const VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vertexBuffer.internalBuffer,
	};

	GPUMeshBuffers newSurface{
		.vertexBuffer = vertexBuffer,
		.indexBuffer = indexBuffer,
		//find the address of the vertex buffer
		.vertexBufferAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo),
	};

	std::optional<AllocatedBuffer> stagingResult = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	if (!stagingResult.has_value()) {
		SDL_Log("Failed to create staging buffer");
		return std::nullopt;
	}
	AllocatedBuffer stagingBuffer = stagingResult.value();

	void* data = stagingBuffer.allocation->GetMappedData();

	memcpy(data, vertices.data(), vertexBufferSize); // copy vertex buffer
	memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize); // copy index buffer

	ImmediateSubmit([&](const VkCommandBuffer& commandBuffer) {
		const VkBufferCopy vertexCopy{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = vertexBufferSize
		};
		vkCmdCopyBuffer(commandBuffer, stagingBuffer.internalBuffer, newSurface.vertexBuffer.internalBuffer, 1, &vertexCopy);

		const VkBufferCopy indexCopy{
			.srcOffset = vertexBufferSize,
			.dstOffset = 0,
			.size = indexBufferSize
		};
		vkCmdCopyBuffer(commandBuffer, stagingBuffer.internalBuffer, newSurface.indexBuffer.internalBuffer, 1, &indexCopy);
	});

	DestroyBuffer(stagingBuffer);

	return newSurface;
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
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
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

	if (const SDL_AppResult res = InitImgui(); res != SDL_APP_CONTINUE) {
		return res;
	}

	if (const SDL_AppResult res = InitDefaultData(); res != SDL_APP_CONTINUE) {
		return res;
	}

	return SDL_APP_CONTINUE;
}

void VulkanEngine::DrawBackground(const VkCommandBuffer& commandBuffer) const {
	const ComputeEffect& currentEffect = backgroundEffects[currentBackgroundEffectIndex];

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, currentEffect.pipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, currentEffect.layout, 0, 1, &drawImageDescriptors, 0, nullptr);

	vkCmdPushConstants(commandBuffer, currentEffect.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &currentEffect.data);

	vkCmdDispatch(commandBuffer, std::ceil(drawExtent.width / 16.0), std::ceil(drawExtent.height / 16.0), 1);
}

void VulkanEngine::DrawImGui(const VkCommandBuffer& commandBuffer, const VkImageView& targetImageView) const {
	const VkRenderingAttachmentInfo colourAttachment = vk_init::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	const VkRenderingInfo renderingInfo = vk_init::RenderingInfo(swapchainExtent, &colourAttachment, nullptr);

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRendering(commandBuffer);
}

SDL_AppResult VulkanEngine::DrawGeometry(const VkCommandBuffer& commandBuffer) {
	//begin a render pass  connected to our draw image
	const VkRenderingAttachmentInfo colorAttachment = vk_init::AttachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	const VkRenderingAttachmentInfo depthAttachment = vk_init::DepthAttachmentInfo(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	const VkRenderingInfo renderInfo = vk_init::RenderingInfo(drawExtent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(commandBuffer, &renderInfo);

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

	if (ImGui::Begin("Camera")) {
		ImGui::SliderFloat("Camera Radius", &cameraRadius, -20.0f, 20.0f);
		ImGui::SliderFloat("Camera Height", &cameraHeight, -20.0f, 20.0f);
		ImGui::SliderFloat("Camera Rotation Speed", &cameraRotationSpeed, 0.0f, 0.002f);
		ImGui::SliderFloat("Camera FOV", &cameraFOV, 0.0f, 180.0f);
		ImGui::End();
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

	//bind a texture
	std::optional<VkDescriptorSet> imageSetResult = GetCurrentFrame().frameDescriptors.Allocate(device, singleImageDescriptorLayout);
	if (!imageSetResult.has_value()) {
		SDL_Log("Couldn't allocate descriptor set for image");
		return SDL_APP_FAILURE;
	}
	VkDescriptorSet imageSet = imageSetResult.value();
	{
		DescriptorWriter writer;
		writer.WriteImage(0, errorCheckerboardImage.imageView, defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.UpdateSet(device, imageSet);
	}

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, 0, 1, &imageSet, 0, nullptr);

	// > View Matrix
	float camX = sinf(static_cast<float>(SDL_GetTicks()) * cameraRotationSpeed) * cameraRadius;
	float camZ = cosf(static_cast<float>(SDL_GetTicks()) * cameraRotationSpeed) * cameraRadius;
	math::float3 cameraPos = math::float3(camX, -cameraHeight, camZ);
	math::float3 cameraTarget = math::float3(0.0f, 0.0f, 0.0f);
	math::float3 up = math::float3(0.0f, 1.0f, 0.0f);
	math::float4x4 view = inverse(look_at(cameraPos, cameraTarget, up));

	// > Projection Matrix
	math::int2 screenSize;
	SDL_GetWindowSize(window, &screenSize.x, &screenSize.y);
	math::float4x4 projection = perspective(math::degrees(cameraFOV).radians(),
	                                        static_cast<float>(screenSize.x) / static_cast<float>(screenSize.y),
	                                        0.1f, 1000.0f);

	// invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
	projection[1][1] *= -1;

	const GPUDrawPushConstants pushConstants{
		.worldMatrix = view * projection,
		.vertexBufferAddress = meshes[0]->meshBuffers.vertexBufferAddress,
	};

	vkCmdPushConstants(commandBuffer, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);
	vkCmdBindIndexBuffer(commandBuffer, meshes[0]->meshBuffers.indexBuffer.internalBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(commandBuffer, meshes[0]->surfaces[0].count, 1, meshes[0]->surfaces[0].startIndex, 0, 0);


	vkCmdEndRendering(commandBuffer);

	return SDL_APP_CONTINUE;
}

std::optional<AllocatedImage> VulkanEngine::CreateImage(const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped) const {
	AllocatedImage newImage{
		.imageExtent = size,
		.imageFormat = format,
	};

	VkImageCreateInfo imageCreateInfo = vk_init::ImageCreateInfo(format, usage, size);
	if (mipmapped) {
		imageCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	constexpr VmaAllocationCreateInfo allocationCreateInfo{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags{VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
	};
	// allocate and create the image
	VK_CHECK_EMPTY_OPTIONAL(vmaCreateImage(vmaAllocator, &imageCreateInfo, &allocationCreateInfo, &newImage.image, &newImage.allocation, nullptr), "Failed to create image");

	// if the format is a depth format, we will need to have it use the correct aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build an image-view for the image
	VkImageViewCreateInfo viewCreateInfo = vk_init::ImageViewCreateInfo(format, newImage.image, aspectFlag);
	viewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;

	VK_CHECK_EMPTY_OPTIONAL(vkCreateImageView(device, &viewCreateInfo, nullptr, &newImage.imageView), "Couldn't create image view");

	return newImage;
}

std::optional<AllocatedImage> VulkanEngine::CreateImage(const void* data, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const bool mipmapped) const {
	const size_t dataSize = size.depth * size.width * size.height * 4;
	const std::optional<AllocatedBuffer> uploadBufferResult = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	if (!uploadBufferResult.has_value()) {
		SDL_Log("Couldn't create upload buffer for image");
		return std::nullopt;
	}
	const AllocatedBuffer uploadBuffer = uploadBufferResult.value();

	memcpy(uploadBuffer.allocationInfo.pMappedData, data, dataSize);

	const std::optional<AllocatedImage> newImageResult = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);
	if (!newImageResult.has_value()) {
		SDL_Log("Couldn't create image");
		return std::nullopt;
	}
	AllocatedImage new_image = newImageResult.value();

	ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
		vk_util::TransitionImage(commandBuffer, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		const VkBufferImageCopy copyRegion = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = VkImageSubresourceLayers{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageExtent = size,
		};

		// copy the buffer into the image
		vkCmdCopyBufferToImage(commandBuffer, uploadBuffer.internalBuffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		vk_util::TransitionImage(commandBuffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	DestroyBuffer(uploadBuffer);

	return new_image;
}

void VulkanEngine::DestroyImage(const AllocatedImage& allocatedImage) const {
	vkDestroyImageView(device, allocatedImage.imageView, nullptr);
	vmaDestroyImage(vmaAllocator, allocatedImage.image, allocatedImage.allocation);
}

SDL_AppResult VulkanEngine::Draw() {
	if (resizeRequested) {
		if (const SDL_AppResult res = ResizeSwapchain(); res != SDL_APP_CONTINUE) {
			return res;
		}
	}

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	if (ImGui::Begin("Background")) {
		ImGui::SliderFloat("Render Scale", &renderScale, 0.3f, 1.0f);

		const ComputeEffect& currentEffect = backgroundEffects[currentBackgroundEffectIndex];

		ImGui::Text("Selected effect: ", currentEffect.name);
		ImGui::SliderInt("Effect Index", &currentBackgroundEffectIndex, 0, static_cast<int>(backgroundEffects.size() - 1));

		ImGui::InputFloat4("data1", const_cast<float*>(&currentEffect.data.data1.x));
		ImGui::InputFloat4("data2", const_cast<float*>(&currentEffect.data.data2.x));
		ImGui::InputFloat4("data3", const_cast<float*>(&currentEffect.data.data3.x));
		ImGui::InputFloat4("data4", const_cast<float*>(&currentEffect.data.data4.x));
		ImGui::End();
	}

	VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, secondInNanoseconds), "Couldn't wait for fence");

	GetCurrentFrame().frameDeletionQueue.Flush();
	GetCurrentFrame().frameDescriptors.ClearPools(device);

	uint32_t swapchainImageIndex;
	if (const VkResult err = vkAcquireNextImageKHR(device, swapchain, secondInNanoseconds, GetCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
		err == VK_ERROR_OUT_OF_DATE_KHR) {
		resizeRequested = true;
		ImGui::EndFrame();
		return SDL_APP_CONTINUE;
	} else if (err != VK_SUCCESS) {
		SDL_Log("Detected Vulkan error: %s: %s", "Couldn't acquire next image", string_VkResult(err));
		SDL_TriggerBreakpoint();
		return SDL_APP_FAILURE;
	}

	drawExtent.height = std::min(swapchainExtent.height, drawImage.imageExtent.height) * renderScale;
	drawExtent.width = std::min(swapchainExtent.width, drawImage.imageExtent.width) * renderScale;

	VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence), "Couldn't reset fence");

	const VkCommandBuffer& commandBuffer = GetCurrentFrame().mainCommandBuffer;

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0), "Couldn't reset command buffer");

	const VkCommandBufferBeginInfo commandBufferBeginInfo = vk_init::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo), "Couldn't begin command buffer");

	// transition our main draw image into general layout so we can write into it.
	// we will overwrite it all so we don't care about what was the older layout
	vk_util::TransitionImage(commandBuffer, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(commandBuffer);

	vk_util::TransitionImage(commandBuffer, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vk_util::TransitionImage(commandBuffer, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	DrawGeometry(commandBuffer);

	// transition the draw image and the swapchain image into their correct transfer layouts
	vk_util::TransitionImage(commandBuffer, drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vk_util::TransitionImage(commandBuffer, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vk_util::CopyImageToImage(commandBuffer, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

	// set swapchain image layout to Attachment Optimal so we can draw into it from imgui
	vk_util::TransitionImage(commandBuffer, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

	// draw imgui into the swapchain image
	ImGui::Render();
	DrawImGui(commandBuffer, swapchainImageViews[swapchainImageIndex]);

	// set swapchain image layout to Present so we can show it on the screen
	vk_util::TransitionImage(commandBuffer, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	if (const VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
		presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		resizeRequested = true;
		return SDL_APP_CONTINUE;
	} else if (presentResult != VK_SUCCESS) {
		SDL_Log("Detected Vulkan error: %s: %s", "Couldn't present image", string_VkResult(presentResult));
		SDL_TriggerBreakpoint();
		return SDL_APP_FAILURE;
	}

	frameNumber++;

	return SDL_APP_CONTINUE;
}

SDL_AppResult VulkanEngine::HandleEvent(const SDL_Event* event) {
	switch (event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS; // end the program, reporting success to the OS.
		case SDL_EVENT_KEY_DOWN:
			if (event->key.key != SDLK_ESCAPE && event->key.key != SDLK_Q) break;
			return SDL_APP_SUCCESS; // end the program, reporting success to the OS.
		case SDL_EVENT_WINDOW_RESIZED:
			if (const SDL_AppResult res = ResizeSwapchain(); res != SDL_APP_CONTINUE) {
				return res;
			}
			break;
		default:
			break;
	}

	ImGui_ImplSDL3_ProcessEvent(event);

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

		for (const std::shared_ptr mesh : meshes) {
			DestroyBuffer(mesh->meshBuffers.indexBuffer);
			DestroyBuffer(mesh->meshBuffers.vertexBuffer);
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
