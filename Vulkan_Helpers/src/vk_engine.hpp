#pragma once

// C++
#include <filesystem>
#include <string>
#include <vector>

// SDL3
#include <SDL3/SDL.h>

// Vulkan Helper Libraries
// ReSharper disable once CppUnusedIncludeDirective
#include <volk.h>
#include <vk_mem_alloc.h>

// Engine
#include "vk_custom_types.hpp"
#include "vk_descriptors.hpp"

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

	struct FrameData {
		VkCommandPool commandPool = nullptr;
		VkCommandBuffer mainCommandBuffer = nullptr;

		VkSemaphore swapchainSemaphore = nullptr;
		VkFence renderFence = nullptr;

		DeletionQueue frameDeletionQueue;
	};

	static constexpr unsigned int FRAME_OVERLAP = 2;

	unsigned int frameNumber = 0;
	FrameData frames[FRAME_OVERLAP];
	FrameData& GetCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; }
	VkQueue graphicsQueue = nullptr;
	uint32_t graphicsQueueFamilyIndex = 0;

	DeletionQueue mainDeletionQueue;

	VmaAllocator vmaAllocator = nullptr;

	//Draw Resources
	AllocatedImage drawImage = {};
	VkExtent2D drawExtent = {};

	DescriptorAllocator globalDescriptorAllocator = {};

	VkDescriptorSet drawImageDescriptors = nullptr;
	VkDescriptorSetLayout drawImageDescriptorLayout = nullptr;

	VkPipeline gradientPipeline = nullptr;
	VkPipelineLayout gradientPipelineLayout = nullptr;

	VkPipeline trianglePipeline = nullptr;
	VkPipelineLayout trianglePipelineLayout = nullptr;

private:
	SDL_AppResult InitVulkan();
	SDL_AppResult InitCommands();
	SDL_AppResult InitSyncStructures();

private:
	SDL_AppResult CreateSwapchain(uint32_t width, uint32_t height);
	SDL_AppResult InitSwapchain();
	void DestroySwapchain() const;

private:
	SDL_AppResult InitDescriptors();

private:
	SDL_AppResult InitPipelines();
	SDL_AppResult InitBackgroundPipelines();
	SDL_AppResult InitTrianglePipelines();

private:
	void DrawBackground(const VkCommandBuffer& commandBuffer, const VkImage& image) const;
	void DrawGeometry(const VkCommandBuffer& commandBuffer) const;

public:
	VulkanEngine(std::string name, bool debugMode);

	[[nodiscard]] std::filesystem::path GetAssetsDir() const;

	SDL_AppResult Init(int width, int height);
	SDL_AppResult Draw();
	void Cleanup(SDL_AppResult result);
};
