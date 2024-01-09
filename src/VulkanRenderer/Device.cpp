#include "Device.h"

#include "Cleanup.h"
#include "Extensions.h"
#include "RendererPCH.h"
#include "debug/Debug.h"

#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace {
	struct QueueInfo {
		std::unordered_map<QueueFamily, uint32_t> packedQueueFamilyIndices;
		std::vector<uint32_t> numQueuesAtIndex;
	};

	struct QueueFamilyQueueCounts {
		uint32_t graphics;
		uint32_t presentation;
		uint32_t transfer;
	};

	std::optional<uint32_t> bitscanForward(uint32_t val);

	VkPhysicalDevice findSuitablePhysicalDevice(
		const VkInstance &instance, const VkSurfaceKHR &surface
	);

	VkDevice createLogicalDevice(
		const VkPhysicalDevice &physicalDevice,
		const QueueFamilyIndices &queueFamilyIndices,
		const QueueFamilyQueueCounts &queueFamilyCounts,
		const std::vector<const char *> &deviceExtensions,
		const VkPhysicalDeviceFeatures2 &deviceFeatures
	);

	QueueInfo queryDeviceQueueFamilyIndices(
		VkPhysicalDevice physicalDevice, VkSurfaceKHR surface
	);

	QueueFamilyIndices
		selectDeviceQueueFamilyIndices(const QueueInfo &queueFamilyIndices);
}  // namespace

Device VulkanRenderer::createDevice(
	const VkSurfaceKHR &surface,
	const VkInstance &instance,
	DeletionQueue &deletionQueue
) {
	std::vector<const char *> requiredDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	};
	VkPhysicalDeviceVulkan13Features vulkan13Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};
	VkPhysicalDeviceVulkan12Features vulkan12Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &vulkan13Features,
		.descriptorIndexing = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,

	};
	VkPhysicalDeviceVulkan11Features vulkan11Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &vulkan12Features
	};
	VkPhysicalDeviceFeatures defaultFeatures{
		.samplerAnisotropy = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 requiredFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &vulkan11Features,
		.features = defaultFeatures,
	};

	Device device{};

	device.physical = findSuitablePhysicalDevice(instance, surface);

	vkGetPhysicalDeviceProperties(device.physical, &device.properties);

	QueueInfo queueInfo{
		queryDeviceQueueFamilyIndices(device.physical, surface)
	};
	device.queueFamilyIndices = selectDeviceQueueFamilyIndices(queueInfo);

	QueueFamilyQueueCounts queueFamilyCounts{ .graphics = 1,
											  .presentation = 1,
											  .transfer = 1 };

	device.logical = createLogicalDevice(
		device.physical,
		device.queueFamilyIndices,
		queueFamilyCounts,
		requiredDeviceExtensions,
		requiredFeatures
	);

	vkGetPhysicalDeviceMemoryProperties(device.physical, &device.memProperties);

	deletionQueue.pushFunction([=]() {
		vkDestroyDevice(device.logical, nullptr);
	});

	return device;
}

Queues getDeviceQueues(const Device &device) {
	VkQueue graphicsQueue{};
	VkQueue presentationQueue{};
	VkQueue transferQueue{};

	for (auto index : device.queueFamilyIndices.uniqueIndices) {
		for (uint32_t i{}; i < index.second.size(); i++) {
			QueueFamily familySupported{ index.second[i] };
			uint32_t queueFamilyIndex{ index.first };

			switch (familySupported) {
				case QueueFamily::graphics:
					vkGetDeviceQueue(
						device.logical, queueFamilyIndex, i, &graphicsQueue
					);
				case QueueFamily::presentation:
					vkGetDeviceQueue(
						device.logical, queueFamilyIndex, i, &presentationQueue
					);
					break;
				case QueueFamily::transfer:
					vkGetDeviceQueue(
						device.logical, queueFamilyIndex, i, &transferQueue
					);
					break;
				default:
					break;
			}
		}
	}

	Queues queues{ .graphicsQueue = graphicsQueue,
				   .presentationQueue = presentationQueue,
				   .transferQueue = transferQueue };

	return queues;
}

