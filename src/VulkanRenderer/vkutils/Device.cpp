#include "VulkanRenderer/RendererPCH.h"
#include "Device.h"

vkutils::SurfaceSupportDetails vkutils::queryDeviceSurfaceSupportDetails(
		const VkPhysicalDevice& physicalDevice,
		const VkSurfaceKHR& surface
	) {
	SurfaceSupportDetails surfaceSupportDetails{};

	uint32_t surfaceFormatsCount{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr);

	if (surfaceFormatsCount) {
		surfaceSupportDetails.surfaceFormats.resize(surfaceFormatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceSupportDetails.surfaceFormats.data());
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceSupportDetails.surfaceCapabilities);

	uint32_t presentModesCount{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);

	if (presentModesCount) {
		surfaceSupportDetails.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
				physicalDevice,
				surface,
				&presentModesCount,
				surfaceSupportDetails.presentModes.data()
			);
	}


	return surfaceSupportDetails;
}
