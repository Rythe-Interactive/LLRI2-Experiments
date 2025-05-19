#include "PipelineBuilder.hpp"

#include <array>

#include "SDL3/SDL_log.h"

void PipelineBuilder::Clear() {
	inputAssembly = VkPipelineInputAssemblyStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
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

	colourAttachmentFormat = VK_FORMAT_UNDEFINED;

	shaderStages.clear();
}

VkPipeline PipelineBuilder::BuildPipeline(const VkDevice device) {
	// make viewport state from our stored viewport and scissor.
	// at the moment we won't support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	// setup dummy color blending. We aren't using transparent objects yet
	// the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = false,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colourBlendAttachment,
	};

	// completely clear VertexInputStateCreateInfo, as we have no need for it
	constexpr VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	}; // build the actual pipeline

	std::array state = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pDynamicStates = state.data(),
		.dynamicStateCount = state.size(),
	};

	// we now use all the info structs we have been writing into this one
	// to create the pipeline
	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		// connect the renderInfo to the pNext extension mechanism
		.pNext = &renderInfo,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicInfo,
		.layout = pipelineLayout,
	};

	// it's easy to error out on create graphics pipeline, so we handle it a bit
	// better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		SDL_Log("failed to create pipeline");
		return VK_NULL_HANDLE;
	}

	return newPipeline;
}

VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(
	const VkShaderStageFlagBits stage,
	const VkShaderModule shaderModule
) {
	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		//shader stage
		.stage = stage,
		//module containing the code for this shader stage
		.module = shaderModule,
		//the entry point of the shader
		.pName = "main",
	};
}

void PipelineBuilder::SetShaders(const VkShaderModule vertexShader, const VkShaderModule fragmentShader) {
	shaderStages.clear();
	shaderStages.push_back(PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
	shaderStages.push_back(PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::SetInputTopology(const VkPrimitiveTopology topology) {
	inputAssembly.topology = topology;
	// we are not going to use primitive restart on the entire tutorial so leave it on false
	inputAssembly.primitiveRestartEnable = false;
}

void PipelineBuilder::SetPolygonMode(const VkPolygonMode mode) {
	rasterizer.polygonMode = mode;
	rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::SetCullMode(const VkCullModeFlags cullMode, const VkFrontFace frontFace) {
	rasterizer.cullMode = cullMode;
	rasterizer.frontFace = frontFace;
}

void PipelineBuilder::SetMultisamplingNone() {
	multisampling.sampleShadingEnable = VK_FALSE;
	// multisampling defaulted to no multisampling (1 sample per pixel)
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	// no alpha to coverage either
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::DisableBlending() {
	// default write mask
	colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// no blending
	colourBlendAttachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::SetColourAttachmentFormat(const VkFormat format) {
	colourAttachmentFormat = format;
	// connect the format to the renderInfo  structure
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachmentFormats = &colourAttachmentFormat;
}

void PipelineBuilder::SetDepthFormat(const VkFormat format) {
	renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::DisableDepthTest() {
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
	depthStencil.minDepthBounds = 0.f;
	depthStencil.maxDepthBounds = 1.f;
}
