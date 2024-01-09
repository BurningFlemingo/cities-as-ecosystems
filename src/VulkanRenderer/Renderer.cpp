#define GLM_FORCE_DEPTH_ZERO_TO_ONE	 // opengl depth is -1:1, vulkan is 0:1,
									 // this makes it 0:1
#define GLM_ENABLE_EXPERIMENTAL
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "Renderer.h"

#include "RendererPCH.h"
#include "SDL2/SDL.h"
#include "debug/Debug.h"

#include <array>

// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtx/hash.hpp>

// #include "tiny_obj_loader/tiny_obj_loader.h"

// #include "utils/FileIO.h"

#include "vkutils/Commands.h"
#include "Context.h"
#include "DefaultCreateInfos.h"
#include "Device.h"
#include "Extensions.h"
#include "ImGuiIntegration.h"
#include "Image.h"
#include "Instance.h"
#include "State.h"
#include "Swapchain.h"
#include "vkutils/Synchronization.h"

#include <imgui_impl_vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <memory>

namespace {
	using namespace VulkanRenderer;

	struct VulkanRendererState {
		DeletionQueue rendererDeletionQueue;
		VulkanContext context;
		VulkanState state;
		int currentFrameIndex;
	};
	VulkanRendererState *s_RendererInfo{};
}  // namespace

void VulkanRenderer::init(SDL_Window *window) {
	assertFatal(s_RendererInfo == nullptr);

	DeletionQueue rendererDeletionQueue;
	VulkanContext context{ createVulkanContext(window, rendererDeletionQueue) };
	VulkanState state{
		createVulkanState(context, window, rendererDeletionQueue)
	};

	initImGui(context, state, window, rendererDeletionQueue);

	s_RendererInfo =
		new VulkanRendererState{ .rendererDeletionQueue =
									 std::move(rendererDeletionQueue),
								 .context = std::move(context),
								 .state = std::move(state) };
}

