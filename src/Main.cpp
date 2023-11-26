#define GLM_FORCE_DEPTH_ZERO_TO_ONE // opengl depth is -1:1, vulkan is 0:1, this makes it 0:1
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <stb/stb_image.h>

#include "Swapchain.h"
#include "Device.h"
#include "Extensions.h"
#include "debug/Debug.h"

#include <stdint.h>
#include <vector>
#include <algorithm>
#include <optional>
#include <limits.h>
#include <fstream>
#include <array>
#include <unordered_set>
#include <chrono>

std::vector<char> readFile(const std::string& filename);

uint32_t findValidMemoryTypeIndex(uint32_t validTypeFlags, VkPhysicalDeviceMemoryProperties pDeviceMemProps, VkMemoryPropertyFlags requiredMemProperties);
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags, VkBuffer* buffer, VkDeviceMemory* memory);
void copyBuffers(VkDevice device, uint32_t transferQueueFamilyIndex, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

struct Vertex {
	glm::vec2 pos;
	glm::vec3 col;
};

struct UBO {
	glm::vec2 omo;
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main(int argc, char* argv[]) {
	uint32_t WINDOW_HEIGHT{1080 / 2};
	uint32_t WINDOW_WIDTH{1920 / 2};

	if (SDL_Init(SDL_INIT_EVENTS) != 0) {
		std::cerr << "could not init sdl: " << SDL_GetError() << std::endl;
	}

	SDL_Window* window{SDL_CreateWindow("vulkan :D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE)};
	if (!window) {
		std::cerr << "window could not be created: " << SDL_GetError() << std::endl;
	}

	VkInstance instance;
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vulkan :D";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "VulkEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> instanceExtensions{getSurfaceExtensions(window)};

		std::vector<const char*> enabledExtensions{queryInstanceExtensions(instanceExtensions)};
		std::vector<const char*> debugExtensions{DEBUG::getInstanceDebugExtensions()};
		enabledExtensions.reserve(debugExtensions.size());
		enabledExtensions.insert(std::end(enabledExtensions), std::begin(debugExtensions), std::end(debugExtensions));

		std::vector<const char*> enabledLayers{DEBUG::getInstanceDebugLayers()};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{DEBUG::getDebugMessengerCreateInfo()};

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = enabledExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.enabledLayerCount = enabledLayers.size();
		createInfo.ppEnabledLayerNames = enabledLayers.data();
		createInfo.pNext = &debugCreateInfo;

		if(vkCreateInstance(&createInfo, nullptr, &instance)) {
			std::cerr << "could not create vulkan instance" << std::endl;
		}
	}

	VkDebugUtilsMessengerEXT debugMessenger{DEBUG::createDebugMessenger(instance)};

	VkSurfaceKHR surface{};
	SDL_Vulkan_CreateSurface(window, instance, &surface);

	VkPhysicalDevice physicalDevice{};
	QueueFamilyIndices queueFamilyIndices{};
	SurfaceSupportDetails surfaceSupportDetails{};
	std::vector<const char*> deviceExtensions{};
	{
		std::vector<const char*> requiredDeviceExtensions{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME
		};

		uint32_t physicalDeviceCount{};
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

		int highestRating{-1};
		for (const auto& pDevice : physicalDevices) {
			VkPhysicalDeviceProperties pDeviceProps{};

			vkGetPhysicalDeviceProperties(pDevice, &pDeviceProps);

			QueueInfo queueInfo{queryDeviceQueueFamilyIndices(pDevice, surface)};
			QueueFamilyIndices testQueueFamilyIndices{selectDeviceQueueFamilyIndices(queueInfo)};

			if (!testQueueFamilyIndices.uniqueIndices.size()) {
				continue;
			}

			std::vector<const char*> testDeviceExtensions{};
			try {
				testDeviceExtensions = queryDeviceExtensions(pDevice, requiredDeviceExtensions);
			} catch(std::string e) {
				continue;
			}

			SurfaceSupportDetails thisDeviceSurfaceSupportDetails{queryDeviceSurfaceSupportDetails(pDevice, surface)};
			bool swapchainSupported{
				!thisDeviceSurfaceSupportDetails.surfaceFormats.empty() &&
				!thisDeviceSurfaceSupportDetails.presentModes.empty()
			};

			if (!swapchainSupported) {
				continue;
			}

			int currentRating{};
			switch(pDeviceProps.deviceType) {
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
					currentRating += 1000;
					break;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
					currentRating += 100;
					break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU:
					currentRating += 10;
					break;
				default:
					break;
			}

			if (currentRating > highestRating) {
				highestRating = currentRating;
				deviceExtensions = testDeviceExtensions;
				physicalDevice = pDevice;
				surfaceSupportDetails = thisDeviceSurfaceSupportDetails;
				queueFamilyIndices = testQueueFamilyIndices;
			}
		}
	}

	VkDevice device{};
	{
		const float queuePriority[3] = {1.0f, 1.0f, 1.0f};
		size_t nQueueFamilyIndices{queueFamilyIndices.uniqueIndices.size()};

		std::vector<VkDeviceQueueCreateInfo> queuesToCreate{};
		queuesToCreate.reserve(nQueueFamilyIndices);
		for (int i{}; i < nQueueFamilyIndices; i++) {
			uint32_t queueCount{};
			for (const auto& family : queueFamilyIndices.uniqueIndices[i]) {
				if (family == QueueFamily::presentation) {
					continue;
				}
				queueCount++;
			}
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = i;
			queueCreateInfo.queueCount = queueCount;
			queueCreateInfo.pQueuePriorities = queuePriority;

			queuesToCreate.emplace_back(queueCreateInfo);
		}

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = queuesToCreate.size();
		deviceCreateInfo.pQueueCreateInfos = queuesToCreate.data();
		deviceCreateInfo.pEnabledFeatures = nullptr;
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

		vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
	}

	VkQueue graphicsQueue{};
	VkQueue presentQueue{};
	VkQueue transferQueue{};
	for (auto indexMap : queueFamilyIndices.uniqueIndices) {
		for (int i{}; i < indexMap.second.size(); i++) {
			QueueFamily family{indexMap.second[i]};
			uint32_t queueFamilyIndex{indexMap.first};

			switch (family) {
				case QueueFamily::graphics:
					vkGetDeviceQueue(device, queueFamilyIndex, i, &graphicsQueue);
				case QueueFamily::presentation:
					presentQueue = graphicsQueue;
					break;
				case QueueFamily::transfer:
					vkGetDeviceQueue(device, queueFamilyIndex, i, &transferQueue);
					break;
				default:
					break;
			}
		}
	}

	Swapchain swapchain{
		createSwapchain(
				device, window, surface, surfaceSupportDetails)
	};

	VkShaderModule vertexShaderModule{};
	VkShaderModule fragmentShaderModule{};
	{
		std::vector<char> vertShader{readFile("./shaders/first.vert.spv")};
		std::vector<char> fragShader{readFile("./shaders/first.frag.spv")};

		VkShaderModuleCreateInfo vShaderModuleCreateInfo{};
		vShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vShaderModuleCreateInfo.codeSize = vertShader.size();  // size in bytes
		vShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertShader.data());

		VkShaderModuleCreateInfo fShaderModuleCreateInfo{};
		fShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fShaderModuleCreateInfo.codeSize = fragShader.size();
		fShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragShader.data());

		vkCreateShaderModule(device, &vShaderModuleCreateInfo, nullptr, &vertexShaderModule);
		vkCreateShaderModule(device, &fShaderModuleCreateInfo, nullptr, &fragmentShaderModule);
	}

	VkPipelineShaderStageCreateInfo pipelineStages[2];
	{
		VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
		vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageCreateInfo.module = vertexShaderModule;
		vertShaderStageCreateInfo.pName = "main";
		vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
		fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageCreateInfo.module = fragmentShaderModule;
		fragShaderStageCreateInfo.pName = "main";
		fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		pipelineStages[0] = vertShaderStageCreateInfo;
		pipelineStages[1] = fragShaderStageCreateInfo;
	}

	std::vector<Vertex> vertices = {
		{{-0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, 
		{{0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, 
	};

	std::vector<uint16_t> indices = {
		0, 1, 2, 1, 3, 2
	};

	VkVertexInputBindingDescription vertexBindingDescription{};
	std::array<VkVertexInputAttributeDescription, 2> vertexAttributeDescriptions{};
	{
		vertexBindingDescription.binding = 0;
		vertexBindingDescription.stride = sizeof(Vertex);
		vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		vertexAttributeDescriptions[0].binding = 0;
		vertexAttributeDescriptions[0].offset = offsetof(Vertex, pos);
		vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributeDescriptions[0].location = 0;

		vertexAttributeDescriptions[1].binding = 0;
		vertexAttributeDescriptions[1].offset = offsetof(Vertex, col);
		vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributeDescriptions[1].location = 1;

	}

	VkBuffer vertexBuffer{};
	VkDeviceMemory vertexBufferMemory{};
	{
		VkDeviceMemory stagingBufferMemory{};
		VkBuffer stagingBuffer{};

		VkDeviceSize bufferSize{sizeof(vertices[0]) * vertices.size()};

		createBuffer(
				device, physicalDevice,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory
			);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(
				device, physicalDevice,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&vertexBuffer, &vertexBufferMemory
			);

		copyBuffers(device, queueFamilyIndices.familyToIndex[QueueFamily::transfer], transferQueue, stagingBuffer, vertexBuffer, bufferSize);
		
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	VkBuffer indexBuffer{};
	VkDeviceMemory indexBufferMemory{};
	{
		VkDeviceMemory stagingBufferMemory{};
		VkBuffer stagingBuffer{};

		VkDeviceSize bufferSize{sizeof(indices[0]) * indices.size()};

		createBuffer(
				device, physicalDevice,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory
			);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(
				device, physicalDevice,
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&indexBuffer, &indexBufferMemory
			);

		copyBuffers(device, queueFamilyIndices.familyToIndex[QueueFamily::transfer], transferQueue, stagingBuffer, indexBuffer, bufferSize);
		
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	uint32_t MAX_FRAMES_IN_FLIGHT{2};

	VkDescriptorPool descriptorPool{};
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;

		if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			std::cerr << "could not create descriptor pool" << std::endl;
		}
	}

	VkDescriptorSetLayout uboLayout{};
	{
		VkDescriptorSetLayoutBinding binding;
		binding.descriptorCount = 1;
		binding.binding = 0;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = &binding;

		if (vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &uboLayout) != VK_SUCCESS) {
			std::cerr << "could not create ubo layout" << std::endl;
		}
	}

	std::vector<VkDescriptorSet> uboDescriptorSets(MAX_FRAMES_IN_FLIGHT);
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, uboLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = layouts.size();
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = descriptorPool;

		if (vkAllocateDescriptorSets(device, &allocInfo, uboDescriptorSets.data()) != VK_SUCCESS) {
			std::cerr << "could not allocate descriptor sets" << std::endl;
		}
	}

	std::vector<VkBuffer> uboBuffers(MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDeviceMemory> uboMemory(MAX_FRAMES_IN_FLIGHT);
	std::vector<void*> uboMappedMem(MAX_FRAMES_IN_FLIGHT);
	{
		VkDeviceSize bufferSize{sizeof(UBO)};

		for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(
					device, physicalDevice,
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&uboBuffers[i],
					&uboMemory[i]
				);

			vkMapMemory(device, uboMemory[i], 0, bufferSize, 0, &uboMappedMem[i]);
		}
	}

	{
		for (int i{}; i < uboDescriptorSets.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.range = VK_WHOLE_SIZE;
			bufferInfo.buffer = uboBuffers[i];

			VkWriteDescriptorSet writeSet{};
			writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeSet.descriptorCount = 1;
			writeSet.dstSet = uboDescriptorSets[i];
			writeSet.dstBinding = 0;
			writeSet.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
		}
	}

	VkPipelineLayout pipelineLayout{};
	VkRenderPass renderPass{};
	VkPipeline pipeline{};
	{
		std::vector<VkDynamicState> dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
		dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
		vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{}; 
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = 0;

		VkPipelineViewportStateCreateInfo viewportCreateInfo{};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.viewportCount = 1;
		// viewportCreateInfo.pViewports = &viewport;
		// viewportCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerCreationInfo{};
		rasterizerCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreationInfo.depthBiasEnable = VK_FALSE;
		rasterizerCreationInfo.rasterizerDiscardEnable = VK_FALSE;

		rasterizerCreationInfo.lineWidth = 1.0;

		rasterizerCreationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

		rasterizerCreationInfo.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo;
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &uboLayout;

		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			std::cerr << "could not create pipeline layout" << std::endl;
		}

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchain.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS) {
			std::cerr << "could not create render pass" << std::endl;
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = pipelineStages;
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreationInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;

		pipelineCreateInfo.layout = pipelineLayout;

		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = 0;

		pipelineCreateInfo.basePipelineIndex = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
			std::cerr << "pipeline could not be created" << std::endl;
		}

		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
	}

	std::vector<VkImageView> swapchainImageViews{
		createSwapchainImageViews(
				device,
				swapchain)
	};

	std::vector<VkFramebuffer> swapchainFramebuffers{
		createSwapchainFramebuffers(device, swapchainImageViews, swapchain.extent, renderPass)
	};

	VkCommandPool commandPool{};
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.familyToIndex[QueueFamily::graphics];

		if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
			std::cerr << "could not create command pool" << std::endl;
		}
	}

	std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);
	{
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = commandPool;
		commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(device, &commandBufferAllocInfo, commandBuffers.data()) != VK_SUCCESS) {
			std::cerr << "could not create command buffers" << std::endl;
		}
	}

	std::vector<VkSemaphore> imageAvaliableSemaphores(MAX_FRAMES_IN_FLIGHT);
	std::vector<VkSemaphore> renderFinishedSemaphores(MAX_FRAMES_IN_FLIGHT);
	std::vector<VkFence> inFlightFences(MAX_FRAMES_IN_FLIGHT);
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvaliableSemaphores[i]) != VK_SUCCESS) {
				std::cerr << "could not create semaphore" << std::endl;
			}
			if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
				std::cerr << "could not create semaphore" << std::endl;
			}
			if (vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				std::cerr << "could not create fence" << std::endl;
			}
		}
	}

	SDL_Event e;
	bool running{true};
	bool windowResized{};
	bool rendering{true};
	uint32_t frame{};

	uint32_t framesRendered{};
	auto startTime{ std::chrono::high_resolution_clock::now() };
	auto fpsTime{startTime};

	while (running) {
			while (SDL_PollEvent(&e)) {
				switch (e.type) {
					case SDL_QUIT:
						running = false;
						break;
					case SDL_KEYDOWN:
						if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
							running = false;
						}
						break;
					case SDL_WINDOWEVENT:
						switch (e.window.event) {
							case SDL_WINDOWEVENT_RESIZED:
							case SDL_WINDOWEVENT_SIZE_CHANGED:
								windowResized = true;
								break;
							case SDL_WINDOWEVENT_MINIMIZED:
								rendering = false;
								break;
							case SDL_WINDOWEVENT_RESTORED:
								rendering = true;
								break;
							default: break;
						}
						break;
					default: break;
				}
			}

		if (rendering) {
			uint32_t swapchainImageIndex{};
			{
				vkWaitForFences(device, 1, &inFlightFences[frame], VK_TRUE, UINT64_MAX);

				VkResult result {vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, imageAvaliableSemaphores[frame], VK_NULL_HANDLE, &swapchainImageIndex)};
				bool swapchainIsDirty{result == VK_ERROR_OUT_OF_DATE_KHR || windowResized};

				if (swapchainIsDirty) {
					windowResized = false;
					VkSwapchainKHR oldSwapchainHandle{swapchain.handle};
					swapchain = recreateSwapchain(device, window, surface, queryDeviceSurfaceSupportDetails(physicalDevice, surface), oldSwapchainHandle);

					destroySwapchain(device, oldSwapchainHandle, nullptr, nullptr);
					vkWaitForFences(device, MAX_FRAMES_IN_FLIGHT, inFlightFences.data(), VK_TRUE, UINT64_MAX);

					destroySwapchainImageViews(device, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device, swapchainImageViews, swapchain.extent, renderPass);

					continue;
				}
				if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
					std::cerr << "could not get image from swapchain" << std::endl;
				}
			}

			vkResetFences(device, 1, &inFlightFences[frame]);
			vkResetCommandBuffer(commandBuffers[frame], 0);
			
			{
				auto currentTime{ std::chrono::high_resolution_clock::now() };
				float time{ std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() };

				UBO ubo{};
				ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(1.f, 1.f, 1.f));
				ubo.view = glm::lookAt(glm::vec3(2.f, 0.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
				ubo.proj = glm::perspective(glm::radians(45.f), (float)swapchain.extent.width / swapchain.extent.height, 0.1f, 10.f);

				memcpy(uboMappedMem[frame], &ubo, sizeof(ubo));
			}

			{
				VkCommandBufferBeginInfo cmdBuffBeginInfo{};
				cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
				if (vkBeginCommandBuffer(commandBuffers[frame], &cmdBuffBeginInfo) != VK_SUCCESS) {
					std::cerr << "could not begin command buffer recording" << std::endl;
				}
			}
	
			{
				VkClearValue clearColor{};
				clearColor.color = {{0.f, 0.f, 0.f, 1.f}};
	
				VkRenderPassBeginInfo renderPassBeginInfo{};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = swapchainFramebuffers[swapchainImageIndex];
				renderPassBeginInfo.renderArea.extent = swapchain.extent;
	
				renderPassBeginInfo.clearValueCount = 1;
				renderPassBeginInfo.pClearValues = &clearColor;
	
				vkCmdBeginRenderPass(commandBuffers[frame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			}
	
			{
				vkCmdBindPipeline(commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				VkBuffer vertexBuffers[] = {vertexBuffer};
				VkDeviceSize offsets[] = {0};

				vkCmdBindVertexBuffers(commandBuffers[frame], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffers[frame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	
				VkViewport viewport{};

				// flipped y of viewport to make glm work out of the box (opengl has y going up)
				viewport.height = -static_cast<float>(swapchain.extent.height);
				viewport.y = (float)swapchain.extent.height;

				viewport.width = static_cast<float>(swapchain.extent.width);
				viewport.maxDepth = 1.0;
	
				VkRect2D scissor{};
				scissor.extent = swapchain.extent;
	
				vkCmdSetViewport(commandBuffers[frame], 0, 1, &viewport);
				vkCmdSetScissor(commandBuffers[frame], 0, 1, &scissor);

				vkCmdBindDescriptorSets(commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uboDescriptorSets[frame], 0, nullptr);
			}

			vkCmdDrawIndexed(commandBuffers[frame], indices.size(), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[frame]);
			if (vkEndCommandBuffer(commandBuffers[frame]) != VK_SUCCESS) {
				std::cerr << "could not record command buffer" << std::endl;
			}

			{
				VkSemaphore waitSemaphores[] = {imageAvaliableSemaphores[frame]};
				VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[frame]};
				VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pCommandBuffers = &commandBuffers[frame];
				submitInfo.pSignalSemaphores = signalSemaphores;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;

				if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[frame]) != VK_SUCCESS) {
					std::cout << "could not submit draw command buffer" << std::endl;
				}

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;

				VkSwapchainKHR swapchains[] = {swapchain.handle};
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapchains;
				presentInfo.pImageIndices = &swapchainImageIndex;

				VkResult result{vkQueuePresentKHR(presentQueue, &presentInfo)};
				bool swapchainIsDirty{result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized};

				if (swapchainIsDirty) {
					windowResized = false;
					VkSwapchainKHR oldSwapchainHandle{swapchain.handle};
					swapchain = recreateSwapchain(device, window, surface, queryDeviceSurfaceSupportDetails(physicalDevice, surface), oldSwapchainHandle);

					vkDeviceWaitIdle(device);
					destroySwapchain(device, oldSwapchainHandle, nullptr, nullptr);

					destroySwapchainImageViews(device, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device, swapchainImageViews, swapchain.extent, renderPass);

					continue;
				}
				if (result != VK_SUCCESS) {
					std::cerr << "could not present image" << std::endl;
				}
			}

			frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
			framesRendered++;

			auto currentTime{ std::chrono::high_resolution_clock::now() };
			float duration{ std::chrono::duration<float, std::chrono::seconds::period>(currentTime - fpsTime).count() };
			if (duration >= 1) {
				fpsTime = currentTime;
				std::cout << "frames renderered: " << framesRendered << " | ms per frame: " << 1.f / framesRendered << std::endl;;
				framesRendered = 0;
			}
		}
	}
	vkDeviceWaitIdle(device);
	for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, imageAvaliableSemaphores[i], nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);

		vkDestroyBuffer(device, uboBuffers[i], nullptr);
		vkFreeMemory(device, uboMemory[i], nullptr);
	}

	destroySwapchain(device, swapchain.handle, &swapchainImageViews, &swapchainFramebuffers);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, uboLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	DEBUG::destroyDebugMessenger(instance, debugMessenger, nullptr);
	vkDestroyInstance(instance, nullptr);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file) {
		std::cout << "could not open shader" << std::endl;
		return {};
	}

	auto fileLength{static_cast<size_t>(file.tellg())};

	std::vector<char> buf(fileLength);
	file.seekg(0);
	file.read(buf.data(), fileLength);
	file.close();

	return buf;
}

