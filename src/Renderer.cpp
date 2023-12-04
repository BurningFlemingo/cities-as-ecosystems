#include "Renderer.h"

#include <iostream>

#include "debug/Debug.h"

#include <optional>
#include <fstream>
#include <chrono>
#include <limits.h>
#include <assert.h>

void VulkanRenderer::createInstance(SDL_Window* window) {
	VkUtils::Instance instance{VkUtils::createInstance(window)};

}
