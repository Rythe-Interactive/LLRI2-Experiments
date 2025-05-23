#pragma once

// C++
#include <deque>
#include <functional>
#include <ranges>

// Vulkan Helper Libraries
#include <vk_mem_alloc.h>

struct DeletionQueue {
	std::deque<std::function<void()>> deleters;

	void PushFunction(std::function<void()>&& function) {
		deleters.push_back(function);
	}

	void Flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto& deleter : std::ranges::reverse_view(deleters)) {
			deleter(); //call functors
		}

		deleters.clear();
	}
};

struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
};
