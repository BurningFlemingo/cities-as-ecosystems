#include "VulkanRenderer/RendererPCH.h"

#include <array>

#include "Commands.h"

#include "debug/Debug.h"
#include "VulkanRenderer/Context.h"
#include "VulkanRenderer/DefaultCreateInfos.h"

void vkutils::immediateSubmit(
	const VulkanContext& ctx,
	const VkCommandBuffer cmdBuffer,
	const VkQueue queue,
	VkFence fence,
	const std::function<void()>&& function
) {
	CHECK_VK_FATAL(vkResetFences(ctx.device.logical, 1, &fence));
	CHECK_VK_FATAL(vkResetCommandBuffer(cmdBuffer, 0));

	VkCommandBufferBeginInfo cmdBufferBeginInfo{
		vkdefaults::commandBufferBeginInfo(
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		)
	};

	vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

	function();

	vkEndCommandBuffer(cmdBuffer);

	VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {
		vkdefaults::cmdBufferSubmitInfo(cmdBuffer)
	};

	VkSubmitInfo2 submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmdBufferSubmitInfo,
	};

	if (vkQueueSubmit2(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
		logWarning("could not submit transient command");
	}

	CHECK_VK_FATAL(
		vkWaitForFences(ctx.device.logical, 1, &fence, VK_TRUE, UINT_MAX)
	);
}
