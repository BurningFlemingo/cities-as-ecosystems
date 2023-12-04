#include "CommandUtils.h"
#include "debug/Debug.h"

namespace VkUtils {

VkCommandBuffer beginTransientCommands(const Device& device, VkCommandPool commandPool) {
	VkCommandBuffer commandBuffer;
	{
		VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.commandPool = commandPool;
		cmdBufferAllocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device.logical, &cmdBufferAllocInfo, &commandBuffer) != VK_SUCCESS) {
			logFatal("could not allocate command buffer");
		}
	}

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);

	return commandBuffer;
}

void endTransientCommands(const Device& device, VkCommandPool pool, VkCommandBuffer cmdBuffer, VkQueue queue) {
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo bufSubmitInfo{};
	bufSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	bufSubmitInfo.commandBufferCount = 1;
	bufSubmitInfo.pCommandBuffers = &cmdBuffer;

	if (vkQueueSubmit(queue, 1, &bufSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		logFatal("could not submit command buffer");
	}
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device.logical, pool, 1, &cmdBuffer);
}

}
