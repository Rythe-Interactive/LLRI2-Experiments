#pragma once

#include "mass_includer.hpp"

struct DescriptorLayoutBuilder {
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	void AddBinding(uint32_t binding, VkDescriptorType type);
	void Clear();
	std::optional<VkDescriptorSetLayout> Build(const VkDevice& device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator {
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

	VkDescriptorPool pool;

	void InitPool(const VkDevice& device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void ClearDescriptors(const VkDevice& device) const;
	void DestroyPool(const VkDevice& device) const;

	std::optional<VkDescriptorSet> Allocate(const VkDevice& device, VkDescriptorSetLayout layout) const;
};
