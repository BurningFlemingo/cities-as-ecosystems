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
	constexpr uint32_t bitsInUInt{32};
	for (int i{}; i < bitsInUInt; i++) {
		if (val & 1) {
			return i;
		}
		val = val >> 1;
	}

	return {};
}

QueueFamilyIndices queryDeviceQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
	QueueFamilyIndices queueFamilyIndices{};

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
	}

	uint32_t transferQueueNoGraphicsIndices{transferQueueIndices & (~graphicsQueueIndices)};
	if (transferQueueNoGraphicsIndices) {
		std::optional<uint32_t> indexOfTransferQueue{bitscanforward(transferQueueNoGraphicsIndices)};
		queueFamilyIndices.transferFamilyIndex = indexOfTransferQueue.value();
	} else {
		std::optional<uint32_t> indexOfTransferQueue{bitscanforward(graphicsQueueIndices)};
		queueFamilyIndices.transferFamilyIndex = indexOfTransferQueue.value();
	}

	uint32_t graphicsPresentationQueue{graphicsQueueIndices & presentationQueueIndices};
	std::optional<uint32_t> indexOfBestCaseQueue{bitscanforward(graphicsPresentationQueue)};
	if (indexOfBestCaseQueue.has_value()) {
		queueFamilyIndices.graphicsFamilyIndex = indexOfBestCaseQueue.value();
		queueFamilyIndices.presentationFamilyIndex = indexOfBestCaseQueue.value();
	} else {
		std::optional<uint32_t> indexOfGraphicsQueue{bitscanforward(graphicsQueueIndices)};
		queueFamilyIndices.graphicsFamilyIndex = indexOfGraphicsQueue;
		
		std::optional<uint32_t> indexOfPresentationQueue{bitscanforward(presentationQueueIndices)};
		queueFamilyIndices.presentationFamilyIndex = indexOfPresentationQueue;
	}

	return queueFamilyIndices;
}
