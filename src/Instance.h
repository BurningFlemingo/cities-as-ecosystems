#pragma once
#include "pch.h"

struct Instance {
	VkInstance handle{};
	VkDebugUtilsMessengerEXT debugMessenger{};
};

Instance createInstance(SDL_Window* window);
