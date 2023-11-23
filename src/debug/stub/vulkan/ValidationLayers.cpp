#include "debug/include/vulkan/ValidationLayers.h"

namespace DEBUG {

std::vector<const char*> getInstanceDebugLayers() { return {}; }
std::vector<const char*> getInstanceDebugExtensions() { return {}; }
VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() { return {}; }

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) { return {}; }
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger, VkAllocationCallbacks* pAllocator) {}

}

