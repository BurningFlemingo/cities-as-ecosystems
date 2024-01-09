#pragma once

#include <vulkan/vulkan.h>

// forward declerations
struct Device;

uint32_t findValidMemoryTypeIndex(
	const Device& device,
	uint32_t validTypeFlags,
	VkMemoryPropertyFlags requiredMemProperties
);
