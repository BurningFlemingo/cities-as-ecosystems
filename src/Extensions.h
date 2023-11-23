#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vector>

std::vector<const char*> getSurfaceExtensions(SDL_Window* window);
std::vector<const char*> queryInstanceExtensions(const std::vector<const char*>& extensions);
std::vector<const char*> queryDeviceExtensions(
		VkPhysicalDevice physicalDevice,
		const std::vector<const char*>& extensions);
