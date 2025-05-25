#include "vk_pipelines.hpp"

#include <SDL3/SDL.h>

std::optional<VkShaderModule> vk_util::LoadShaderModule(const char* filePath, const VkDevice& device) {
	size_t fileSize;
	void* contents = SDL_LoadFile(filePath, &fileSize);
	if (contents == nullptr) {
		SDL_Log("Couldn't load shader from disk! %s\n%s", filePath, SDL_GetError());
		return std::nullopt;
	}

	// create a new shader module, using the buffer we loaded
	const VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fileSize,
		.pCode = static_cast<uint32_t*>(contents),
	};

	// check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return std::nullopt;
	}
	return shaderModule;
}
