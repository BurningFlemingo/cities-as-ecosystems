#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace DEBUG {
#ifdef DEBUG_VALIDATION_LAYERS

std::vector<const char*> getInstanceDebugLayers();
std::vector<const char*> getInstanceDebugExtensions();
VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger, VkAllocationCallbacks* pAllocator);

#else

inline std::vector<const char*> getInstanceDebugLayers() { return {}; };
inline std::vector<const char*> getInstanceDebugExtensions() { return {}; };
inline VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() { return {}; };

inline VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) { return {}; };
inline void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger, VkAllocationCallbacks* pAllocator) { };

#endif

};
