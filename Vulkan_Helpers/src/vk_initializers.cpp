#include "vk_initializers.hpp"

VkCommandPoolCreateInfo vk_init::CommandPoolCreateInfo(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags) {
	return VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = flags,
		.queueFamilyIndex = queueFamilyIndex,
	};
}

VkCommandBufferAllocateInfo vk_init::CommandBufferAllocateInfo(const VkCommandPool& commandPool, const uint32_t count) {
	return VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = count,
	};
}

VkCommandBufferBeginInfo vk_init::CommandBufferBeginInfo(const VkCommandBufferUsageFlags flags) {
	return VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = flags,
		.pInheritanceInfo = nullptr,
	};
}

VkCommandBufferSubmitInfo vk_init::CommandBufferSubmitInfo(const VkCommandBuffer& commandBuffer) {
	return VkCommandBufferSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = commandBuffer,
		.deviceMask = 0,
	};
}

VkFenceCreateInfo vk_init::FenceCreateInfo(const VkFenceCreateFlags flags) {
	return VkFenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = flags,
	};
}

VkSemaphoreCreateInfo vk_init::SemaphoreCreateInfo(const VkSemaphoreCreateFlags flags) {
	return VkSemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.flags = flags,
	};
}

VkSubmitInfo2 vk_init::SubmitInfo(const VkCommandBufferSubmitInfo* commandBuffer, const VkSemaphoreSubmitInfo* signalSemaphoreInfo, const VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
	return VkSubmitInfo2{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0u : 1u,
		.pWaitSemaphoreInfos = waitSemaphoreInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = commandBuffer,
		.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0u : 1u,
		.pSignalSemaphoreInfos = signalSemaphoreInfo
	};
}

VkPresentInfoKHR vk_init::PresentInfo() {
	return VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.swapchainCount = 0,
		.pSwapchains = nullptr,
		.pImageIndices = nullptr,
	};
}

VkRenderingAttachmentInfo vk_init::AttachmentInfo(const VkImageView& view, const VkClearValue* clear, const VkImageLayout layout) {
	return VkRenderingAttachmentInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = view,
		.imageLayout = layout,
		.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = clear ? *clear : VkClearValue{},
	};
}

VkRenderingAttachmentInfo vk_init::DepthAttachmentInfo(const VkImageView& view, const VkImageLayout layout) {
	return VkRenderingAttachmentInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = view,
		.imageLayout = layout,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = VkClearValue{.depthStencil = VkClearDepthStencilValue{.depth = 0.0f}}
	};
}

VkRenderingInfo vk_init::RenderingInfo(const VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colourAttachment, const VkRenderingAttachmentInfo* depthAttachment) {
	return VkRenderingInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = VkRect2D{.offset = VkOffset2D{.x = 0, .y = 0}, .extent = renderExtent},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = colourAttachment,
		.pDepthAttachment = depthAttachment,
		.pStencilAttachment = nullptr
	};
}

VkImageSubresourceRange vk_init::ImageSubresourceRange(const VkImageAspectFlags aspectMask) {
	return VkImageSubresourceRange{
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS
	};
}

VkSemaphoreSubmitInfo vk_init::SemaphoreSubmitInfo(const VkPipelineStageFlags2 stageMask, const VkSemaphore& semaphore) {
	return VkSemaphoreSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = semaphore,
		.value = 1,
		.stageMask = stageMask,
		.deviceIndex = 0,
	};
}

VkDescriptorSetLayoutBinding vk_init::DescriptorSetLayoutBinding(const VkDescriptorType type, const VkShaderStageFlags stageFlags, const uint32_t binding) {
	return VkDescriptorSetLayoutBinding{
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = 1,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}

VkDescriptorSetLayoutCreateInfo vk_init::DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding* bindings, const uint32_t bindingCount) {
	return VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = 0,
		.bindingCount = bindingCount,
		.pBindings = bindings
	};
}

VkWriteDescriptorSet vk_init::WriteDescriptorImage(const VkDescriptorType type, const VkDescriptorSet& dstSet, const VkDescriptorImageInfo* imageInfo, const uint32_t binding) {
	return VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = imageInfo,
	};
}

VkWriteDescriptorSet vk_init::WriteDescriptorBuffer(const VkDescriptorType type, const VkDescriptorSet& dstSet, const VkDescriptorBufferInfo* bufferInfo, const uint32_t binding) {
	return VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = bufferInfo,
	};
}

VkDescriptorBufferInfo vk_init::BufferInfo(const VkBuffer& buffer, const VkDeviceSize offset, const VkDeviceSize range) {
	return VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = offset,
		.range = range
	};
}

VkImageCreateInfo vk_init::ImageCreateInfo(const VkFormat format, const VkImageUsageFlags usageFlags, const VkExtent3D extent) {
	return VkImageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		//for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
		.samples = VK_SAMPLE_COUNT_1_BIT,
		//optimal tiling, which means the image is stored on the best gpu format
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usageFlags,
	};
}

VkImageViewCreateInfo vk_init::ImageViewCreateInfo(const VkFormat format, const VkImage& image, const VkImageAspectFlags aspectFlags) {
	// build an image-view for the depth image to use for rendering
	return VkImageViewCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};
}

VkPipelineLayoutCreateInfo vk_init::PipelineLayoutCreateInfo(const std::span<VkPushConstantRange> pushConstantRanges, const std::span<VkDescriptorSetLayout> setLayouts) {
	return VkPipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.flags = 0,
		.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
		.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data(),
		.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
		.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data(),
	};
}

VkPipelineLayoutCreateInfo vk_init::PipelineLayoutCreateInfo(const VkPushConstantRange* pushConstantRange, const VkDescriptorSetLayout* setLayout) {
	return VkPipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.flags = 0,
		.setLayoutCount = setLayout ? 1u : 0u,
		.pSetLayouts = setLayout ? setLayout : nullptr,
		.pushConstantRangeCount = pushConstantRange ? 1u : 0u,
		.pPushConstantRanges = pushConstantRange ? pushConstantRange : nullptr,
	};
}

VkPipelineShaderStageCreateInfo vk_init::PipelineShaderStageCreateInfo(const VkShaderStageFlagBits stage, const VkShaderModule& shaderModule, const char* entry) {
	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = stage,
		.module = shaderModule,
		.pName = entry,
	};
}
