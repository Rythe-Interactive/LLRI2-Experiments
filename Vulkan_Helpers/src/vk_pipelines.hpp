#pragma once

// C++
#include <optional>

// Vulkan Helper Libraries
#include <volk.h>

namespace vk_util {
	std::optional<VkShaderModule> LoadShaderModule(const char* filePath, const VkDevice& device);
};
