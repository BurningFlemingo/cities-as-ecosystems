#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <stdint.h>
#include <vector>
#include <algorithm>

constexpr uint32_t WINDOW_HEIGHT{1080 / 2};
constexpr uint32_t WINDOW_WIDTH{1920 / 2};

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		std::cerr << "could not init sdl: " << SDL_GetError() << std::endl;
	}

	SDL_Window* window{SDL_CreateWindow("vulkan :D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN)};
	if (!window) {
		std::cerr << "window could not be created: " << SDL_GetError() << std::endl;
	}

	VkInstance instance;
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vulkan :D";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "VulkEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		uint32_t SDLExtensionCount{};
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, nullptr);

		std::vector<const char*> SDLExtensions(SDLExtensionCount);
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions.data());

		std::vector<const char*> requiredExtensions;
		requiredExtensions.reserve(SDLExtensionCount);
		for (const auto& extension : SDLExtensions) {
			requiredExtensions.emplace_back(extension);
		}

		uint32_t avaliableExtensionCount{};
		vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, nullptr);

		std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, avaliableExtensions.data());

		std::vector<const char*> enabledExtensions;
		for (const auto extension : avaliableExtensions) {
			auto itt{
				std::find_if(
						requiredExtensions.begin(),
						requiredExtensions.end(),
						[&](const auto& requiredExtension) { return strcmp(requiredExtension, extension.extensionName) == 0; }
						)
			};

			if (itt != avaliableLayers.end()) {
				enabledLayers.emplace_back(validationLayer);
			}
		}

		std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

		uint32_t layerCount{};
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> avaliableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

		std::vector<const char*> enabledLayers;
		for (const auto& validationLayer : validationLayers) {
			auto itt{
				std::find_if(
						avaliableLayers.begin(),
						avaliableLayers.end(),
						[&](const auto& layer) { return strcmp(validationLayer, layer.layerName) == 0; }
						)
			};

			if (itt != avaliableLayers.end()) {
				enabledLayers.emplace_back(validationLayer);
			}
		}
		if (enabledLayers.size() < validationLayers.size()) {
			std::cout << "not all validation layers avaliable: " << enabledLayers.size() << " of " << validationLayers.size() << std::endl;
		}

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = requiredExtensions.size();
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.enabledLayerCount = enabledLayers.size();
		createInfo.ppEnabledLayerNames = enabledLayers.data();

		if(vkCreateInstance(&createInfo, nullptr, &instance)) {
			std::cerr << "could not create vulkan instance" << std::endl;
		}
	}

	SDL_Event e;
	bool running{true};
	while (running) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
						running = false;
					}
				default:
					break;
			}
		}
	}

	vkDestroyInstance(instance, nullptr);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
