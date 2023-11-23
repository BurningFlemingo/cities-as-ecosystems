#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace DEBUG {

std::vector<const char*> getInstanceDebugLayers();
std::vector<const char*> getInstanceDebugExtensions();
VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger, VkAllocationCallbacks* pAllocator);

};
