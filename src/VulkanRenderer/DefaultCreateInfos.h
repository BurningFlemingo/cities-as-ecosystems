#pragma once

#include <span>
#include <vulkan/vulkan.h>

namespace vkdefaults {
	VkImageSubresourceRange
		subresourceRange(const VkImageAspectFlags aspectFlags);

	VkCommandBufferBeginInfo
		commandBufferBeginInfo(const VkCommandBufferUsageFlags flags = 0);

	VkSemaphoreSubmitInfo semSubmitInfo(
		const VkSemaphore semaphore, const VkPipelineStageFlags2 stage
	);
	VkCommandBufferSubmitInfo
		cmdBufferSubmitInfo(const VkCommandBuffer cmdBuffer);

	VkSubmitInfo2 submitInfo(
		const std::span<const VkCommandBufferSubmitInfo>& cmdBufferInfos,
		const std::span<const VkSemaphoreSubmitInfo>& semWaitInfos,
		const std::span<const VkSemaphoreSubmitInfo>& semSignalInfos
	);

	VkImageCreateInfo imageCreateInfo(
		const VkExtent3D& extent,
		const VkFormat format,
		const VkImageUsageFlags usage
	);

	VkImageViewCreateInfo imageViewCreateInfo(
		const VkImage image,
		const VkFormat format,
		const VkImageAspectFlags aspect
	);

	VkImageBlit2
		blitRegion(const VkExtent3D srcExtent, const VkExtent3D dstExtent);
}  // namespace vkdefaults
