#pragma once

#include <span>
#include <filesystem>
#include <vector>

#include <vulkan/vulkan.h>

#include "Cleanup.h"

// forward declerations
namespace vkcore {
	struct ShaderLayout;
}

struct VulkanContext;
class DeletionQueue;

VkPipelineLayout createPipelineLayout(
	const VulkanContext& ctx,
	const vkcore::ShaderLayout& shaderLayout,
	DeletionQueue& deletionQueue
);
