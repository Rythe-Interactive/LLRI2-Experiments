#pragma once

#include "mass_includer.hpp"

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
	math::float4 pos;
	math::float2 uv;
};

struct GPUMeshBuffers {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
	math::float4x4 modelMatrix;
	math::float4x4 viewMatrix;
	math::float4x4 projectionMatrix;
	VkDeviceAddress vertexBufferAddress;
};
