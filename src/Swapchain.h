#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#include <vector>
#include <stdint.h>

struct Swapchain {
	VkSwapchainKHR handle;

	VkPresentModeKHR presentMode{};
	VkColorSpaceKHR colorSpace{};
	VkFormat format{};
	VkExtent2D extent{};
};

struct SurfaceSupportDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> surfaceFormats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

Swapchain createSwapchain(
		VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails, 
		const std::vector<uint32_t>& queueFamilyIndices);

std::vector<VkImageView> createSwapchainImageViews(
		VkDevice device,
		const Swapchain& swapchain);

std::vector<VkFramebuffer> createSwapchainFramebuffers(
		VkDevice device,
		const std::vector<VkImageView>& swapchainImageViews,
		const VkExtent2D swapchainExtent,
		const VkRenderPass renderPass);
