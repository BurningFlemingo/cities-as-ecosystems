#include "Device.h"

#include "Extensions.h"

#include <string>
#include <sstream>
#include <iostream>
#include <optional>

namespace VkUtils {

struct PhysicalDeviceInfo {
	VkPhysicalDevice handle;

	SurfaceSupportDetails surfaceSupportDetails{};
	VkPhysicalDeviceProperties properties{};
	VkPhysicalDeviceFeatures features{};
};

SurfaceSupportDetails queryDeviceSurfaceSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
	SurfaceSupportDetails swapchainDetails{};

	uint32_t surfaceFormatsCount{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr);

	if (surfaceFormatsCount) {
		swapchainDetails.surfaceFormats.resize(surfaceFormatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, swapchainDetails.surfaceFormats.data());
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainDetails.surfaceCapabilities);

	uint32_t presentModesCount{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);

	if (presentModesCount) {
		swapchainDetails.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
				physicalDevice,
				surface,
				&presentModesCount,
				swapchainDetails.presentModes.data()
			);
	}

	return swapchainDetails;
}

std::optional<uint32_t> bitscanforward(uint32_t val) {
	constexpr uint32_t bitsInUInt{sizeof(val) * 8};
	for (int i{}; i < bitsInUInt; i++) {
		if (val & 1) {
			return i;
		}
		val = val >> 1;
	}

	return {};
}

// a map is used instaed of a vector for simplicity, change if it becomes a bottleneck
QueueInfo queryDeviceQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
	QueueInfo queueInfo{};

	uint32_t queueFamilyPropCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyPropCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropCount, queueFamilyProps.data());

	// indices are packed into an int so that bitwise operations can be performed on them
	uint32_t graphicsQueueIndices{};
	uint32_t presentationQueueIndices{};
	uint32_t transferQueueIndices{};
	for (uint32_t i{}; i < queueFamilyProps.size(); i++) {
		queueInfo.numQueuesAtIndex[i] = queueFamilyProps[i].queueCount;

		VkBool32 surfaceSupport{};
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport);
		if (surfaceSupport) {
			presentationQueueIndices |= 1 << i;
		}

		if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsQueueIndices |= 1 << i;
		}

		if (queueFamilyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT){
			transferQueueIndices |= 1 << i;
		}
	}

	queueInfo.packedQueueFamilyIndices[QueueFamily::graphics] = graphicsQueueIndices;
	queueInfo.packedQueueFamilyIndices[QueueFamily::presentation] = presentationQueueIndices;
	queueInfo.packedQueueFamilyIndices[QueueFamily::transfer] = transferQueueIndices;

	return queueInfo;
}

QueueFamilyIndices selectDeviceQueueFamilyIndices(const QueueInfo& queueInfo) {
	const uint32_t& graphicsQueueIndices{queueInfo.packedQueueFamilyIndices.at(QueueFamily::graphics)};
	const uint32_t& presentationQueueIndices{queueInfo.packedQueueFamilyIndices.at(QueueFamily::presentation)};
	const uint32_t& transferQueueIndices{queueInfo.packedQueueFamilyIndices.at(QueueFamily::transfer)};

	QueueFamilyIndices queueFamilyIndices{};

	uint32_t bestCaseGraphicsPresentationIndices{graphicsQueueIndices & presentationQueueIndices};

	uint32_t graphicsIndex{};
	uint32_t presentationIndex{};

	if (bestCaseGraphicsPresentationIndices) {
		graphicsIndex = bitscanforward(bestCaseGraphicsPresentationIndices).value();
		presentationIndex = graphicsIndex;
	} else {
		graphicsIndex = bitscanforward(graphicsQueueIndices).value();
		presentationIndex = bitscanforward(presentationQueueIndices).value();
	}

	uint32_t bestTransferIndices{transferQueueIndices & (~graphicsQueueIndices)};
	uint32_t transferIndex{};

	if (bestTransferIndices) {
		transferIndex = bitscanforward(bestTransferIndices).value();
	} else {
		transferIndex = graphicsIndex;
	}

	queueFamilyIndices.graphicsIndex = graphicsIndex;
	queueFamilyIndices.presentationIndex = presentationIndex;
	queueFamilyIndices.transferIndex = transferIndex;

	queueFamilyIndices.uniqueIndices[graphicsIndex].emplace_back(QueueFamily::graphics);
	queueFamilyIndices.uniqueIndices[presentationIndex].emplace_back(QueueFamily::presentation);
	queueFamilyIndices.uniqueIndices[transferIndex].emplace_back(QueueFamily::transfer);

	return queueFamilyIndices;
}

