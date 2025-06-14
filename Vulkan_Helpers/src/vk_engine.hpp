#pragma once

#include "mass_includer.hpp"

// Engine
#include "vk_custom_types.hpp"
#include "vk_descriptors.hpp"
#include "vk_loader.hpp"

class VulkanEngine {
	const std::string name;
	const bool debugMode = false;
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
	std::vector<VkSemaphore> readyForPresentSemaphores;
	VkExtent2D swapchainExtent = {};

	static constexpr uint64_t secondInNanoseconds = 1'000'000'000;

	struct FrameData {
		VkCommandPool commandPool = nullptr;
		VkCommandBuffer mainCommandBuffer = nullptr;

		VkSemaphore swapchainSemaphore = nullptr;
		VkFence renderFence = nullptr;

		DeletionQueue frameDeletionQueue;
		DescriptorAllocatorGrowable frameDescriptors;
	};

	Uint32 frameNumber = 0;
	std::array<Uint32, FRAME_NUMBERS> frameTimes = {};

	std::array<FrameData, 2> frames = {};
	FrameData& GetCurrentFrame() { return frames[frameNumber % frames.size()]; }
	VkQueue graphicsQueue = nullptr;
	uint32_t graphicsQueueFamilyIndex = 0;

	DeletionQueue mainDeletionQueue;

	VmaAllocator vmaAllocator = nullptr;

	//Draw Resources
	AllocatedImage drawImage = {};
	AllocatedImage depthImage = {};
	VkExtent2D drawExtent = {};
	float renderScale = 1.0f;
	VkFilter renderScaleFilter = VK_FILTER_LINEAR;

	bool resizeRequested = false;

	DescriptorAllocator globalDescriptorAllocator = {};

	VkPipeline meshPipeline = nullptr;
	VkPipelineLayout meshPipelineLayout = nullptr;

	std::vector<std::shared_ptr<MeshAsset>> meshes;
	static constexpr int selectedMeshIndex = 0;

	//Immediate Submit
	VkFence immediateSubmitFence = nullptr;
	VkCommandBuffer immediateSubmitCommandBuffer = nullptr;
	VkCommandPool immediateSubmitCommandPool = nullptr;

	AllocatedImage imageTexture = {};

	VkSampler defaultSamplerLinear = nullptr;

	VkDescriptorSetLayout singleImageDescriptorLayout = nullptr;

private:
	[[nodiscard]] SDL_AppResult InitVulkan();
	[[nodiscard]] SDL_AppResult InitCommands();
	[[nodiscard]] SDL_AppResult InitSyncStructures();

private:
	[[nodiscard]] SDL_AppResult CreateSwapchain(uint32_t width, uint32_t height);
	[[nodiscard]] SDL_AppResult InitSwapchain();
	void DestroySwapchain() const;
	[[nodiscard]] SDL_AppResult ResizeSwapchain();

private:
	[[nodiscard]] SDL_AppResult InitDescriptors();

private:
	[[nodiscard]] SDL_AppResult InitPipelines();
	[[nodiscard]] SDL_AppResult InitMeshPipeline();

private:
	[[nodiscard]] SDL_AppResult InitDefaultData();

private:
	[[nodiscard]] std::optional<AllocatedBuffer> CreateBuffer(size_t allocSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) const;
	void DestroyBuffer(const AllocatedBuffer& buffer) const;

private:
	void DrawBackground(const VkCommandBuffer& commandBuffer) const;
	[[nodiscard]] SDL_AppResult DrawGeometry(const VkCommandBuffer& commandBuffer);

private:
	[[nodiscard]] std::optional<AllocatedImage> CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) const;
	[[nodiscard]] std::optional<AllocatedImage> CreateImage(const void* data, VkExtent3D imageSize, size_t pixelSize, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;
	void DestroyImage(const AllocatedImage& allocatedImage) const;

public:
	VulkanEngine(std::string name, bool debugMode);

	[[nodiscard]] std::filesystem::path GetAssetsDir() const;
	/// @param imagePath Path to image file, relative from the directory where the application was run from.
	/// @param desiredChannels Colour channels of the image to load.
	[[nodiscard]] SDL_Surface* LoadImage(const std::filesystem::path& imagePath, int desiredChannels) const;
	[[nodiscard]] std::optional<GPUMeshBuffers> UploadMesh(std::span<Uint16> indices, std::span<MyVertex> vertices) const;

	[[nodiscard]] SDL_AppResult Init(int width, int height);
	[[nodiscard]] SDL_AppResult Draw();
	[[nodiscard]] SDL_AppResult HandleEvent(const SDL_Event* event);
	void Cleanup(SDL_AppResult result);

public:
	[[nodiscard]] SDL_AppResult ImmediateSubmit(std::function<void(VkCommandBuffer commandBuffer)>&& function) const;
};
