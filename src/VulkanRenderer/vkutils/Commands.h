#pragma once

#include <vulkan/vulkan.h>
#include <functional>

struct VulkanContext;

namespace vkutils {
	void immediateSubmit(
		const VulkanContext& ctx,
		const VkCommandBuffer cmdBuffer,
		const VkQueue queue,
		VkFence fence,
		const std::function<void()>&& function
	);
}
