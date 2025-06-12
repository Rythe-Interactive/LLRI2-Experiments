#pragma once

// C++
#include <deque>
#include <filesystem>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <vector>

// SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// RSL
#define RSL_DEFAULT_ALIGNED_MATH false
#include <rsl/math>

// Vulkan Helper Libraries
#include <volk.h>
#define VK_NO_PROTOTYPES
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>

// Assimp
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post-processing flags
