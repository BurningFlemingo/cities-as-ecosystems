#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vector>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <array>

enum class QueueFamily : uint32_t {
	graphics, 
	presentation, 
	transfer
};

struct SurfaceSupportDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> surfaceFormats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

struct QueueFamilyIndices {
	std::unordered_map<QueueFamily, uint32_t> familyToIndex{};
	std::unordered_map<uint32_t, std::vector<QueueFamily>> uniqueIndices{};
};

struct QueueInfo {
	std::unordered_map<QueueFamily, uint32_t> packedQueueFamilyIndices{};
	std::unordered_map<uint32_t, uint32_t> nQueueAtIndex;
};

SurfaceSupportDetails queryDeviceSurfaceSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
QueueInfo queryDeviceQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
QueueFamilyIndices selectDeviceQueueFamilyIndices(const QueueInfo& queueFamilyIndices);
