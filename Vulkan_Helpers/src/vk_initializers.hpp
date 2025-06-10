#pragma once

#include "mass_includer.hpp"

namespace vk_init {
	[[nodiscard]] VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	[[nodiscard]] VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool& commandPool, uint32_t count = 1);

	[[nodiscard]] VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	[[nodiscard]] VkCommandBufferSubmitInfo CommandBufferSubmitInfo(const VkCommandBuffer& commandBuffer);

	[[nodiscard]] VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

	[[nodiscard]] VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	[[nodiscard]] VkSubmitInfo2 SubmitInfo(const VkCommandBufferSubmitInfo* commandBuffer, const VkSemaphoreSubmitInfo* signalSemaphoreInfo, const VkSemaphoreSubmitInfo* waitSemaphoreInfo);
	[[nodiscard]] VkPresentInfoKHR PresentInfo();

	[[nodiscard]] VkRenderingAttachmentInfo AttachmentInfo(const VkImageView& view, const VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	[[nodiscard]] VkRenderingAttachmentInfo DepthAttachmentInfo(const VkImageView& view, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	[[nodiscard]] VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colourAttachment, const VkRenderingAttachmentInfo* depthAttachment);

	[[nodiscard]] VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

	[[nodiscard]] VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, const VkSemaphore& semaphore);
	[[nodiscard]] VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	[[nodiscard]] VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
	[[nodiscard]] VkWriteDescriptorSet WriteDescriptorImage(VkDescriptorType type, const VkDescriptorSet& dstSet, const VkDescriptorImageInfo* imageInfo, uint32_t binding);
	[[nodiscard]] VkWriteDescriptorSet WriteDescriptorBuffer(VkDescriptorType type, const VkDescriptorSet& dstSet, const VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	[[nodiscard]] VkDescriptorBufferInfo BufferInfo(const VkBuffer& buffer, VkDeviceSize offset, VkDeviceSize range);

	[[nodiscard]] VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	[[nodiscard]] VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, const VkImage& image, VkImageAspectFlags aspectFlags);
	[[nodiscard]] VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(std::span<VkPushConstantRange> pushConstantRanges = {}, std::span<VkDescriptorSetLayout> setLayouts = {});
	[[nodiscard]] VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(const VkPushConstantRange* pushConstantRange = nullptr, const VkDescriptorSetLayout* setLayout = nullptr);
	[[nodiscard]] VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, const VkShaderModule& shaderModule, const char* entry = "main");
};
