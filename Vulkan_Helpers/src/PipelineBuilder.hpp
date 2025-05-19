#pragma once
#include <volk.h>
#include <vector>

class PipelineBuilder {
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState colourBlendAttachment;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineLayout pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineRenderingCreateInfo renderInfo;
	VkFormat colourAttachmentFormat;

public:
	PipelineBuilder() { Clear(); }

	void Clear();

	VkPipeline BuildPipeline(VkDevice device);

	void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	void SetInputTopology(VkPrimitiveTopology topology);
	void SetPolygonMode(VkPolygonMode mode);
	void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void SetMultisamplingNone();
	void DisableBlending();
	void SetColourAttachmentFormat(VkFormat format);
	void SetDepthFormat(VkFormat format);
	void DisableDepthTest();
};
