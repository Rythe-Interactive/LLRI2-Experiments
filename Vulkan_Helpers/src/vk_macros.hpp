#pragma once

// C++
#include <optional>

// SDL3
#include <SDL3/SDL.h>

// Vulkan Helper Libraries
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

#define VK_CHECK_EMPTY_OPTIONAL(x, msg) \
	do { \
		VkResult err = x; \
		if (err) { \
			SDL_Log("Detected Vulkan error: %s: %s", msg, string_VkResult(err)); \
			SDL_TriggerBreakpoint(); \
			return std::nullopt; \
		} \
	} while (0)
