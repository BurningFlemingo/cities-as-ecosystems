#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#undef DEBUG_INLINE
#undef DEBUG_STUB
#undef DEBUG_STUB_RET

#define DEBUG_INLINE  inline
#define DEBUG_STUB {}
#define DEBUG_STUB_RET { return {}; }

#ifdef DEBUG_VALIDATION_LAYERS
	#undef DEBUG_INLINE
	#undef DEBUG_STUB
	#undef DEBUG_STUB_RET

	#define DEBUG_INLINE 
	#define DEBUG_STUB 
	#define DEBUG_STUB_RET
#endif

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
