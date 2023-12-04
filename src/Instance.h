#pragma once
#include "pch.h"

namespace VkUtils {

struct Instance {
	VkInstance handle{};
	VkDebugUtilsMessengerEXT debugMessenger{};
};

Instance createInstance(SDL_Window* window);

}

