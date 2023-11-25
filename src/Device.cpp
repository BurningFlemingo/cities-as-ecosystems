#include "Device.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

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
		queueInfo.nQueueAtIndex[i] = queueFamilyProps[i].queueCount;
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

	queueFamilyIndices.familyToIndex[QueueFamily::graphics] = graphicsIndex;
	queueFamilyIndices.familyToIndex[QueueFamily::presentation] = presentationIndex;
	queueFamilyIndices.familyToIndex[QueueFamily::transfer] = transferIndex;

	queueFamilyIndices.uniqueIndices[graphicsIndex].emplace_back(QueueFamily::graphics);
	queueFamilyIndices.uniqueIndices[presentationIndex].emplace_back(QueueFamily::presentation);
	queueFamilyIndices.uniqueIndices[transferIndex].emplace_back(QueueFamily::transfer);

	return queueFamilyIndices;
}
