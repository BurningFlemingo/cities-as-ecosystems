#pragma once

#include <vulkan/vulkan.h>
#include <tuple>

class DeletionQueue;

namespace VulkanRenderer {
	std::tuple<VkInstance, VkDebugUtilsMessengerEXT>
		createInstance(SDL_Window* window, DeletionQueue& deletionQueue);
}
