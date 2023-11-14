#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <stdint.h>
#include <vector>
#include <algorithm>
#include <optional>

constexpr uint32_t WINDOW_HEIGHT{1080 / 2};
constexpr uint32_t WINDOW_WIDTH{1920 / 2};

std::vector<const char*> getLayers();
std::vector<const char*> getExtensions(SDL_Window* window);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

// dll loaded functions
PFN_vkCreateDebugUtilsMessengerEXT load_PFK_vkCreateDebugUtilsMessengerEXT(VkInstance instance);
PFN_vkDestroyDebugUtilsMessengerEXT load_PFK_vkDestroyDebugUtilsMessengerEXT(VkInstance instance);

VkDebugUtilsMessengerCreateInfoEXT populateDebugCreateInfo();

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		std::cerr << "could not init sdl: " << SDL_GetError() << std::endl;
	}

	SDL_Window* window{SDL_CreateWindow("vulkan :D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN)};
	if (!window) {
		std::cerr << "window could not be created: " << SDL_GetError() << std::endl;
	}

	VkInstance instance;
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vulkan :D";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "VulkEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> enabledExtensions{getExtensions(window)};
		std::vector<const char*> enabledLayers{getLayers()};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{populateDebugCreateInfo()};

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.enabledLayerCount = enabledLayers.size();
		createInfo.ppEnabledLayerNames = enabledLayers.data();
		createInfo.pNext = &debugCreateInfo;

		if(vkCreateInstance(&createInfo, nullptr, &instance)) {
			std::cerr << "could not create vulkan instance" << std::endl;
		}
	}

	PFN_vkCreateDebugUtilsMessengerEXT VK_CreateDebugUtilsMessengerEXT{load_PFK_vkCreateDebugUtilsMessengerEXT(instance)};
	PFN_vkDestroyDebugUtilsMessengerEXT VK_DestroyDebugUtilsMessengerEXT{load_PFK_vkDestroyDebugUtilsMessengerEXT(instance)};

	VkDebugUtilsMessengerEXT debugMessenger{};
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{populateDebugCreateInfo()};
		if(VK_CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			std::cerr << "could not create debug messenger" << std::endl;
		}
	}

	VkPhysicalDevice physicalDevice{};
	std::optional<uint32_t> graphicsQueueFamilyIndex{};
	{

		uint32_t nPhysicalDevices{};
		vkEnumeratePhysicalDevices(instance, &nPhysicalDevices, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(nPhysicalDevices);
		vkEnumeratePhysicalDevices(instance, &nPhysicalDevices, physicalDevices.data());

		int highestRating{-1};
		for (const auto& pDevice : physicalDevices) {
			VkPhysicalDeviceProperties pDeviceProps{};
			VkPhysicalDeviceFeatures pDeviceFeats{};

			vkGetPhysicalDeviceProperties(pDevice, &pDeviceProps);
			vkGetPhysicalDeviceFeatures(pDevice, &pDeviceFeats);

			int currentRating{};

			switch(pDeviceProps.deviceType) {
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

			uint32_t queueFamilyCount{};
			vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilyProps.data());

			for (int i{}; i < queueFamilyProps.size(); i++) {
				if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsQueueFamilyIndex.value() = i;
				}
			}

			if ((currentRating > highestRating) && (graphicsQueueFamilyIndex.has_value())) {
				highestRating = currentRating;
				physicalDevice = pDevice;
			}
		}
	}

	{
		const float queuePriority{1.f};

		VkDeviceQueueCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = &queuePriority;

	}

	SDL_Event e;
	bool running{true};
	while (running) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
						running = false;
					}
				default:
					break;
			}
		}
	}

	VK_DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	vkDestroyInstance(instance, nullptr);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

std::vector<const char*> getExtensions(SDL_Window* window) {
	std::vector<const char*> enabledExtensions;

	uint32_t SDLExtensionCount{};
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, nullptr);

	std::vector<const char*> SDLExtensions(SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions.data());

	std::vector<const char*> extensions;
	extensions.reserve(SDLExtensionCount);
	for (const auto& extension : SDLExtensions) {
		extensions.emplace_back(extension);
	}
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	uint32_t avaliableExtensionCount{};
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, avaliableExtensions.data());

	for (const auto& extension : extensions) {
		auto itt{
			std::find_if(
					avaliableExtensions.begin(),
					avaliableExtensions.end(),
					[&](const auto& avaliableExtension) { return strcmp(avaliableExtension.extensionName, extension) == 0; }
					)
		};

		if (itt != avaliableExtensions.end()) {
			enabledExtensions.emplace_back(extension);
		}
	}
	if (enabledExtensions.size() < extensions.size()) {
		std::cout << "not all extensions avaliable: " << enabledExtensions.size() << " of " << extensions.size() << std::endl;
	}

	return enabledExtensions;
}

std::vector<const char*> getLayers() {
	std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

	uint32_t layerCount{};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> avaliableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

	std::vector<const char*> enabledLayers;
	for (const auto& validationLayer : validationLayers) {
		auto itt{
			std::find_if(
					avaliableLayers.begin(),
					avaliableLayers.end(),
					[&](const auto& layer) { return strcmp(validationLayer, layer.layerName) == 0; }
					)
		};

		if (itt != avaliableLayers.end()) {
			enabledLayers.emplace_back(validationLayer);
		}
	}
	if (enabledLayers.size() < validationLayers.size()) {
		std::cout << "not all validation layers avaliable: " << enabledLayers.size() << " of " << validationLayers.size() << std::endl;
	}

	return validationLayers;
}


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	std::string severityStr{};
	switch(messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severityStr = "INFO";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severityStr = "VERBOSE";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severityStr = "WARNING";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severityStr = "ERROR";
			break;
		default:
			break;
	}

    std::cerr << "\tvalidation layer: " << severityStr << " : " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// create stub
PFN_vkCreateDebugUtilsMessengerEXT load_PFK_vkCreateDebugUtilsMessengerEXT(VkInstance instance) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    return func;
}

PFN_vkDestroyDebugUtilsMessengerEXT load_PFK_vkDestroyDebugUtilsMessengerEXT(VkInstance instance) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDestroyUtilsMessengerEXT"));
    return func;
}

VkDebugUtilsMessengerCreateInfoEXT populateDebugCreateInfo() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	return createInfo;
}


