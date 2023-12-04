#pragma once
#include "pch.h"
#include "Device.h"

namespace VkUtils {

void createBuffer(
    const Device& device,
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags memFlags, 
	const VkSharingMode sharingMode,
	VkBuffer* buffer,
    VkDeviceMemory* bufferMemory
);

void copyBuffers(
	const Device& device,
	const VkCommandPool transferCmdPool,
	const VkQueue transferQueue,
	const VkBuffer srcBuffer,
	const VkBuffer dstBuffer,
	const VkDeviceSize copyAmmount
);

}