namespace {
	QueueFamilyIndices selectDeviceQueueFamilyIndices(const QueueInfo &queueInfo
	) {
		const uint32_t &graphicsQueueIndices{
			queueInfo.packedQueueFamilyIndices.at(QueueFamily::graphics)
		};
		const uint32_t &presentationQueueIndices{
			queueInfo.packedQueueFamilyIndices.at(QueueFamily::presentation)
		};

		QueueFamilyIndices queueFamilyIndices{};

		uint32_t bestCaseGraphicsPresentationIndices{
			graphicsQueueIndices & presentationQueueIndices
		};

		uint32_t graphicsIndex{};
		uint32_t presentationIndex{};
		uint32_t transferIndex{};

		if (bestCaseGraphicsPresentationIndices) {
			graphicsIndex =
				bitscanForward(bestCaseGraphicsPresentationIndices).value();
			presentationIndex = graphicsIndex;
		} else {
			graphicsIndex = bitscanForward(graphicsQueueIndices).value();
			presentationIndex =
				bitscanForward(presentationQueueIndices).value();
		}
		transferIndex = graphicsIndex;

		queueFamilyIndices.graphicsIndex = graphicsIndex;
		queueFamilyIndices.presentationIndex = presentationIndex;
		queueFamilyIndices.transferIndex = transferIndex;

		queueFamilyIndices.uniqueIndices[graphicsIndex].emplace_back(
			QueueFamily::graphics
		);
		queueFamilyIndices.uniqueIndices[presentationIndex].emplace_back(
			QueueFamily::presentation
		);
		queueFamilyIndices.uniqueIndices[transferIndex].emplace_back(
			QueueFamily::transfer
		);

		queueFamilyIndices.indices[QueueFamily::graphics] = graphicsIndex;
		queueFamilyIndices.indices[QueueFamily::presentation] =
			presentationIndex;
		queueFamilyIndices.indices[QueueFamily::transfer] = transferIndex;

		return queueFamilyIndices;
	}

	VkPhysicalDevice findSuitablePhysicalDevice(
		const VkInstance &instance, const VkSurfaceKHR &surface
	) {
		VkPhysicalDevice physicalDevice{};

		uint32_t physicalDeviceCount{};
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(
			instance, &physicalDeviceCount, physicalDevices.data()
		);

		int highestRating{ -1 };
		for (const auto &pDevice : physicalDevices) {
			VkPhysicalDeviceProperties pDeviceProps{};
			vkGetPhysicalDeviceProperties(pDevice, &pDeviceProps);

			VkPhysicalDeviceFeatures pDeviceFeats{};
			vkGetPhysicalDeviceFeatures(pDevice, &pDeviceFeats);

			uint32_t queueFamilyCount{};
			vkGetPhysicalDeviceQueueFamilyProperties(
				pDevice, &queueFamilyCount, nullptr
			);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount
			);
			vkGetPhysicalDeviceQueueFamilyProperties(
				pDevice, &queueFamilyCount, queueFamilies.data()
			);

			VkBool32 thisDeviceSupportsSurface{};
			VkBool32 thisDeviceSupportsGraphics{};
			for (uint32_t i{}; i < queueFamilies.size(); i++) {
				VkBool32 surfaceSupported{};
				vkGetPhysicalDeviceSurfaceSupportKHR(
					pDevice, i, surface, &surfaceSupported
				);

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
				physicalDevice = pDevice;
			}
		}

