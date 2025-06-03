#pragma once

// C++
#include <span>

// Vulkan Helper Libraries
#include <volk.h>

namespace vk_init {
	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool& commandPool, uint32_t count = 1);

	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkCommandBufferSubmitInfo CommandBufferSubmitInfo(const VkCommandBuffer& commandBuffer);

	VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

	VkSubmitInfo2 SubmitInfo(const VkCommandBufferSubmitInfo* commandBuffer, const VkSemaphoreSubmitInfo* signalSemaphoreInfo, const VkSemaphoreSubmitInfo* waitSemaphoreInfo);
	VkPresentInfoKHR PresentInfo();

	VkRenderingAttachmentInfo AttachmentInfo(const VkImageView& view, const VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo DepthAttachmentInfo(const VkImageView& view, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colourAttachment, const VkRenderingAttachmentInfo* depthAttachment);

	VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

	VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, const VkSemaphore& semaphore);
	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
	VkWriteDescriptorSet WriteDescriptorImage(VkDescriptorType type, const VkDescriptorSet& dstSet, const VkDescriptorImageInfo* imageInfo, uint32_t binding);
	VkWriteDescriptorSet WriteDescriptorBuffer(VkDescriptorType type, const VkDescriptorSet& dstSet, const VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkDescriptorBufferInfo BufferInfo(const VkBuffer& buffer, VkDeviceSize offset, VkDeviceSize range);

	VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, const VkImage& image, VkImageAspectFlags aspectFlags);
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(std::span<VkPushConstantRange> pushConstantRanges = {}, std::span<VkDescriptorSetLayout> setLayouts = {});
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(const VkPushConstantRange* pushConstantRange = nullptr, const VkDescriptorSetLayout* setLayout = nullptr);
	VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, const VkShaderModule& shaderModule, const char* entry = "main");
};
