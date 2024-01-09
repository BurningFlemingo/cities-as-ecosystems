#pragma once

#include <vector>
#include <vulkan/vulkan.h>

// forward declerations
typedef struct SDL_Window SDL_Window;

std::vector<const char*> getSurfaceExtensions(SDL_Window* window);
std::vector<const char*> queryInstanceExtensions(const std::vector<const char*>& extensions);
std::vector<const char*> queryInstanceExtensions(
		const std::vector<const char*>& requiredSurfaceExtensions,
		const std::vector<const char*>& additionalRequiredExtensions,
		const std::vector<const char*>& optionalExtensions
);
std::vector<const char*> queryDeviceExtensions(
		VkPhysicalDevice physicalDevice,
		const std::vector<const char*>& extensions);
