#pragma once

#include "mass_includer.hpp"

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
