#pragma once

#include "pch.h"

std::vector<const char*> getSurfaceExtensions(SDL_Window* window);
std::vector<const char*> queryInstanceExtensions(const std::vector<const char*>& extensions);
std::vector<const char*> queryDeviceExtensions(
		VkPhysicalDevice physicalDevice,
		const std::vector<const char*>& extensions);
