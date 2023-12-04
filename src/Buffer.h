#pragma once
#include "pch.h"
#include "Device.h"

namespace VkUtils {

void createBuffer(
    const Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memFlags, 
	VkSharingMode sharingMode,
	VkBuffer* buffer,
    VkDeviceMemory* bufferMemory
);

void copyBuffers(
	const Device& device,
	VkCommandPool transferCmdPool,
	VkQueue transferQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize copyAmmount
);

}
