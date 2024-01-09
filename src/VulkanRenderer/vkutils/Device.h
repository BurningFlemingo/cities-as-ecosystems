#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace vkutils {
	struct SurfaceSupportDetails {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SurfaceSupportDetails queryDeviceSurfaceSupportDetails(
			const VkPhysicalDevice& physicalDevice,
			const VkSurfaceKHR& surface
		);
}
