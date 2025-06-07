#pragma once

#include "mass_includer.hpp"

// Engine
#include "vk_custom_types.hpp"

class VulkanEngine;

struct GeoSurface {
	uint32_t startIndex;
	uint32_t count;
};

struct MeshAsset {
	std::string name;

	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> ImportMesh(VulkanEngine* engine, const std::filesystem::path& fullPath);
