// SDL3
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

// Vulkan
#include <volk.h>
#include <VkBootstrap.h>

// C++
#include <string>

struct MyAppState {
	std::string name = "Hello, Vulkan with Helpers!";
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
};

///Returns true on success, false on failure
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

///Returns true on success, false on failure
bool InitVulkan(MyAppState* myAppState) {
	//TODO: Maybe try SDL_Vulkan_LoadLibrary instead of Volk?
	//Attempt to load Vulkan loader from the system
	if (const VkResult result = volkInitialize(); result != VK_SUCCESS) {
		SDL_Log("Couldn't initialize Volk: %d", result);
		return false;
	}

	//Make the Vulkan instance, with some debug features
	vkb::InstanceBuilder builder;
	vkb::Result<vkb::Instance> inst_ret = builder.set_app_name(myAppState->name.c_str())
	                                             .set_app_version(1, 0, 0)
	                                             .request_validation_layers(true)
	                                             .use_default_debug_messenger()
	                                             .require_api_version(1, 3, 0)
	                                             .build();

	if (!inst_ret.has_value()) {
		SDL_Log("Couldn't create Vulkan instance: %s", inst_ret.error().message().c_str());
		return false;
	}

	//Yoink the actual instance and debug messenger from the result
	const vkb::Instance vkb_inst = inst_ret.value();
	myAppState->instance = vkb_inst.instance;
	myAppState->debugMessenger = vkb_inst.debug_messenger;

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
	vkb::PhysicalDeviceSelector selector(vkb_inst);
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
	const vkb::PhysicalDevice& physicalDevice = physDev_ret.value();

	//Use VkBootstrap to create the final Vulkan Device
	vkb::DeviceBuilder deviceBuilder(physicalDevice);
	vkb::Result<vkb::Device> device_ret = deviceBuilder
		.build();

	if (!device_ret.has_value()) {
		SDL_Log("Couldn't create Vulkan device: %s", device_ret.error().message().c_str());
		return false;
	}

	//Yoink the device from the result
	myAppState->device = device_ret.value().device;
	myAppState->physicalDevice = physicalDevice.physical_device;

	return true;
}

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


	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, const SDL_AppResult result) {
	SDL_Log("Exiting with result %d", result);

	const MyAppState* myAppState = static_cast<MyAppState*>(appstate);

	if (result == SDL_APP_SUCCESS) {
		DestroySwapchain(myAppState);

		vkDestroySurfaceKHR(myAppState->instance, myAppState->surface, nullptr);

		vkDestroyDevice(myAppState->device, nullptr);
		vkb::destroy_debug_utils_messenger(myAppState->instance, myAppState->debugMessenger);
		vkDestroyInstance(myAppState->instance, nullptr);
	}


	delete myAppState;
}
