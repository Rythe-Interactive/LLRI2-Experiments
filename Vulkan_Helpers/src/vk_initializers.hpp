#pragma once

#include <volk.h>

namespace vk_init {
	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t count = 1);
	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);
	VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
	VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
	VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer commandBuffer);
	VkSubmitInfo2 SubmitInfo(const VkCommandBufferSubmitInfo* commandBuffer, const VkSemaphoreSubmitInfo* signalSemaphoreInfo, const VkSemaphoreSubmitInfo* waitSemaphoreInfo);
	VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
};
