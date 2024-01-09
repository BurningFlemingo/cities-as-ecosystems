#include "VulkanRenderer/RendererPCH.h"
#include "ShaderLayout.h"

#include "VulkanRenderer/Context.h"
#include "debug/Debug.h"
#include "VulkanRenderer/Shader.h"

#include <vector>
#include <unordered_map>

namespace {
	std::vector<VkDescriptorPoolSize> createDescriptorPoolSizes(
		const std::span<const vkcore::DescriptorBindingInfo>& descriptorInfos
	);

	std::vector<VkDescriptorPoolSize> createDescriptorPoolSizes(
		const std::span<const vkcore::DescriptorSetLayoutInfo>& shaderLayoutInfo
	);

	VkDescriptorPool baseCreateDescriptorPool(
		const VulkanContext& ctx,
		const std::span<const VkDescriptorPoolSize>& descriptorSizes,
		const uint32_t maxSets,
		DeletionQueue& deletionQueue
	);
}  // namespace

vkcore::ShaderLayout vkcore::createShaderLayout(
	const VulkanContext& ctx,
	const ShaderInputInfo& inputInfo,
	const VkShaderStageFlags stage,
	DeletionQueue& deletionQueue
) {
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		createDescriptorSetLayouts(
			ctx,
			inputInfo.layoutInfo.descriptorSetLayoutInfos,
			stage,
			deletionQueue
		)
	};

	ShaderLayout layout{
		.shaderDescriptorLayout = descriptorSetLayouts,
		.pushConstants = inputInfo.layoutInfo.pushConstantRanges,
	};

	return layout;
}

VkDescriptorPool vkcore::createDescriptorPool(
	const VulkanContext& ctx,
	const std::span<const DescriptorBindingInfo>& descriptorInfos,
	const uint32_t maxSets,
	DeletionQueue& deletionQueue
) {
	std::vector<VkDescriptorPoolSize> descriptorSizes{
		createDescriptorPoolSizes(descriptorInfos)
	};
	VkDescriptorPool pool{
		baseCreateDescriptorPool(ctx, descriptorSizes, maxSets, deletionQueue)
	};

	return pool;
}

VkDescriptorPool vkcore::createDescriptorPool(
	const VulkanContext& ctx,
	const std::span<const DescriptorSetLayoutInfo>& shaderDescriptorInfo,
	const uint32_t maxSets,
	DeletionQueue& deletionQueue
) {
	std::vector<VkDescriptorPoolSize> descriptorSizes{
		createDescriptorPoolSizes(shaderDescriptorInfo)
	};
	VkDescriptorPool pool{
		baseCreateDescriptorPool(ctx, descriptorSizes, maxSets, deletionQueue)
	};

	return pool;
}

std::vector<VkDescriptorSetLayout> vkcore::createDescriptorSetLayouts(
	const VulkanContext& ctx,
	const std::span<const DescriptorSetLayoutInfo>& shaderDescriptorInfo,
	const VkShaderStageFlags stages,
	DeletionQueue& deletionQueue
) {
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(shaderDescriptorInfo.size());

	for (auto& setInfo : shaderDescriptorInfo) {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.reserve(setInfo.size());

		for (auto& bindingInfo : setInfo) {
			VkDescriptorSetLayoutBinding bindingLayout{
				.binding = bindingInfo.binding,
				.descriptorType = bindingInfo.type,
				.descriptorCount = bindingInfo.count,
				.stageFlags = stages
			};
			bindings.emplace_back(bindingLayout);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data(),
		};

		VkDescriptorSetLayout layout{};
		if (vkCreateDescriptorSetLayout(
				ctx.device.logical, &descriptorSetLayoutInfo, nullptr, &layout
			) != VK_SUCCESS) {
			logFatal("couldnt create descriptor set layout");
		}

		layouts.emplace_back(layout);
	}

	auto deleter{ [=]() {
		for (auto& layout : layouts) {
			vkDestroyDescriptorSetLayout(ctx.device.logical, layout, nullptr);
		}
	} };
	deletionQueue.pushFunction(deleter);

	return layouts;
}

std::vector<VkDescriptorSet> vkcore::allocateDescriptorSets(
	const VulkanContext& ctx,
	const std::span<const VkDescriptorSetLayout>& descriptorSetLayouts,
	const VkDescriptorPool pool
) {
	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = (uint32_t)descriptorSetLayouts.size(),
		.pSetLayouts = descriptorSetLayouts.data(),
	};

	std::vector<VkDescriptorSet> sets(descriptorSetLayouts.size());
	if (vkAllocateDescriptorSets(ctx.device.logical, &allocInfo, sets.data()) !=
		VK_SUCCESS) {
		logFatal("could not create descriptor sets");
	}

	return sets;
}

VkDescriptorSet vkcore::allocateDescriptorSets(
	const VulkanContext& ctx,
	const VkDescriptorSetLayout descriptorSetLayout,
	const VkDescriptorPool pool
) {
	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorSetLayout,
	};

	VkDescriptorSet set{};
	if (vkAllocateDescriptorSets(ctx.device.logical, &allocInfo, &set) !=
		VK_SUCCESS) {
		logFatal("could not create descriptor sets");
	}

	return set;
}

namespace {
	std::vector<VkDescriptorPoolSize> createDescriptorPoolSizes(
		const std::span<const vkcore::DescriptorBindingInfo>& descriptorInfos
	) {
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(descriptorInfos.size());

		for (const auto& info : descriptorInfos) {
			VkDescriptorPoolSize descriptorSize{
				.type = info.type,
				.descriptorCount = info.count,
			};
			sizes.emplace_back(descriptorSize);
		}

		return sizes;
	}

	std::vector<VkDescriptorPoolSize> createDescriptorPoolSizes(
		const std::span<const vkcore::DescriptorSetLayoutInfo>& shaderLayoutInfo
	) {
		std::vector<VkDescriptorPoolSize> sizes;

		for (const auto& setInfo : shaderLayoutInfo) {
			sizes.reserve(setInfo.size());

			for (auto& info : setInfo) {
				VkDescriptorPoolSize descriptorSize{
					.type = info.type,
					.descriptorCount = info.count,
				};
				sizes.emplace_back(descriptorSize);
			}
		}

		return sizes;
	}

	VkDescriptorPool baseCreateDescriptorPool(
		const VulkanContext& ctx,
		const std::span<const VkDescriptorPoolSize>& descriptorSizes,
		const uint32_t maxSets,
		DeletionQueue& deletionQueue
	) {
		VkDescriptorPoolCreateInfo descriptorPoolInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = maxSets,
			.poolSizeCount = (uint32_t)descriptorSizes.size(),
			.pPoolSizes = descriptorSizes.data(),
		};

		VkDescriptorPool pool{};
		if (vkCreateDescriptorPool(
				ctx.device.logical, &descriptorPoolInfo, nullptr, &pool
			) != VK_SUCCESS) {
			logFatal("couldnt create descriptor pool");
		}

		auto deleter{ [=]() {
			vkDestroyDescriptorPool(ctx.device.logical, pool, nullptr);
		} };
		deletionQueue.pushFunction(deleter);

		return pool;
	}
}  // namespace
