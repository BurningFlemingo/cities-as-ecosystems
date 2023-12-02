#pragma once

#include "pch.h"
#include "Instance.h"

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
	uint32_t graphicsIndex{static_cast<uint32_t>(-1)};
	uint32_t transferIndex{static_cast<uint32_t>(-1)};
	uint32_t presentationIndex{static_cast<uint32_t>(-1)};

	std::unordered_map<uint32_t, std::vector<QueueFamily>> uniqueIndices{};
};

struct QueueInfo {
	std::unordered_map<QueueFamily, uint32_t> packedQueueFamilyIndices{};
	std::unordered_map<uint32_t, uint32_t> numQueuesAtIndex;
};

struct Device {
	VkDevice logical{};
	VkPhysicalDevice physical{};

	QueueFamilyIndices queueFamilyIndices{};

	VkPhysicalDeviceProperties properties{};
	VkPhysicalDeviceFeatures features{};
	VkPhysicalDeviceMemoryProperties memProperties{};
};

Device createDevice(const Instance& instance, VkSurfaceKHR surface);

SurfaceSupportDetails queryDeviceSurfaceSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
QueueInfo queryDeviceQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
QueueFamilyIndices selectDeviceQueueFamilyIndices(const QueueInfo& queueFamilyIndices);
