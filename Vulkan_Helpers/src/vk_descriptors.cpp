#include "vk_descriptors.hpp"

#include "vk_macros.hpp"

void DescriptorLayoutBuilder::AddBinding(const uint32_t binding, const VkDescriptorType type) {
	const VkDescriptorSetLayoutBinding newBinding{
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = 1,
	};

	bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::Clear() {
	bindings.clear();
}

std::optional<VkDescriptorSetLayout> DescriptorLayoutBuilder::Build(const VkDevice& device, const VkShaderStageFlags shaderStages, void* pNext, const VkDescriptorSetLayoutCreateFlags flags) {
	for (VkDescriptorSetLayoutBinding& b : bindings) {
		b.stageFlags |= shaderStages;
	}

	const VkDescriptorSetLayoutCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = pNext,
		.flags = flags,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data(),
	};

	VkDescriptorSetLayout set;
	VK_CHECK_EMPTY_OPTIONAL(vkCreateDescriptorSetLayout(device, &info, nullptr, &set), "Couldn't create descriptor set layout");

	return set;
}

void DescriptorAllocator::InitPool(const VkDevice& device, const uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (const auto [type, ratio] : poolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = type,
			.descriptorCount = static_cast<uint32_t>(ratio * static_cast<float>(maxSets))
		});
	}

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = maxSets,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void DescriptorAllocator::ClearDescriptors(const VkDevice& device) const {
	vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::DestroyPool(const VkDevice& device) const {
	vkDestroyDescriptorPool(device, pool, nullptr);
}

std::optional<VkDescriptorSet> DescriptorAllocator::Allocate(const VkDevice& device, VkDescriptorSetLayout layout) const {
	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout,
	};

	VkDescriptorSet ds;
	VK_CHECK_EMPTY_OPTIONAL(vkAllocateDescriptorSets(device, &allocInfo, &ds), "Couldn't allocate descriptor set");

	return ds;
}
