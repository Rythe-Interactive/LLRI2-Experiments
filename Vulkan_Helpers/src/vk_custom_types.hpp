#pragma once

// C++
#include <deque>
#include <functional>
#include <ranges>

// Vulkan Helper Libraries
#include <vk_mem_alloc.h>

// RSL
#include <rsl/math>

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

struct AllocatedBuffer {
	VkBuffer internalBuffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
};

struct MyVertex {
	math::float3 pos;
	math::float1 uvX; //spread out to keep the struct small
	math::float3 normal;
	math::float1 uvY; //spread out to keep the struct small
	math::float4 colour;
};

struct GPUMeshBuffers {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
	math::float4x4 worldMatrix;
	VkDeviceAddress vertexBufferAddress;
};
