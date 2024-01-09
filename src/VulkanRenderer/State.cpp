#include "RendererPCH.h"
#include "State.h"

#include "Context.h"
#include "vkutils/Synchronization.h"
#include "DefaultCreateInfos.h"
#include "vkcore/ShaderLayout.h"
#include "Pipelines.h"
#include "Shader.h"

#include "debug/Debug.h"
#include "utils/FileIO.h"

using namespace vkcore;

VulkanRenderer::VulkanState VulkanRenderer::createVulkanState(
	const VulkanContext& ctx, SDL_Window* window, DeletionQueue& deletionQueue
) {
	constexpr int MAX_FRAMES_IN_FLIGHT{ VulkanState::MAX_FRAMES_IN_FLIGHT };

	DeletionQueue swapchainDeletionQueue;
	Deletable<Swapchain> swapchain{ createSwapchain(ctx, window) };
	swapchainDeletionQueue.pushFunction(std::move(swapchain.deleter));

	Queues queues{ getDeviceQueues(ctx.device) };

	VkCommandPool frameCommandPool{};
	{
		VkCommandPoolCreateInfo poolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = ctx.device.queueFamilyIndices.graphicsIndex,
		};

		if (vkCreateCommandPool(
				ctx.device.logical, &poolCreateInfo, nullptr, &frameCommandPool
			) != VK_SUCCESS) {
			logFatal("could not create command pool");
		}

		deletionQueue.pushFunction([=]() {
			vkDestroyCommandPool(ctx.device.logical, frameCommandPool, nullptr);
		});
	}

	std::array<PerFrameVulkanState, MAX_FRAMES_IN_FLIGHT> frames;
	{
		std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBuffers;

		VkCommandBufferAllocateInfo cmdBuffAllocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = frameCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT,
		};

		if (vkAllocateCommandBuffers(
				ctx.device.logical, &cmdBuffAllocInfo, cmdBuffers.data()
			) != VK_SUCCESS) {
			logFatal("could not allocate command buffers");
		}

		for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
			PerFrameVulkanState& frame{ frames[i] };
			frame.commandBuffer = cmdBuffers[i];
			frame.fenceRenderFinished =
				vkutils::createFence(ctx, VK_FENCE_CREATE_SIGNALED_BIT);

			frame.semRenderFinished = vkutils::createSemaphore(ctx);
			frame.semFrameAvaliable = vkutils::createSemaphore(ctx);
		}

		deletionQueue.pushFunction([=]() {
			for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
				const PerFrameVulkanState& frame{ frames[i] };
				vkDestroyFence(
					ctx.device.logical, frame.fenceRenderFinished, nullptr
				);

				vkDestroySemaphore(
					ctx.device.logical, frame.semFrameAvaliable, nullptr
				);
				vkDestroySemaphore(
					ctx.device.logical, frame.semRenderFinished, nullptr
				);
			}
		});
	}

	Image drawImage{
		createDrawImage(ctx, swapchain.obj, swapchainDeletionQueue)
	};

	UniqueShaderObjects uniqueGradientShaderInfo{};
	SharedShaderObjects sharedGradientShaderInfo{};
	{
		std::array<std::filesystem::path, 1> shaderPaths{
			"shaders/second.comp.spv"
		};
		ShaderInfo gradientShaderInfo = parseShaders(shaderPaths)[0];

		sharedGradientShaderInfo = vkcore::createSharedShaderObjects(
			ctx,
			gradientShaderInfo.inputInfo,
			gradientShaderInfo.sourceInfo.stage,
			deletionQueue
		);
		uniqueGradientShaderInfo = vkcore::createUniqueShaderObjects(
			ctx, gradientShaderInfo.sourceInfo, sharedGradientShaderInfo
		);
	}

	{
		VkDescriptorImageInfo imageInfo{
			.imageView = drawImage.view,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};

		VkWriteDescriptorSet writeDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = uniqueGradientShaderInfo.descriptorSets[0],
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &imageInfo,
		};

		vkUpdateDescriptorSets(
			ctx.device.logical, 1, &writeDescriptorSet, 0, nullptr
		);
	}

	VkPipelineLayout gradientPipelineLayout{};
	VkPipeline gradientPipeline{};
	{
		gradientPipelineLayout = createPipelineLayout(
			ctx, sharedGradientShaderInfo.layout, deletionQueue
		);

		VkPipelineShaderStageCreateInfo shaderStageInfo{ createShaderStages(
			ctx,
			std::array{ uniqueGradientShaderInfo.sourceInfo },
			deletionQueue
		)[0] };

		VkComputePipelineCreateInfo pipeInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = shaderStageInfo,
			.layout = gradientPipelineLayout,
		};

		if (vkCreateComputePipelines(
				ctx.device.logical, 0, 1, &pipeInfo, nullptr, &gradientPipeline
			) != VK_SUCCESS) {
			logWarning("could not create compute pipelines");
		}

		deletionQueue.pushFunction([=]() {
			vkDestroyPipeline(ctx.device.logical, gradientPipeline, nullptr);
		});
	}

	VkCommandPool immediateCommandPool{};
	VkCommandBuffer immediateCommandBuffer{};
	VkFence immediateFence{};
	{
		VkCommandPoolCreateInfo immediateCommandPoolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = ctx.device.queueFamilyIndices.graphicsIndex
		};
		if (vkCreateCommandPool(
				ctx.device.logical,
				&immediateCommandPoolInfo,
				nullptr,
				&immediateCommandPool
			) != VK_SUCCESS) {
			logFatal("couldnt create immediate command pool");
		}

		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = immediateCommandPool,
			.commandBufferCount = 1
		};

		if (vkAllocateCommandBuffers(
				ctx.device.logical, &allocInfo, &immediateCommandBuffer
			) != VK_SUCCESS) {
			logFatal("couldnt allocate immediate command buffer");
		}

		VkFenceCreateInfo fenceInfo{ .sType =
										 VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									 .flags = VK_FENCE_CREATE_SIGNALED_BIT };

		if (vkCreateFence(
				ctx.device.logical, &fenceInfo, nullptr, &immediateFence
			) != VK_SUCCESS) {
			logFatal("couldnt create fence");
		}
	}

	VkGraphicsPipelineCreateInfo pipeInfo{};
	{
		DeletionQueue pipeTempDeletionQueue;
		std::array<std::filesystem::path, 2> shaderPaths{
			"shaders/first.vert.spv", "shaders/first.frag.spv"
		};

		std::vector<ShaderInfo> shaders{ parseShaders(shaderPaths) };

		for (const auto& shader : shaders) {
			SharedShaderObjects sharedShaderObjects{ createSharedShaderObjects(
				ctx, shader.inputInfo, shader.sourceInfo.stage, deletionQueue
			) };
		};

		VkPipelineShaderStageCreateInfo shaderStages{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,

		};
		pipeInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	}

	VulkanState state{
		.swapchainDeletionQueue = swapchainDeletionQueue,
		.swapchain = swapchain.obj,
		.drawImage = drawImage,

		.graphicsQueue = queues.graphicsQueue,
		.presentationQueue = queues.presentationQueue,
		.transferQueue = queues.transferQueue,

		.immediateCommandPool = immediateCommandPool,
		.immediateCommandBuffer = immediateCommandBuffer,
		.immediateFence = immediateFence,

		.frameCommandPool = frameCommandPool,
		.frames = std::move(frames),

		.mainDescriptorPool = sharedGradientShaderInfo.descriptorPool,
		.imageDescriptoreSetLayout =
			sharedGradientShaderInfo.layout.shaderDescriptorLayout[0],
		.imageDescriptorSet = uniqueGradientShaderInfo.descriptorSets[0],

		.gradientPipeline = gradientPipeline,
		.gradientPipeLayout = gradientPipelineLayout
	};
	return state;
}
