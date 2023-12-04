#pragma once
#include "pch.h"
#include "Device.h"

uint32_t findValidMemoryTypeIndex(
		const Device& device,
		uint32_t validTypeFlags,
		VkMemoryPropertyFlags requiredMemProperties
	);
