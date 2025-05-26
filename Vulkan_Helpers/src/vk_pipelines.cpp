#include "vk_pipelines.hpp"

// C++
#include <array>
#include <optional>

// SDL3
#include <SDL3/SDL.h>

#include "vk_initializers.hpp"
#include "vk_macros.hpp"

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

PipelineBuilder::PipelineBuilder() {
	Clear();
}

// clear all the structs we need back to 0 with their correct stype
void PipelineBuilder::Clear() {
	inputAssembly = VkPipelineInputAssemblyStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
	};
	rasterizer = VkPipelineRasterizationStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
	};
	colourBlendAttachment = VkPipelineColorBlendAttachmentState{
	};
	multisampling = VkPipelineMultisampleStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
	};
	pipelineLayout = VkPipelineLayout{
	};
	depthStencil = VkPipelineDepthStencilStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
	};
	renderInfo = VkPipelineRenderingCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO
	};
	_shaderStages.clear();
}

void PipelineBuilder::SetShaders(const VkShaderModule& vertexShader, const VkShaderModule& fragmentShader) {
	_shaderStages.clear();

	_shaderStages.push_back(vk_init::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
	_shaderStages.push_back(vk_init::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::SetInputTopology(const VkPrimitiveTopology topology) {
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = false;
}

void PipelineBuilder::SetPolygonMode(const VkPolygonMode mode) {
	rasterizer.polygonMode = mode;
	rasterizer.lineWidth = 1.0f;
}

void PipelineBuilder::SetCullMode(const VkCullModeFlags cullMode, const VkFrontFace frontFace) {
	rasterizer.cullMode = cullMode;
	rasterizer.frontFace = frontFace;
}

void PipelineBuilder::SetMultiSamplingNone() {
	multisampling.sampleShadingEnable = false;
	// multisampling defaulted to no multisampling (1 sample per pixel)
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	// no alpha to coverage either
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;
}

void PipelineBuilder::DisableBlending() {
	// default write mask
	colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	// no blending
	colourBlendAttachment.blendEnable = false;
}

void PipelineBuilder::SetColourAttachmentFormat(const VkFormat format) {
	colourAttachmentFormat = format;
	// connect the format to the renderInfo  structure
	renderInfo.pColorAttachmentFormats = &colourAttachmentFormat;
	renderInfo.colorAttachmentCount = 1;
}

void PipelineBuilder::SetDepthFormat(const VkFormat format) {
	renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::DisableDepthTest() {
	depthStencil.depthTestEnable = false;
	depthStencil.depthWriteEnable = false;
	depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	depthStencil.depthBoundsTestEnable = false;
	depthStencil.stencilTestEnable = false;
	depthStencil.front = VkStencilOpState{};
	depthStencil.back = VkStencilOpState{};
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
}

std::optional<VkPipeline> PipelineBuilder::BuildPipeline(const VkDevice& device) {
	// make viewport state from our stored viewport and scissor.
	// at the moment we won't support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	// setup dummy colour blending. We aren't using transparent objects yet
	// the blending is just "no blend", but we do write to the colour attachment
	VkPipelineColorBlendStateCreateInfo colourBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = false,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colourBlendAttachment,
	};

	// completely clear VertexInputStateCreateInfo, as we have no need for it
	constexpr VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	//dynamic state
	std::array states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = states.size(),
		.pDynamicStates = states.data(),
	};

	// build the actual pipeline
	// we now use all the info structs we have been writing into this one to create the pipeline
	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		// connect the renderInfo to the pNext extension mechanism
		.pNext = &renderInfo,
		.stageCount = static_cast<uint32_t>(_shaderStages.size()),
		.pStages = _shaderStages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colourBlending,
		.pDynamicState = &dynamicInfo,
		.layout = pipelineLayout,
	};

	//actually create the graphics pipeline
	VkPipeline newPipeline;
	VK_CHECK_EMPTY_OPTIONAL(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo,nullptr, &newPipeline), "Couldn't create graphics pipeline");

	return newPipeline;
}
