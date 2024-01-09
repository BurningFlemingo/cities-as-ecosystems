#pragma once

typedef struct SDL_Window SDL_Window;

namespace VulkanRenderer {
	void init(SDL_Window* window);
	void renderFrame(SDL_Window* window);
	void cleanup();
};	// namespace VulkanRenderer