PhysicalDeviceInfo createPhysicalDevice(Instance instance, VkSurfaceKHR surface) {
	PhysicalDeviceInfo deviceInfo{};

	uint32_t physicalDeviceCount{};
	vkEnumeratePhysicalDevices(instance.handle, &physicalDeviceCount, nullptr);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance.handle, &physicalDeviceCount, physicalDevices.data());

	int highestRating{-1};
	for (const auto& pDevice : physicalDevices) {
		VkPhysicalDeviceProperties pDeviceProps{};
		vkGetPhysicalDeviceProperties(pDevice, &pDeviceProps);

		VkPhysicalDeviceFeatures pDeviceFeats{};
		vkGetPhysicalDeviceFeatures(pDevice, &pDeviceFeats);

		uint32_t queueFamilyCount{};
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());

		bool thisDeviceSupportsSurface{};
		bool thisDeviceSupportsGraphics{};
		for (uint32_t i{}; i < queueFamilies.size(); i++) {
			VkBool32 surfaceSupported{};
			vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &surfaceSupported);

			thisDeviceSupportsSurface |= surfaceSupported;

			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				thisDeviceSupportsGraphics |= true;
			}
		}


		if (!thisDeviceSupportsSurface || !thisDeviceSupportsGraphics) {
			continue;
		}

		int currentRating{};
		switch (pDeviceProps.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				currentRating += 1000;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				currentRating += 100;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				currentRating += 10;
				break;
			default:
				break;
		}

		if (pDeviceFeats.samplerAnisotropy) {
			currentRating += 1;
		}

		if (currentRating > highestRating) {
			highestRating = currentRating;
			deviceInfo.handle = pDevice;
			deviceInfo.properties = pDeviceProps;
			deviceInfo.features = pDeviceFeats;
		}
	}

	return deviceInfo;
}

VkDevice createLogicalDevice(
		const PhysicalDeviceInfo& physicalDeviceInfo,
		const QueueFamilyIndices& queueFamilyIndices,
		const std::vector<const char*> deviceExtensions
	) {
	VkDevice device{};

	const float queuePriority[3] = {1.0f, 1.0f, 1.0f};
	size_t nQueueFamilyIndices{queueFamilyIndices.uniqueIndices.size()};

	std::vector<VkDeviceQueueCreateInfo> queuesToCreate{};
	queuesToCreate.reserve(nQueueFamilyIndices);
	for (int i{}; i < nQueueFamilyIndices; i++) {
		uint32_t queueCount{};
		for (const auto& family : queueFamilyIndices.uniqueIndices.at(i)) {
			if (family == QueueFamily::presentation) {
				continue;
			}
			queueCount++;
		}
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = i;
		queueCreateInfo.queueCount = queueCount;
		queueCreateInfo.pQueuePriorities = queuePriority;

		queuesToCreate.emplace_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = queuesToCreate.size();
	deviceCreateInfo.pQueueCreateInfos = queuesToCreate.data();
	deviceCreateInfo.pEnabledFeatures = nullptr;
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceInfo.features;

	vkCreateDevice(physicalDeviceInfo.handle, &deviceCreateInfo, nullptr, &device);

	return device;
}

Device createDevice(const Instance& instance, VkSurfaceKHR surface) {
	std::vector<const char*> requiredDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME
	};
	Device device{};

	PhysicalDeviceInfo physicalInfo{createPhysicalDevice(instance, surface)};
	device.physical = physicalInfo.handle;
	device.properties = physicalInfo.properties;
	device.features = physicalInfo.features;

	QueueInfo queueInfo{queryDeviceQueueFamilyIndices(device.physical, surface)};
	device.queueFamilyIndices = selectDeviceQueueFamilyIndices(queueInfo);

	device.logical = createLogicalDevice(physicalInfo, device.queueFamilyIndices, requiredDeviceExtensions);

	VkPhysicalDeviceProperties deviceProps{};
	vkGetPhysicalDeviceProperties(device.physical, &deviceProps);

	VkPhysicalDeviceMemoryProperties memProps{};
	vkGetPhysicalDeviceMemoryProperties(device.physical, &memProps);
	device.memProperties = memProps;

	return device;
}

}
