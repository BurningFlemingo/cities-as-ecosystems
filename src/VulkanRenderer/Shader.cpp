#include "RendererPCH.h"
#include "Shader.h"
#include "utils/FileIO.h"
#include "Context.h"
#include "Cleanup.h"
#include "Pipelines.h"

#include "debug/Debug.h"

#include <unordered_map>
#include <string>

#include <spirv_reflect.h>

using namespace vkcore;

namespace {
	const std::unordered_map<std::string, VkShaderStageFlags>
		extensionToShaderStageMap{ { ".comp", VK_SHADER_STAGE_COMPUTE_BIT } };

	ShaderSourceInfo readShader(const std::filesystem::path& shaderPath);
	ShaderLayoutInfo parseShaderLayoutInfo(const ShaderSourceInfo& shaderSrcInfo
	);
}  // namespace

SharedShaderObjects vkcore::createSharedShaderObjects(
	const VulkanContext& ctx,
	const ShaderInputInfo& inputInfo,
	const VkShaderStageFlags stage,
	DeletionQueue& deletionQueue
) {
	VkDescriptorPool pool{ createDescriptorPool(
		ctx,
		inputInfo.layoutInfo.descriptorSetLayoutInfos,
		inputInfo.layoutInfo.descriptorSetLayoutInfos.size(),
		deletionQueue
	) };

	ShaderLayout layout{
		createShaderLayout(ctx, inputInfo, stage, deletionQueue)
	};

	SharedShaderObjects shaderObjects{ .descriptorPool = pool,
									   .layout = layout };

	return shaderObjects;
}

UniqueShaderObjects vkcore::createUniqueShaderObjects(
	const VulkanContext& ctx,
	const ShaderSourceInfo& sourceInfo,
	const SharedShaderObjects sharedShaderObjects
) {
	std::vector<VkDescriptorSet> descriptorSets{ allocateDescriptorSets(
		ctx,
		sharedShaderObjects.layout.shaderDescriptorLayout,
		sharedShaderObjects.descriptorPool
	) };

	UniqueShaderObjects uniqueShaderObjects{
		.descriptorSets = descriptorSets,
		.sourceInfo = sourceInfo,
	};

	return uniqueShaderObjects;
}

// have it use default shaders if a shader isnt found
std::vector<ShaderInfo> vkcore::parseShaders(
	const std::span<const std::filesystem::path>& shaderPaths
) {
	std::vector<ShaderInfo> shaderInfos;

	for (auto& path : shaderPaths) {
		ShaderSourceInfo sourceInfo{ readShader(path) };

		// TODO: change to default shader
		ShaderInfo shaderInfo{};

		if (sourceInfo.spv.data()) {
			ShaderInputInfo inputInfo{ .layoutInfo =
										   parseShaderLayoutInfo(sourceInfo) };

			shaderInfo = { .sourceInfo = sourceInfo, .inputInfo = inputInfo };
		}

		shaderInfos.emplace_back(shaderInfo);
	}

	return shaderInfos;
}

std::vector<VkPipelineShaderStageCreateInfo> vkcore::createShaderStages(
	const VulkanContext& ctx,
	const std::span<const ShaderSourceInfo>& shaderSourceInfos,
	DeletionQueue& deletionQueue
) {
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
	shaderStageCreateInfos.reserve(shaderSourceInfos.size());

	for (auto& info : shaderSourceInfos) {
		VkShaderModuleCreateInfo moduleInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = (uint32_t)info.spv.size(),
			.pCode = (uint32_t*)info.spv.data(),
		};

		VkShaderModule shaderModule{};
		if (vkCreateShaderModule(
				ctx.device.logical, &moduleInfo, nullptr, &shaderModule
			) != VK_SUCCESS) {
			logWarning("could not compile shader");
		}

		VkPipelineShaderStageCreateInfo shaderStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = (VkShaderStageFlagBits)info.stage,
			.module = shaderModule,
			.pName = "main",
		};

		shaderStageCreateInfos.emplace_back(shaderStageInfo);
	}

	auto deleter{ ([=]() {
		for (auto& stage : shaderStageCreateInfos) {
			vkDestroyShaderModule(ctx.device.logical, stage.module, nullptr);
		}
	}) };

	deletionQueue.pushFunction(deleter);

	return shaderStageCreateInfos;
}

