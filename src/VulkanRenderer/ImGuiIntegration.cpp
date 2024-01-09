#include "RendererPCH.h"
#include "ImGuiIntegration.h"

#include "State.h"
#include "Context.h"
#include "vkutils/Commands.h"
#include "debug/Debug.h"
#include "Cleanup.h"

#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <array>

void VulkanRenderer::initImGui(
	const VulkanContext& ctx,
	const VulkanState& state,
	SDL_Window* window,
	DeletionQueue& deletionQueue
) {
	// countes / types taken from imgui demo
	VkDescriptorPoolSize imguiPoolSizes[]{
		{ .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		  .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
		  .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
		  .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		  .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		  .descriptorCount = 1000 },
		{ .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		  .descriptorCount = 1000 },
	};
	VkDescriptorPoolCreateInfo imguiPoolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = std::size(imguiPoolSizes),
		.pPoolSizes = imguiPoolSizes,
	};

	VkDescriptorPool imguiPool{};
	if (vkCreateDescriptorPool(
			ctx.device.logical, &imguiPoolInfo, nullptr, &imguiPool
		) != VK_SUCCESS) {
		logWarning("could not create imgui descriptor pool");
	}

	ImGui_ImplVulkan_InitInfo imguiInfo{
		.Instance = ctx.instance,
		.PhysicalDevice = ctx.device.physical,
		.Device = ctx.device.logical,
		.QueueFamily = ctx.device.queueFamilyIndices.graphicsIndex,
		.Queue = state.graphicsQueue,
		.DescriptorPool = imguiPool,
		.MinImageCount = 3,
		.ImageCount = 3,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.UseDynamicRendering = VK_TRUE,
		.ColorAttachmentFormat = state.swapchain.format,
	};

	ImGui_ImplSDL2_InitForVulkan(window);
	ImGui_ImplVulkan_Init(&imguiInfo, nullptr);

	vkutils::immediateSubmit(
		ctx,
		state.immediateCommandBuffer,
		state.transferQueue,
		state.immediateFence,
		[]() {
			ImGui_ImplVulkan_CreateFontsTexture();
		}
	);

	ImGui_ImplVulkan_DestroyFontsTexture();
	deletionQueue.pushFunction([&]() {
		vkDestroyDescriptorPool(ctx.device.logical, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	});
}

void VulkanRenderer::updateImGui() {
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	// doesnt actually render, only readys data
	ImGui::Render();
}

void VulkanRenderer::cmdRenderImGui(
	const VulkanContext& ctx,
	const VkExtent2D& renderExtent,
	const VkCommandBuffer cmdBuffer,
	const VkImageView dstImageView
) {
	VkRenderingAttachmentInfo colorAttachmentInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = dstImageView,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	};
	VkRenderingInfo renderInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
								.renderArea = { .extent = renderExtent },
								.layerCount = 1,
								.colorAttachmentCount = 1,
								.pColorAttachments = &colorAttachmentInfo };
	vkCmdBeginRendering(cmdBuffer, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

	vkCmdEndRendering(cmdBuffer);
}
