#pragma once

#include "mass_includer.hpp"

namespace vk_util {
	[[nodiscard]] std::optional<VkShaderModule> LoadShaderModule(const char* filePath, const VkDevice& device);
};

class PipelineBuilder {
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages = {};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	VkPipelineRasterizationStateCreateInfo rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
	VkPipelineMultisampleStateCreateInfo multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipelineDepthStencilStateCreateInfo depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	VkPipelineRenderingCreateInfo renderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	VkFormat colourAttachmentFormat = VK_FORMAT_UNDEFINED;

	PipelineBuilder();

	void Clear();

	void SetShaders(const VkShaderModule& vertexShader, const VkShaderModule& fragmentShader);
	void SetInputTopology(VkPrimitiveTopology topology);
	void SetPolygonMode(VkPolygonMode mode);
	void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void SetMultiSamplingNone();
	void EnableBlendingAdditive();
	void EnableBlendingAlphaBlend();
	void DisableBlending();
	void SetColourAttachmentFormat(VkFormat format);
	void SetDepthFormat(VkFormat format);
	void EnableDepthTest(bool depthWriteEnable, VkCompareOp op);
	void DisableDepthTest();

	[[nodiscard]] std::optional<VkPipeline> BuildPipeline(const VkDevice& device);
};
