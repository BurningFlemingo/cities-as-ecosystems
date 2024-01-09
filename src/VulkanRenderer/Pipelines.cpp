#include "RendererPCH.h"
#include "Pipelines.h"

#include <array>

#include "debug/Debug.h"
#include "Context.h"
#include "Shader.h"

using namespace vkcore;

VkPipelineLayout createPipelineLayout(
	const VulkanContext& ctx,
	const ShaderLayout& shaderLayout,
	DeletionQueue& deletionQueue
) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = (uint32_t)shaderLayout.shaderDescriptorLayout.size(),
		.pSetLayouts = shaderLayout.shaderDescriptorLayout.data(),
		.pushConstantRangeCount = (uint32_t)shaderLayout.pushConstants.size(),
		.pPushConstantRanges = shaderLayout.pushConstants.data()
	};

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(
			ctx.device.logical, &pipelineLayoutInfo, nullptr, &pipelineLayout
		) != VK_SUCCESS) {
		logWarning("could not create pipeline layout");
		return {};
	}

	auto deleter{ [=]() {
		vkDestroyPipelineLayout(ctx.device.logical, pipelineLayout, nullptr);
	} };

	deletionQueue.pushFunction(deleter);

	return pipelineLayout;
}