		return physicalDevice;
	}

	VkDevice createLogicalDevice(
		const VkPhysicalDevice &physicalDevice,
		const QueueFamilyIndices &queueFamilyIndices,
		const QueueFamilyQueueCounts &queueFamilyCounts,
		const std::vector<const char *> &deviceExtensions,
		const VkPhysicalDeviceFeatures2 &deviceFeatures
	) {
		VkDevice device{};

		const float queuePriority[3] = { 1.0f, 1.0f, 1.0f };

		std::vector<VkDeviceQueueCreateInfo> queuesToCreate{};
		size_t totalNumQueues{ queueFamilyCounts.graphics
							   + queueFamilyCounts.presentation
							   + queueFamilyCounts.transfer };
		queuesToCreate.reserve(totalNumQueues);

		bool graphicsQueuesFound{};
		bool transferQueuesFound{};
		bool presentationQueuesFound{};

		for (const auto &index : queueFamilyIndices.uniqueIndices) {
			uint32_t nQueues{};

			if (graphicsQueuesFound && presentationQueuesFound
				&& transferQueuesFound) {
				break;
			}

			for (const auto &family : index.second) {
				switch (family) {
					case QueueFamily::graphics:
						if (!graphicsQueuesFound) {
							graphicsQueuesFound = true;
							nQueues += queueFamilyCounts.graphics;
						}
						break;
					case QueueFamily::presentation:
						if (!presentationQueuesFound) {
							presentationQueuesFound = true;
							nQueues += queueFamilyCounts.presentation;
						}
						break;
					case QueueFamily::transfer:
						if (!transferQueuesFound) {
							transferQueuesFound = true;
							nQueues += queueFamilyCounts.transfer;
						}
						break;
					default:
						break;
				}
			}

			if (nQueues) {
				VkDeviceQueueCreateInfo queueCreateInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = index.first,
					.queueCount = nQueues,
					.pQueuePriorities = queuePriority,
				};

				queuesToCreate.emplace_back(queueCreateInfo);
			}
		}

		VkDeviceCreateInfo deviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &deviceFeatures,
			.queueCreateInfoCount = (uint32_t)queuesToCreate.size(),
			.pQueueCreateInfos = queuesToCreate.data(),
			.enabledExtensionCount = (uint32_t)deviceExtensions.size(),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = nullptr,
		};

		vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

		return device;
	}

	QueueInfo queryDeviceQueueFamilyIndices(
		VkPhysicalDevice physicalDevice, VkSurfaceKHR surface
	) {
		QueueInfo queueInfo{};

		uint32_t queueFamilyPropCount{};
		vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevice, &queueFamilyPropCount, nullptr
		);

		std::vector<VkQueueFamilyProperties> queueFamilyProps(
			queueFamilyPropCount
		);
		vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevice, &queueFamilyPropCount, queueFamilyProps.data()
		);

		queueInfo.numQueuesAtIndex.resize(queueFamilyProps.size());

		// indices are packed into a uint32_t so that bitwise operations can be
		// performed on them
		uint32_t graphicsQueueIndices{};
		uint32_t presentationQueueIndices{};
		uint32_t transferQueueIndices{};
		for (uint32_t i{}; i < queueFamilyProps.size(); i++) {
			queueInfo.numQueuesAtIndex[i] = queueFamilyProps[i].queueCount;

			VkBool32 surfaceSupport{};
			vkGetPhysicalDeviceSurfaceSupportKHR(
				physicalDevice, i, surface, &surfaceSupport
			);
			if (surfaceSupport) {
				presentationQueueIndices |= 1 << i;
			}

			if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsQueueIndices |= 1 << i;
			}

			if (queueFamilyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
				transferQueueIndices |= 1 << i;
			}
		}

		queueInfo.packedQueueFamilyIndices[QueueFamily::graphics] =
			graphicsQueueIndices;
		queueInfo.packedQueueFamilyIndices[QueueFamily::presentation] =
			presentationQueueIndices;
		queueInfo.packedQueueFamilyIndices[QueueFamily::transfer] =
			transferQueueIndices;

		return queueInfo;
	}

	std::optional<uint32_t> bitscanForward(uint32_t val) {
		constexpr size_t bitsInUInt{ sizeof(val) * 8 };
		for (uint32_t i{}; i < bitsInUInt; i++) {
			if (val & 1) {
				return i;
			}
			val = val >> 1;
		}

		return {};
	}
}  // namespace
