#include "debug/vulkan/ValidationLayers.h"
#ifdef DEBUG_VALIDATION_LAYERS

#include "Extensions.h"

#include <vulkan/vulkan.h>
#include <sstream>
#include <iostream>


namespace DEBUG {

std::vector<const char*> findLayers(
		 const std::vector<VkLayerProperties>& avaliableLayerProperties,
		 const std::vector<const char*>& layersToFind);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

std::vector<const char*> getInstanceDebugLayers() {
	std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

	uint32_t layerCount{};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> avaliableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

	std::vector<const char*> layers{findLayers(avaliableLayers, validationLayers)};
	return layers;
}

std::vector<const char*> getInstanceDebugExtensions() {
	std::vector<const char*> debugExtensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

	std::vector<const char*> extensions{queryInstanceExtensions(debugExtensions)};
	return extensions;
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

VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() {
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

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
    auto createDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	VkDebugUtilsMessengerEXT debugMessenger;
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{getDebugMessengerCreateInfo()};

	if(createDebugUtilsMessenger(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("could not create debug messenger");
	}

	return debugMessenger;
}

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger, VkAllocationCallbacks* pAllocator) {
    auto destroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
				instance, "vkCreateDestroyUtilsMessengerEXT"));
	destroyDebugMessenger(instance, debugUtilsMessenger, pAllocator);
}

std::vector<const char*> findLayers(
		 const std::vector<VkLayerProperties>& avaliableLayerProperties,
		 const std::vector<const char*>& layersToFind) {
	std::stringstream err;
	std::vector<const char*> foundLayers{};
	for (const auto& layer : layersToFind) {
		auto itt{
			std::find_if(
					avaliableLayerProperties.begin(),
					avaliableLayerProperties.end(),
					[&](const auto& avaliableLayerProperty) {
						return strcmp(avaliableLayerProperty.layerName, layer) == 0;
					}
			)
		};

		if (itt != avaliableLayerProperties.end()) {
			foundLayers.emplace_back(layer);
		} else {
			err << layer << " ";
		}
	}
	if (err.str().size() > 0) {
		throw std::runtime_error(err.str());
	}

	return foundLayers;
}

}

#endif
