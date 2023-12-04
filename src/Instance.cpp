#include "Instance.h"

#include "Extensions.h"
#include "debug/Debug.h"

Instance createInstance(SDL_Window* window) {
	Instance instance{};

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vulkan :D";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "VulkEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> instanceExtensions{getSurfaceExtensions(window)};

	std::vector<const char*> enabledExtensions{queryInstanceExtensions(instanceExtensions)};
	std::vector<const char*> debugExtensions{DEBUG::getInstanceDebugExtensions()};
	enabledExtensions.reserve(debugExtensions.size());
	enabledExtensions.insert(std::end(enabledExtensions), std::begin(debugExtensions), std::end(debugExtensions));

	std::vector<const char*> enabledLayers{DEBUG::getInstanceDebugLayers()};

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{DEBUG::getDebugMessengerCreateInfo()};

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = enabledExtensions.size();
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();
	createInfo.enabledLayerCount = enabledLayers.size();
	createInfo.ppEnabledLayerNames = enabledLayers.data();
	createInfo.pNext = &debugCreateInfo;

	if(vkCreateInstance(&createInfo, nullptr, &instance.handle)) {
		logFatal("could not create instance");
	}

	instance.debugMessenger = DEBUG::createDebugMessenger(instance.handle);

	return instance;
}