namespace {
	ShaderLayoutInfo parseShaderLayoutInfo(const ShaderSourceInfo& shaderSrcInfo
	) {
		SpvReflectResult res{};

		SpvReflectShaderModule reflectModule{};
		res = spvReflectCreateShaderModule(
			shaderSrcInfo.spv.size(), shaderSrcInfo.spv.data(), &reflectModule
		);
		assertWarning(res == SPV_REFLECT_RESULT_SUCCESS);

		uint32_t inSetsCount{};
		res = spvReflectEnumerateDescriptorSets(
			&reflectModule, &inSetsCount, nullptr
		);
		assertWarning(res == SPV_REFLECT_RESULT_SUCCESS);

		std::vector<SpvReflectDescriptorSet*> sets(inSetsCount);
		res = spvReflectEnumerateDescriptorSets(
			&reflectModule, &inSetsCount, sets.data()
		);
		assertWarning(res == SPV_REFLECT_RESULT_SUCCESS);

		uint32_t pushConstantCount{};
		res = spvReflectEnumeratePushConstantBlocks(
			&reflectModule, &pushConstantCount, nullptr
		);
		assertWarning(res == SPV_REFLECT_RESULT_SUCCESS);

		std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
		res = spvReflectEnumeratePushConstantBlocks(
			&reflectModule, &pushConstantCount, pushConstants.data()
		);
		assertWarning(res == SPV_REFLECT_RESULT_SUCCESS);

		int nSets{ (int)sets.size() };

		ShaderLayoutInfo layoutInfo;
		layoutInfo.descriptorSetLayoutInfos.reserve(nSets);

		for (size_t setIndex{}; setIndex < nSets; setIndex++) {
			DescriptorSetLayoutInfo setLayout(sets[setIndex]->binding_count);

			for (size_t bindingIndex{};
				 bindingIndex < sets[setIndex]->binding_count;
				 bindingIndex++) {
				auto& binding{ sets[setIndex]->bindings[bindingIndex] };

				setLayout[bindingIndex] = { .binding = binding->binding,
											.type = (VkDescriptorType
											)binding->descriptor_type,
											.count = binding->count };
			}

			layoutInfo.descriptorSetLayoutInfos.emplace_back(setLayout);
		}

		layoutInfo.pushConstantRanges.reserve(pushConstantCount);
		for (const auto& pushConstant : pushConstants) {
			VkPushConstantRange range{
				.stageFlags = shaderSrcInfo.stage,
				.offset = pushConstant->offset,
				.size = pushConstant->size,
			};

			layoutInfo.pushConstantRanges.emplace_back(range);
		}

		spvReflectDestroyShaderModule(&reflectModule);

		return layoutInfo;
	}

	ShaderSourceInfo readShader(const std::filesystem::path& shaderPath) {
		bool fileExists{ std::filesystem::exists(shaderPath) };
		if (!fileExists) {
			logWarning(
				"could not find ", std::filesystem::absolute(shaderPath)
			);
			return {};
		}

		// stemed to remove .spv
		std::string extension{ shaderPath.stem().extension().string() };

		auto shaderStageItt{ extensionToShaderStageMap.find(extension) };
		if (shaderStageItt == extensionToShaderStageMap.end()) {
			logWarning("invalid shader extension");
			return {};
		}

		ShaderSourceInfo info{
			.spv = readFile(shaderPath.string()),
			.stage = shaderStageItt->second,
		};
		return info;
	}
}  // namespace
