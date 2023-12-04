#include "Buffer.h"
#include "MemoryUtils.h"
#include "CommandUtils.h"
#include "debug/Debug.h"

void createBuffer(
    const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memFlags, 
	VkSharingMode sharingMode,
	VkBuffer* buffer,
    VkDeviceMemory* bufferMemory
) {
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = sharingMode;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.size = size;

	if (vkCreateBuffer(device.logical, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS) {
		logFatal("could not create buffer");
	}

	VkMemoryRequirements bufferMemoryRequirements{};
	vkGetBufferMemoryRequirements(device.logical, *buffer, &bufferMemoryRequirements);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = bufferMemoryRequirements.size;
	memAllocInfo.memoryTypeIndex = findValidMemoryTypeIndex(device, bufferMemoryRequirements.memoryTypeBits, memFlags);

	// TODO: use a better allocation strategy
	if (vkAllocateMemory(device.logical, &memAllocInfo, nullptr, bufferMemory) != VK_SUCCESS) {
		logFatal("could not allocate buffer");
	}

	vkBindBufferMemory(device.logical, *buffer, *bufferMemory, 0);
}
void copyBuffers(
	const Device& device,
	VkCommandPool transferCmdPool,
	VkQueue transferQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize copyAmmount
) {
	VkCommandBuffer cmdBuffer{beginTransientCommands(device, transferCmdPool)};

	VkBufferCopy bufCopyRegion{};
	bufCopyRegion.size = copyAmmount;

	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &bufCopyRegion);

	endTransientCommands(device, transferCmdPool, cmdBuffer, transferQueue);
}
