#include "vk_images.hpp"

VkImageSubresourceRange vk_util::ImageSubresourceRange(const VkImageAspectFlags aspectMask) {
	return VkImageSubresourceRange{
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS,
	};
}

void vk_util::TransitionImage(const VkCommandBuffer& commandBuffer, const VkImage& image, const VkImageLayout currentLayout, const VkImageLayout newLayout) {
	const VkImageAspectFlags aspectMask = newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		                                      ? VK_IMAGE_ASPECT_DEPTH_BIT
		                                      : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageMemoryBarrier2 imageBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
		.oldLayout = currentLayout,
		.newLayout = newLayout,
		.image = image,
		.subresourceRange = ImageSubresourceRange(aspectMask),
	};

	const VkDependencyInfo depInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier,
	};

	vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void vk_util::CopyImageToImage(const VkCommandBuffer commandBuffer, const VkImage source, const VkImage destination, const VkExtent2D srcSize, const VkExtent2D dstSize) {
	VkImageBlit2 blitRegion{
		.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
		.srcSubresource = VkImageSubresourceLayers{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.dstSubresource = VkImageSubresourceLayers{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	const VkBlitImageInfo2 blitInfo{
		.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
		.srcImage = source,
		.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.dstImage = destination,
		.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.regionCount = 1,
		.pRegions = &blitRegion,
		.filter = VK_FILTER_LINEAR,
	};

	vkCmdBlitImage2(commandBuffer, &blitInfo);
}
