#pragma once

// forward declerations
struct VulkanContext;
class DeletionQueue;

typedef SDL_Window SDL_Window;

namespace VulkanRenderer {
	struct VulkanState;
}

namespace VulkanRenderer {
	void initImGui(
			const VulkanContext& ctx,
			const VulkanState& state,
			SDL_Window* window,
			DeletionQueue& deletionQueue
		);

	void updateImGui();

	void cmdRenderImGui(
			const VulkanContext& ctx, 
			const VkExtent2D& renderExtent, 
			const VkCommandBuffer cmdBuffer,
			const VkImageView dstImageView
		);
}
