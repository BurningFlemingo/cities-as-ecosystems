#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#include <vector>
#include <stdint.h>

#include "Device.h"

struct Swapchain {
	VkSwapchainKHR handle;

	VkPresentModeKHR presentMode{};
	VkColorSpaceKHR colorSpace{};
	VkFormat format{};
	VkExtent2D extent{};
};

Swapchain createSwapchain(
		VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails);

Swapchain recreateSwapchain(VkDevice device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		const SurfaceSupportDetails& surfaceSupportDetails, 
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
