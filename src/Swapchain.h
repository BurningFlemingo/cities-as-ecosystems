#pragma once

#include "pch.h"

#include "Device.h"

struct Swapchain {
	VkSwapchainKHR handle;

	VkPresentModeKHR presentMode{};
	VkColorSpaceKHR colorSpace{};
	VkFormat format{};
	VkExtent2D extent{};
};

Swapchain createSwapchain(
		const Device& device,
		SDL_Window* window,
		VkSurfaceKHR surface
	);

Swapchain recreateSwapchain(const Device& device,
		SDL_Window* window,
		VkSurfaceKHR surface,
		VkSwapchainKHR oldSwapchainHandle
	);

std::vector<VkImageView> createSwapchainImageViews(
		const Device& device,
		const Swapchain& swapchain
	);

std::vector<VkFramebuffer> createSwapchainFramebuffers(
		const Device& device,
		const std::vector<VkImageView>& swapchainImageViews,
		const VkExtent2D swapchainExtent,
		const VkRenderPass renderPass
	);

void destroySwapchainFramebuffers(VkDevice device, std::vector<VkFramebuffer>* framebuffers);
void destroySwapchainImageViews(VkDevice device, std::vector<VkImageView>* imageViews, std::vector<VkFramebuffer>* framebuffers);
void destroySwapchain(
		const Device& device,
		VkSwapchainKHR swapchainHandle,
		std::vector<VkImageView>* imageViews,
		std::vector<VkFramebuffer>* framebuffers
	);
