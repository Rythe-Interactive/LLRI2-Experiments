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

	unsigned int frameNumber = 0;
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

	bool resizeRequested = false;

	DescriptorAllocator globalDescriptorAllocator = {};

	VkDescriptorSet drawImageDescriptors = nullptr;
	VkDescriptorSetLayout drawImageDescriptorLayout = nullptr;

	VkPipeline meshPipeline = nullptr;
	VkPipelineLayout meshPipelineLayout = nullptr;

	std::vector<std::shared_ptr<MeshAsset>> meshes;

	//Immediate Submit
	VkFence immediateSubmitFence = nullptr;
	VkCommandBuffer immediateSubmitCommandBuffer = nullptr;
	VkCommandPool immediateSubmitCommandPool = nullptr;

	//Push Constants for the Compute Background
	struct ComputePushConstants {
		math::float4 data1;
		math::float4 data2;
		math::float4 data3;
		math::float4 data4;
	};

	struct ComputeEffect {
		const char* name{};
		VkPipeline pipeline{};
		VkPipelineLayout layout{};
		ComputePushConstants data;
	};

	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffectIndex = 0;

	float cameraRadius = 10.0f;
	float cameraHeight = 3.0f;
	float cameraRotationSpeed = 0.001f;
	float cameraFOV = 45.0f;

	struct GPUSceneData {
		math::float4x4 view;
		math::float4x4 proj;
		math::float4x4 viewProj;
		math::float4 ambientColour;
		math::float4 sunlightDirection; //w is the sun's power
		math::float4 sunlightColour;
	};

	GPUSceneData sceneData;
	VkDescriptorSetLayout gpuSceneDataDescriptorLayout = nullptr;

	AllocatedImage whiteImage = {};
	AllocatedImage blackImage = {};
	AllocatedImage greyImage = {};
	AllocatedImage errorCheckerboardImage = {};

	VkSampler defaultSamplerLinear = nullptr;
	VkSampler defaultSamplerNearest = nullptr;

	VkDescriptorSetLayout singleImageDescriptorLayout = nullptr;

private:
	SDL_AppResult InitVulkan();
	SDL_AppResult InitCommands();
	SDL_AppResult InitSyncStructures();

private:
	SDL_AppResult CreateSwapchain(uint32_t width, uint32_t height);
	SDL_AppResult InitSwapchain();
	void DestroySwapchain() const;
	SDL_AppResult ResizeSwapchain();

private:
	SDL_AppResult InitDescriptors();

private:
	SDL_AppResult InitPipelines();
	SDL_AppResult InitBackgroundPipelines();
	SDL_AppResult InitMeshPipeline();

private:
	SDL_AppResult InitImgui();

private:
	SDL_AppResult InitDefaultData();

private:
	[[nodiscard]] std::optional<AllocatedBuffer> CreateBuffer(size_t allocSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) const;
	void DestroyBuffer(const AllocatedBuffer& buffer) const;

private:
	void DrawBackground(const VkCommandBuffer& commandBuffer) const;
	void DrawImGui(const VkCommandBuffer& commandBuffer, const VkImageView& targetImageView) const;
	SDL_AppResult DrawGeometry(const VkCommandBuffer& commandBuffer);

private:
	std::optional<AllocatedImage> CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) const;
	std::optional<AllocatedImage> CreateImage(const void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) const;
	void DestroyImage(const AllocatedImage& allocatedImage) const;

public:
	VulkanEngine(std::string name, bool debugMode);

	[[nodiscard]] std::filesystem::path GetAssetsDir() const;
	[[nodiscard]] std::optional<GPUMeshBuffers> UploadMesh(std::span<Uint16> indices, std::span<MyVertex> vertices) const;

	SDL_AppResult Init(int width, int height);
	SDL_AppResult Draw();
	SDL_AppResult HandleEvent(const SDL_Event* event);
	void Cleanup(SDL_AppResult result);

public:
	SDL_AppResult ImmediateSubmit(std::function<void(VkCommandBuffer commandBuffer)>&& function) const;
};
