#include "CAEngine.h"

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <optional>
#include <fstream>
#include <chrono>
#include <limits.h>
#include <functional>

#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include "vulkanRenderer/Renderer.h"

#include "debug/Logging.h"
#include "debug/Assertions.h"

namespace {
	constexpr int WINDOW_WIDTH{ 1920 / 2 };
	constexpr int WINDOW_HEIGHT{ 1080 / 2 };

	struct KeyState {
		std::vector<SDL_Scancode> keysPressed;
		std::vector<SDL_Scancode> keysLifted;
	};

	struct AppState {
		SDL_Window* window;
		KeyState keyState;
	};

	AppState* s_AppState{};
}  // namespace

void CAEngine::startup() {
	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		std::cerr << "could not init sdl: " << SDL_GetError() << std::endl;
	}

	SDL_Window* window{ SDL_CreateWindow(
		"vulkan :D",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	) };
	assertFatal(window, "window could not be created: ", SDL_GetError());

	ImGui::CreateContext();

	auto startTime{ std::chrono::high_resolution_clock::now() };
	auto fpsTime{ startTime };

	VulkanRenderer::init(window);

	auto currentTime{ std::chrono::high_resolution_clock::now() };
	float duration{
		std::chrono::duration<float, std::chrono::milliseconds::period>(
			currentTime - fpsTime
		)
			.count()
	};

	fpsTime = currentTime;
	std::cout << "vulkan init: " << duration << "ms" << std::endl;

	s_AppState = new AppState{ .window = window };
}

void CAEngine::run() {
	AppState& state{ *s_AppState };

	SDL_Event event{};
	bool running{ true };

	int framesRendered{};

	auto frameStartTime{ std::chrono::high_resolution_clock::now() };
	while (running) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.scancode == SDL_SCANCODE_TAB) {
						running = false;
					}
					break;
				case SDL_WINDOWEVENT_RESIZED:
				default:
					break;
			}

			ImGui_ImplSDL2_ProcessEvent(&event);
		}

		ImGui_ImplSDL2_NewFrame();
		VulkanRenderer::renderFrame(state.window);
		framesRendered++;

		auto frameEndTime{ std::chrono::high_resolution_clock::now() };
		float duration{
			std::chrono::duration<float, std::chrono::seconds::period>(
				frameEndTime - frameStartTime
			)
				.count()
		};
		if (duration >= 1) {
			frameStartTime = frameEndTime;
			std::cout << "frames renderered: " << framesRendered
					  << " | ms per frame: " << (1.f / framesRendered) * 1000
					  << std::endl;
			framesRendered = 0;
		}
	}
}

void CAEngine::shutdown() {
	AppState& state{ *s_AppState };

	VulkanRenderer::cleanup();
	SDL_DestroyWindow(state.window);

	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	SDL_Quit();
}
