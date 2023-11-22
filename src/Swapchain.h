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

Swapchain recreateSwapchain(VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails, 
		const std::vector<uint32_t>& queueFamilyIndices,
		VkSwapchainKHR oldSwapchainHandle);

std::vector<VkImageView> createSwapchainImageViews(
		VkDevice device,
		const Swapchain& swapchain);

std::vector<VkFramebuffer> createSwapchainFramebuffers(
		VkDevice device,
		const std::vector<VkImageView>& swapchainImageViews,
		const VkExtent2D swapchainExtent,
		const VkRenderPass renderPass);

void destroySwapchainFramebuffers(VkDevice device, std::vector<VkFramebuffer>* framebuffers);
void destroySwapchainImageViews(VkDevice device, std::vector<VkImageView>* imageViews, std::vector<VkFramebuffer>* framebuffers);
void destroySwapchain(VkDevice device, VkSwapchainKHR swapchainHandle, std::vector<VkImageView>* imageViews, std::vector<VkFramebuffer>* framebuffers);
