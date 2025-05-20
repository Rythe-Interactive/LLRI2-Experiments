#include "vk_initializers.hpp"

VkCommandPoolCreateInfo vk_init::CommandPoolCreateInfo(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags) {
	return VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = flags,
		.queueFamilyIndex = queueFamilyIndex,
	};
}

VkCommandBufferAllocateInfo vk_init::CommandBufferAllocateInfo(const VkCommandPool commandPool, const uint32_t count) {
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

VkSemaphoreSubmitInfo vk_init::SemaphoreSubmitInfo(const VkPipelineStageFlags2 stageMask, const VkSemaphore semaphore) {
	return VkSemaphoreSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = semaphore,
		.value = 1,
		.stageMask = stageMask,
		.deviceIndex = 0,
	};
}

VkCommandBufferSubmitInfo vk_init::CommandBufferSubmitInfo(const VkCommandBuffer commandBuffer) {
	return VkCommandBufferSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = commandBuffer,
		.deviceMask = 0,
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
