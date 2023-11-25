#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vector>
#include <optional>
#include <tuple>

struct SurfaceSupportDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> surfaceFormats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamilyIndex;
	std::optional<uint32_t> presentationFamilyIndex;
	std::optional<uint32_t> transferNoGraphicsFamilyIndex;
};

SurfaceSupportDetails queryDeviceSurfaceSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
QueueFamilyIndices queryDeviceQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
std::tuple<int, int> findSuitablePhysicalDevice();
