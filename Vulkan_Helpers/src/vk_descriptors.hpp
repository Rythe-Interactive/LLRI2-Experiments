#pragma once

#include "mass_includer.hpp"

struct DescriptorLayoutBuilder {
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	void AddBinding(uint32_t binding, VkDescriptorType type);
	void Clear();
	[[nodiscard]] std::optional<VkDescriptorSetLayout> Build(const VkDevice& device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator {
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

	VkDescriptorPool pool;

	void InitPool(const VkDevice& device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void ClearPool(const VkDevice& device) const;
	void DestroyPool(const VkDevice& device) const;

	[[nodiscard]] std::optional<VkDescriptorSet> Allocate(const VkDevice& device, VkDescriptorSetLayout layout) const;
};

struct DescriptorAllocatorGrowable {
public:
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

	void InitPools(const VkDevice& device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
	void ClearPools(const VkDevice& device);
	void DestroyPools(const VkDevice& device);

	[[nodiscard]] std::optional<VkDescriptorSet> Allocate(const VkDevice& device, const VkDescriptorSetLayout& layout, const void* pNext = nullptr);

private:
	[[nodiscard]] VkDescriptorPool GetPool(const VkDevice& device);
	[[nodiscard]] static VkDescriptorPool CreatePool(const VkDevice& device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> fullPools;
	std::vector<VkDescriptorPool> readyPools;
	uint32_t setsPerPool = 0;
};

struct DescriptorWriter {
	std::deque<VkDescriptorImageInfo> imageInfos;
	std::deque<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkWriteDescriptorSet> writes;

	void WriteImage(uint32_t binding, const VkImageView& image, const VkSampler& sampler, VkImageLayout layout, VkDescriptorType type);
	void WriteBuffer(uint32_t binding, const VkBuffer& buffer, size_t size, size_t offset, VkDescriptorType type);

	void Clear();
	void UpdateSet(const VkDevice& device, const VkDescriptorSet& set);
};
