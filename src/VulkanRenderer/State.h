#pragma once

#include <vulkan/vulkan.h>
#include <array>

#include "Swapchain.h"
#include "Context.h"
#include "Cleanup.h"
#include "Image.h"

namespace VulkanRenderer {

	struct PerFrameVulkanState {
		VkCommandBuffer commandBuffer;

		VkSemaphore semFrameAvaliable;
		VkSemaphore semRenderFinished;
		VkFence fenceRenderFinished;
	};

	struct VulkanState {
		static constexpr int MAX_FRAMES_IN_FLIGHT{ 2 };

		DeletionQueue swapchainDeletionQueue;
		Swapchain swapchain;

		Image drawImage;

		VkQueue graphicsQueue;
		VkQueue presentationQueue;
		VkQueue transferQueue;

		VkCommandPool immediateCommandPool{};
		VkCommandBuffer immediateCommandBuffer{};
		VkFence immediateFence{};

		VkCommandPool frameCommandPool;
		std::array<PerFrameVulkanState, MAX_FRAMES_IN_FLIGHT> frames;

		VkDescriptorPool mainDescriptorPool;
		VkDescriptorSetLayout imageDescriptoreSetLayout;
		VkDescriptorSet imageDescriptorSet;

		VkPipeline gradientPipeline;
		VkPipelineLayout gradientPipeLayout;
	};

	VulkanState createVulkanState(
		const VulkanContext& ctx,
		SDL_Window* window,
		DeletionQueue& deletionQueue
	);
}  // namespace VulkanRenderer
