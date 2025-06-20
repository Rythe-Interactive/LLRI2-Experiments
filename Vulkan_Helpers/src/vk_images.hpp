#pragma once

#include "mass_includer.hpp"

namespace vk_util {
	[[nodiscard]] VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);
	void TransitionImage(const VkCommandBuffer& commandBuffer, const VkImage& image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void CopyImageToImage(const VkCommandBuffer& commandBuffer, const VkImage& source, const VkImage& destination, VkExtent2D srcSize, VkExtent2D dstSize, VkFilter filter = VK_FILTER_LINEAR);
}