void VulkanRenderer::renderFrame(SDL_Window *window) {
	assertFatal(s_RendererInfo != nullptr);

	updateImGui();

	const VulkanContext &ctx{ s_RendererInfo->context };
	const VulkanState &state{ s_RendererInfo->state };

	const PerFrameVulkanState &frame{
		state.frames[s_RendererInfo->currentFrameIndex]
	};

	vkWaitForFences(
		ctx.device.logical, 1, &frame.fenceRenderFinished, VK_TRUE, UINT64_MAX
	);
	vkResetFences(ctx.device.logical, 1, &frame.fenceRenderFinished);

	uint32_t swapchainImageIndex{};
	{
		VkResult res{ vkAcquireNextImageKHR(
			ctx.device.logical,
			state.swapchain.handle,
			UINT64_MAX,
			frame.semFrameAvaliable,
			VK_NULL_HANDLE,
			&swapchainImageIndex
		) };

		if (res != VK_SUCCESS) {
			std::cout << "uh oh" << std::endl;
		}
	}
	vkResetCommandBuffer(frame.commandBuffer, 0);

	VkCommandBufferBeginInfo cmdBeginInfo{ vkdefaults::commandBufferBeginInfo(
	) };
	vkBeginCommandBuffer(frame.commandBuffer, &cmdBeginInfo);

	{
		VkImage swapchainImage{ state.swapchain.images[swapchainImageIndex] };

		vkutils::cmdTransitionImage(
			ctx,
			frame.commandBuffer,
			state.drawImage.handle,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			ctx.device.queueFamilyIndices.graphicsIndex,
			ctx.device.queueFamilyIndices.graphicsIndex
		);

		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			state.gradientPipeLayout,
			0,
			1,
			&state.imageDescriptorSet,
			0,
			nullptr
		);
		vkCmdBindPipeline(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			state.gradientPipeline
		);

		struct PushConstants {
			glm::vec4 d1;
			glm::vec4 d2;
			glm::vec4 d3;
			glm::vec4 d4;
		};

		PushConstants constants{
			.d1 = { 1.0, 1.0, 1.0, 1.0 },
			.d2 = { 0.0, 0.0, 1.0, 1.0 },
		};

		vkCmdPushConstants(
			frame.commandBuffer,
			state.gradientPipeLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(constants),
			&constants
		);

		vkCmdDispatch(
			frame.commandBuffer,
			state.swapchain.extent.width / 16 + 1,
			state.swapchain.extent.height / 16 + 1,
			1
		);

		vkutils::cmdTransitionImage(
			ctx,
			frame.commandBuffer,
			state.drawImage.handle,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			ctx.device.queueFamilyIndices.graphicsIndex,
			ctx.device.queueFamilyIndices.graphicsIndex
		);

		vkutils::cmdTransitionImage(
			ctx,
			frame.commandBuffer,
			swapchainImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			ctx.device.queueFamilyIndices.graphicsIndex,
			ctx.device.queueFamilyIndices.graphicsIndex
		);

		VkExtent3D swapchainExtent{ .width = state.swapchain.extent.width,
									.height = state.swapchain.extent.height,
									.depth = 1 };
		VkImageBlit2 blitRegion{
			vkdefaults::blitRegion(state.drawImage.extent, swapchainExtent)
		};

		VkBlitImageInfo2 blitInfo{
			.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
			.srcImage = state.drawImage.handle,
			.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.dstImage = swapchainImage,
			.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.regionCount = 1,
			.pRegions = &blitRegion,
			.filter = VK_FILTER_LINEAR,
		};

		vkCmdBlitImage2(frame.commandBuffer, &blitInfo);

		vkutils::cmdTransitionImage(
			ctx,
			frame.commandBuffer,
			swapchainImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			ctx.device.queueFamilyIndices.graphicsIndex,
			ctx.device.queueFamilyIndices.presentationIndex
		);

		cmdRenderImGui(
			ctx,
			state.swapchain.extent,
			frame.commandBuffer,
			state.swapchain.imageViews[swapchainImageIndex]
		);

		vkutils::cmdTransitionImage(
			ctx,
			frame.commandBuffer,
			swapchainImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			ctx.device.queueFamilyIndices.graphicsIndex,
			ctx.device.queueFamilyIndices.presentationIndex
		);
	}

	vkEndCommandBuffer(frame.commandBuffer);

	std::array<VkCommandBufferSubmitInfo, 1> cmdBufferInfo{
		vkdefaults::cmdBufferSubmitInfo(frame.commandBuffer)
	};
	std::array<VkSemaphoreSubmitInfo, 1> semWaitInfo{ vkdefaults::semSubmitInfo(
		frame.semFrameAvaliable, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
	) };
	std::array<VkSemaphoreSubmitInfo, 1> semSignalInfo{
		vkdefaults::semSubmitInfo(
			frame.semRenderFinished,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
		)
	};

	VkSubmitInfo2 submitInfo{
		vkdefaults::submitInfo(cmdBufferInfo, semWaitInfo, semSignalInfo)
	};
	if (vkQueueSubmit2(
			state.graphicsQueue, 1, &submitInfo, frame.fenceRenderFinished
		) != VK_SUCCESS) {
		logWarning("main render queue submit failed");
	}

	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame.semRenderFinished,
		.swapchainCount = 1,
		.pSwapchains = &state.swapchain.handle,
		.pImageIndices = &swapchainImageIndex,
	};

	{
		VkResult res{
			vkQueuePresentKHR(state.presentationQueue, &presentInfo)
		};
		if (res != VK_SUCCESS) {
			logWarning("present uh oh");
		}
	}

	s_RendererInfo->currentFrameIndex =
		(s_RendererInfo->currentFrameIndex + 1) %
		VulkanState::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::cleanup() {
	assertFatal(s_RendererInfo != nullptr);

	vkDeviceWaitIdle(s_RendererInfo->context.device.logical);
	s_RendererInfo->rendererDeletionQueue.flush();

	delete (s_RendererInfo);
}
