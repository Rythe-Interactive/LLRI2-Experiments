#pragma once

#include <volk.h>

namespace vk_util {
	VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);
	void TransitionImage(const VkCommandBuffer& commandBuffer, const VkImage& image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void CopyImageToImage(const VkCommandBuffer& commandBuffer, const VkImage& source, const VkImage& destination, VkExtent2D srcSize, VkExtent2D dstSize);
}