uint32_t findValidMemoryTypeIndex(uint32_t validTypeFlags, VkPhysicalDeviceMemoryProperties pDeviceMemProps, VkMemoryPropertyFlags requiredMemProperties) {
	for (int i{}; i < pDeviceMemProps.memoryTypeCount; i++) {
		VkMemoryPropertyFlags requiredProperties{VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
		if ((validTypeFlags & (1 << i)) &&  // bit i is set only if pDeviceMemProps.memoryTypes[i] is a valid type
			((pDeviceMemProps.memoryTypes[i].propertyFlags & requiredProperties)) == requiredProperties) {  // having more properties isnt invalid
			return i;
		}
	}
	throw std::runtime_error("could not find valid memory type");
}

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags, VkBuffer* buffer, VkDeviceMemory* memory) {
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.size = size;

	if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS) {
		throw std::runtime_error("could not create buffer");
	}

	VkMemoryRequirements bufferMemoryRequirements{};
	vkGetBufferMemoryRequirements(device, *buffer, &bufferMemoryRequirements);

	VkPhysicalDeviceMemoryProperties memProps{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = bufferMemoryRequirements.size;
	memAllocInfo.memoryTypeIndex = findValidMemoryTypeIndex(bufferMemoryRequirements.memoryTypeBits, memProps, memFlags);

	// TODO: use a better allocation strategy
	if (vkAllocateMemory(device, &memAllocInfo, nullptr, memory) != VK_SUCCESS) {
		throw std::runtime_error("could not allocate buffer");
	}

	vkBindBufferMemory(device, *buffer, *memory, 0);
}

