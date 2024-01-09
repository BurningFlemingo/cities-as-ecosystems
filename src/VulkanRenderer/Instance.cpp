#include "RendererPCH.h"

#include "Instance.h"

#include "Extensions.h"
#include "debug/Debug.h"
#include "Cleanup.h"

std::tuple<VkInstance, VkDebugUtilsMessengerEXT> VulkanRenderer::createInstance(
		SDL_Window* window,
		DeletionQueue& deletionQueue
	) {
	std::vector<const char*> requiredSurfaceExtensions{getSurfaceExtensions(window)};

	std::vector<const char*> additionalRequiredExtensions{
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};
	std::vector<const char*> optionalExtensions{
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	std::vector<const char*> enabledExtensions{
		queryInstanceExtensions(
				requiredSurfaceExtensions,
				additionalRequiredExtensions,
				optionalExtensions
		)
	};

	std::vector<const char*> enabledLayers{DEBUG::getInstanceDebugLayers()};

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
		.pApplicationName = "vulkan :D", 
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0), 
		.pEngineName = "VulkEngine", 
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0), 
		.apiVersion = VK_API_VERSION_1_3
	};

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{DEBUG::getDebugMessengerCreateInfo()};

	VkInstanceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 
		.pNext = &debugCreateInfo, 
		.pApplicationInfo = &appInfo, 
		.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
		.ppEnabledLayerNames = enabledLayers.data(), 
		.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()), 
		.ppEnabledExtensionNames = enabledExtensions.data(), 
	};

	VkInstance instance{};
	if(vkCreateInstance(&createInfo, nullptr, &instance)) {
		logFatal("could not create instance");
	}

	VkDebugUtilsMessengerEXT debugMessenger{DEBUG::createDebugMessenger(instance)};

	deletionQueue.pushFunction([=]() {
		DEBUG::destroyDebugMessenger(instance, debugMessenger, nullptr);
		vkDestroyInstance(instance, nullptr);
	});

	return {instance, debugMessenger};
}
