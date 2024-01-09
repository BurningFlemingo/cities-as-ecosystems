#include "VulkanRenderer/RendererPCH.h"

#include "Memory.h"

#include "debug/Debug.h"
#include "VulkanRenderer/Device.h"

uint32_t findValidMemoryTypeIndex(
	const Device& device,
	uint32_t validTypeFlags,
	VkMemoryPropertyFlags requiredMemProperties
) {
	for (int i{}; i < device.memProperties.memoryTypeCount; i++) {
		if ((validTypeFlags & (1 << i)
			) &&  // bit i is set only if pDeviceMemProps.memoryTypes[i] is a
				  // valid type
			((device.memProperties.memoryTypes[i].propertyFlags &
			  requiredMemProperties)) ==
				requiredMemProperties) {  // having more properties isnt invalid
			return i;
		}
	}

	logWarning("could not find valid memory type");
	return 0;
}