void copyBuffers(VkDevice device, uint32_t transferQueueFamilyIndex, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize) {
	VkCommandPool transferPool{};
	{
		VkCommandPoolCreateInfo transferPoolCreateInfo{};
		transferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		transferPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		transferPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

		if (vkCreateCommandPool(device, &transferPoolCreateInfo, nullptr, &transferPool) != VK_SUCCESS) {
			throw std::runtime_error("could not create transfer command pool");
		}
	}

	VkCommandBuffer transferCommandBuffer;
	{
		VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.commandPool = transferPool;
		cmdBufferAllocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, &transferCommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("could not allocate transfer command buffer");
		}
	}

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(transferCommandBuffer, &cmdBufferBeginInfo);

	VkBufferCopy bufCopyRegion{};
	bufCopyRegion.size = bufferSize;

	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufCopyRegion);

	vkEndCommandBuffer(transferCommandBuffer);

	VkSubmitInfo bufSubmitInfo{};
	bufSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	bufSubmitInfo.commandBufferCount = 1;
	bufSubmitInfo.pCommandBuffers = &transferCommandBuffer;

	if (vkQueueSubmit(transferQueue, 1, &bufSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("could not submit to transfer queue");
	}
	vkQueueWaitIdle(transferQueue);

	vkDestroyCommandPool(device, transferPool, nullptr);
}
