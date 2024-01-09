#pragma once

#include <vulkan/vulkan.h>

// forward declerations
struct VulkanContext;

namespace vkutils {
	VkSemaphore createSemaphore(const VulkanContext& context);
	VkFence createFence(
		const VulkanContext& context, const VkFenceCreateFlags& flags
	);
}  // namespace vkutils
