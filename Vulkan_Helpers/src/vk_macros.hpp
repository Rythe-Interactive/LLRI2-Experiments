#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(x, msg) \
	do { \
		VkResult err = x; \
		if (err) { \
			SDL_Log("Detected Vulkan error: %s: %s", msg, string_VkResult(err)); \
			SDL_TriggerBreakpoint(); \
			return SDL_APP_FAILURE; \
		} \
	} while (0)
