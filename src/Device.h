#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vector>

struct SurfaceSupportDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> surfaceFormats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

SurfaceSupportDetails querySwapchainSupport(VkPhysicalDevice pDevice, VkSurfaceKHR surface);
