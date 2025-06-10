// Impl
#include "vk_descriptors.hpp"

// Engine
#include "vk_macros.hpp"

void DescriptorLayoutBuilder::AddBinding(const uint32_t binding, const VkDescriptorType type) {
	const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = 1,
	};

	bindings.push_back(descriptorSetLayoutBinding);
}

void DescriptorLayoutBuilder::Clear() {
	bindings.clear();
}

std::optional<VkDescriptorSetLayout> DescriptorLayoutBuilder::Build(const VkDevice& device, const VkShaderStageFlags shaderStages, void* pNext, const VkDescriptorSetLayoutCreateFlags flags) {
	for (VkDescriptorSetLayoutBinding& b : bindings) {
		b.stageFlags |= shaderStages;
	}

	const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = pNext,
		.flags = flags,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data(),
	};

	VkDescriptorSetLayout set;
	VK_CHECK_EMPTY_OPTIONAL(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &set), "Couldn't create descriptor set layout");

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

	const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = maxSets,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &pool);
}

void DescriptorAllocator::ClearPool(const VkDevice& device) const {
	vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::DestroyPool(const VkDevice& device) const {
	vkDestroyDescriptorPool(device, pool, nullptr);
}

std::optional<VkDescriptorSet> DescriptorAllocator::Allocate(const VkDevice& device, VkDescriptorSetLayout layout) const {
	const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout,
	};

	VkDescriptorSet ds;
	VK_CHECK_EMPTY_OPTIONAL(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &ds), "Couldn't allocate descriptor set");

	return ds;
}

void DescriptorAllocatorGrowable::InitPools(const VkDevice& device, const uint32_t initialSets, const std::span<PoolSizeRatio> poolRatios) {
	ratios.clear();

	for (PoolSizeRatio r : poolRatios) {
		ratios.push_back(r);
	}

	const VkDescriptorPool& newPool = CreatePool(device, initialSets, poolRatios);

	setsPerPool = static_cast<uint32_t>(static_cast<float>(initialSets) * 1.5f); //grow it next allocation

	readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::ClearPools(const VkDevice& device) {
	for (const VkDescriptorPool& p : readyPools) {
		vkResetDescriptorPool(device, p, 0);
	}
	for (VkDescriptorPool& p : fullPools) {
		vkResetDescriptorPool(device, p, 0);
		readyPools.push_back(p);
	}
	fullPools.clear();
}

void DescriptorAllocatorGrowable::DestroyPools(const VkDevice& device) {
	for (const VkDescriptorPool& p : readyPools) {
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	readyPools.clear();
	for (const VkDescriptorPool& p : fullPools) {
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	fullPools.clear();
}

std::optional<VkDescriptorSet> DescriptorAllocatorGrowable::Allocate(const VkDevice& device, const VkDescriptorSetLayout& layout, const void* pNext) {
	//get or create a pool to allocate from
	VkDescriptorPool poolToUse = GetPool(device);

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = pNext,
		.descriptorPool = poolToUse,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout
	};

	VkDescriptorSet ds;
	if (const VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
		result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
		//allocation failed. Try again
		fullPools.push_back(poolToUse);

		poolToUse = GetPool(device);
		allocInfo.descriptorPool = poolToUse;

		VK_CHECK_EMPTY_OPTIONAL(vkAllocateDescriptorSets(device, &allocInfo, &ds), "Couldn't allocate descriptor set from growable allocator");
	}

	readyPools.push_back(poolToUse);
	return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::GetPool(const VkDevice& device) {
	VkDescriptorPool newPool;
	if (!readyPools.empty()) {
		newPool = readyPools.back();
		readyPools.pop_back();
	} else {
		//need to create a new pool
		newPool = CreatePool(device, setsPerPool, ratios);

		setsPerPool = static_cast<uint32_t>(static_cast<float>(setsPerPool) * 1.5f);
		if (setsPerPool > 4092) {
			setsPerPool = 4092;
		}
	}

	return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(const VkDevice& device, const uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto [type, ratio] : poolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = type,
			.descriptorCount = static_cast<uint32_t>(ratio * static_cast<float>(setCount))
		});
	}

	const VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = setCount,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &newPool);
	return newPool;
}

void DescriptorWriter::WriteImage(const uint32_t binding, const VkImageView& image, const VkSampler& sampler, const VkImageLayout layout, const VkDescriptorType type) {
	VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back(VkDescriptorImageInfo{
		.sampler = sampler,
		.imageView = image,
		.imageLayout = layout
	});

	const VkWriteDescriptorSet writeDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = nullptr, //left empty for now until we need to write it
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = &imageInfo,
	};
	writes.push_back(writeDescriptorSet);
}

void DescriptorWriter::WriteBuffer(const uint32_t binding, const VkBuffer& buffer, const size_t size, const size_t offset, const VkDescriptorType type) {
	VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back(VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = offset,
		.range = size,
	});

	const VkWriteDescriptorSet writeDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = nullptr, //left empty for now until we need to write it
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = &bufferInfo,
	};
	writes.push_back(writeDescriptorSet);
}

void DescriptorWriter::Clear() {
	imageInfos.clear();
	writes.clear();
	bufferInfos.clear();
}

void DescriptorWriter::UpdateSet(const VkDevice& device, const VkDescriptorSet& set) {
	for (VkWriteDescriptorSet& write : writes) {
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
