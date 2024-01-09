#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Device.h"
#include "Cleanup.h"

// forward declerations
typedef struct SDL_Window SDL_Window;

struct VulkanContext {
	VmaAllocator allocator;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkSurfaceKHR surface;

	Device device;
};

namespace VulkanRenderer {
	VulkanContext createVulkanContext(SDL_Window* window, DeletionQueue& deletionQueue);
}
