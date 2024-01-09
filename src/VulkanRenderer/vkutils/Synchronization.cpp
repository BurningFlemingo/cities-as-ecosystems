#include "Synchronization.h"

#include "VulkanRenderer/Context.h"
#include "VulkanRenderer/RendererPCH.h"
#include "debug/Debug.h"

VkSemaphore vkutils::createSemaphore(const VulkanContext& context) {
	VkSemaphoreCreateInfo semCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkSemaphore semaphore{};
	VkResult res{ vkCreateSemaphore(
		context.device.logical, &semCreateInfo, nullptr, &semaphore
	) };
	assertFatal(res == VK_SUCCESS, "could not create semaphore: ", res);

	return semaphore;
}

VkFence vkutils::createFence(
	const VulkanContext& context, const VkFenceCreateFlags& flags
) {
	VkFenceCreateInfo fenceCreateInfo{ .sType =
										   VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									   .flags = flags };

	VkFence fence{};
	VkResult res{
		vkCreateFence(context.device.logical, &fenceCreateInfo, nullptr, &fence)
	};
	assertFatal(res == VK_SUCCESS, "could not create fence: ", res);

	return fence;
}
