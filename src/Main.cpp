#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Swapchain.h"

#include <stdint.h>
#include <vector>
#include <algorithm>
#include <optional>
#include <limits.h>
#include <fstream>
#include <array>

std::vector<char> readFile(const std::string& filename);

std::vector<const char*> getLayers();
std::vector<const char*> getExtensions(SDL_Window* window);
std::vector<const char*> getDeviceExtensions(VkPhysicalDevice pDevice, const std::vector<const char*>& requiredExtensions);
SurfaceSupportDetails querySwapchainSupport(VkPhysicalDevice pDevice, VkSurfaceKHR surface);
uint32_t findValidMemoryTypeIndex(uint32_t validTypeFlags, VkPhysicalDeviceMemoryProperties pDeviceMemProps, VkMemoryPropertyFlags requiredMemProperties);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

// dll loaded functions
PFN_vkCreateDebugUtilsMessengerEXT load_PFK_vkCreateDebugUtilsMessengerEXT(VkInstance instance);
PFN_vkDestroyDebugUtilsMessengerEXT load_PFK_vkDestroyDebugUtilsMessengerEXT(VkInstance instance);

VkDebugUtilsMessengerCreateInfoEXT populateDebugCreateInfo();

struct Vertex {
	glm::vec2 pos;
	glm::vec3 col;
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

		std::vector<const char*> enabledExtensions{getExtensions(window)};
		std::vector<const char*> enabledLayers{getLayers()};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{populateDebugCreateInfo()};

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


	PFN_vkCreateDebugUtilsMessengerEXT VK_CreateDebugUtilsMessengerEXT{load_PFK_vkCreateDebugUtilsMessengerEXT(instance)};
	PFN_vkDestroyDebugUtilsMessengerEXT VK_DestroyDebugUtilsMessengerEXT{load_PFK_vkDestroyDebugUtilsMessengerEXT(instance)};

	VkDebugUtilsMessengerEXT debugMessenger{};
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{populateDebugCreateInfo()};
		if(VK_CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			std::cerr << "could not create debug messenger" << std::endl;
		}
	}

	VkSurfaceKHR surface{};
	{
		SDL_Vulkan_CreateSurface(window, instance, &surface);
	}

	VkPhysicalDevice physicalDevice{};
	std::optional<uint32_t> graphicsQueueFamilyIndex{};
	std::optional<uint32_t> presentQueueFamilyIndex{};
	std::vector<const char*> deviceExtensions{};
	SurfaceSupportDetails surfaceSupportDetails{};
	{
		std::vector<const char*> requiredDeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		uint32_t nPhysicalDevices{};
		vkEnumeratePhysicalDevices(instance, &nPhysicalDevices, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(nPhysicalDevices);
		vkEnumeratePhysicalDevices(instance, &nPhysicalDevices, physicalDevices.data());

		int highestRating{-1};
		for (const auto& pDevice : physicalDevices) {
			VkPhysicalDeviceProperties pDeviceProps{};

			vkGetPhysicalDeviceProperties(pDevice, &pDeviceProps);

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

			uint32_t queueFamilyCount{};
			vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilyProps.data());

			std::optional<uint32_t> graphicsFamilyIndex;
			std::optional<uint32_t> presentFamilyIndex;

			for (int i{}; i < queueFamilyProps.size(); i++) {
				if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsFamilyIndex = i;
				}

				VkBool32 presentationSupport{};
				vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &presentationSupport);
				if (presentationSupport) {
					presentFamilyIndex = i;
				}
			}

			if (graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value()) {

				std::vector<const char*> tempDeviceExtensions = getDeviceExtensions(pDevice, requiredDeviceExtensions);
				if (currentRating > highestRating && !tempDeviceExtensions.empty()) {
					SurfaceSupportDetails thisDeviceSwapchainSupport{querySwapchainSupport(pDevice, surface)};
					bool swapchainSupported{
						!thisDeviceSwapchainSupport.surfaceFormats.empty() &&
						!thisDeviceSwapchainSupport.presentModes.empty()
					};
					if (swapchainSupported) {
						graphicsQueueFamilyIndex = graphicsFamilyIndex.value();
						presentQueueFamilyIndex = presentFamilyIndex.value();
						deviceExtensions = tempDeviceExtensions;
						physicalDevice = pDevice;

						surfaceSupportDetails = thisDeviceSwapchainSupport;
					}
				}
			}
		}
	}

	VkDevice device{};
	{
		const float queuePriority{1.f};

		VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
		graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();
		graphicsQueueCreateInfo.queueCount = 1;
		graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceQueueCreateInfo presentQueueCreateInfo{};
		presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex.value();
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceQueueCreateInfo queuesToCreate[2];
		queuesToCreate[0] = graphicsQueueCreateInfo;
		queuesToCreate[1] = presentQueueCreateInfo;

		// VkPhysicalDeviceFeatures pDeviceFeats{};
		// vkGetPhysicalDeviceFeatures(physicalDevice, &pDeviceFeats);

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 2;
		deviceCreateInfo.pQueueCreateInfos = queuesToCreate;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

		vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
	}

	VkQueue graphicsQueue{};
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex.value(), 0, &graphicsQueue);

	VkQueue presentQueue{};
	vkGetDeviceQueue(device, presentQueueFamilyIndex.value(), 0, &presentQueue);

	std::vector<uint32_t> queueFamilyIndices{ graphicsQueueFamilyIndex.value(), presentQueueFamilyIndex.value() };

	Swapchain swapchain{
		createSwapchain(
				device, window, surface, surfaceSupportDetails, queueFamilyIndices)
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
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	VkVertexInputBindingDescription vertexBindingDescription{};
	std::array<VkVertexInputAttributeDescription, 2> vertexAttributeDescriptions{};
	{
		vertexBindingDescription.binding = 0; // not location, but buffer index
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
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
			std::cerr << "could not create vertex buffer" << std::endl;
		}

		VkMemoryRequirements bufferMemRequirements;
		vkGetBufferMemoryRequirements(device, vertexBuffer, &bufferMemRequirements);

		VkPhysicalDeviceMemoryProperties pDeviceMemProperties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &pDeviceMemProperties);

		VkMemoryPropertyFlags requiredProperties{VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
		uint32_t typeIndex{findValidMemoryTypeIndex(bufferMemRequirements.memoryTypeBits, pDeviceMemProperties, requiredProperties)};

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = bufferMemRequirements.size;
		allocInfo.memoryTypeIndex = typeIndex;
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
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();

		if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
			std::cerr << "could not create command pool" << std::endl;
		}
	}

	uint32_t MAX_FRAMES_IN_FLIGHT{2};

	std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);
	{
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = commandPool;
		commandBufferAllocInfo.commandBufferCount = 1;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &commandBuffers[i]) != VK_SUCCESS) {
				std::cerr << "could not create command buffers" << std::endl;
			}
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
				VkResult result {vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, imageAvaliableSemaphores[frame], VK_NULL_HANDLE, &swapchainImageIndex)};
				bool swapchainIsDirty{result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized};

				if (swapchainIsDirty) {
					windowResized = false;
					VkSwapchainKHR oldSwapchainHandle{swapchain.handle};
					swapchain = recreateSwapchain(device, window, surface, querySwapchainSupport(physicalDevice, surface), queueFamilyIndices, oldSwapchainHandle);

					destroySwapchain(device, oldSwapchainHandle, nullptr, nullptr);
					vkWaitForFences(device, 2, inFlightFences.data(), VK_TRUE, UINT64_MAX);

					destroySwapchainImageViews(device, &swapchainImageViews, &swapchainFramebuffers);
					swapchainImageViews = createSwapchainImageViews(device, swapchain);
					swapchainFramebuffers = createSwapchainFramebuffers(device, swapchainImageViews, swapchain.extent, renderPass);

					result = vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, imageAvaliableSemaphores[frame], VK_NULL_HANDLE, &swapchainImageIndex);
				}
				if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
					std::cerr << "could not get image from swapchain" << std::endl;
				}

			}

			vkWaitForFences(device, 1, &inFlightFences[frame], VK_TRUE, UINT64_MAX);
			vkResetFences(device, 1, &inFlightFences[frame]);
			vkResetCommandBuffer(commandBuffers[frame], 0);
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
	
				VkViewport viewport{};
				viewport.height = static_cast<float>(swapchain.extent.height);
				viewport.width = static_cast<float>(swapchain.extent.width);
				viewport.maxDepth = 1.0;
	
				VkRect2D scissor{};
				scissor.extent = swapchain.extent;
	
				vkCmdSetViewport(commandBuffers[frame], 0, 1, &viewport);
				vkCmdSetScissor(commandBuffers[frame], 0, 1, &scissor);
			}

			vkCmdDraw(commandBuffers[frame], 3, 1, 0, 0);
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

				vkQueuePresentKHR(presentQueue, &presentInfo);
			}

			frame = (frame + 1) % 2;
		}
	}
	vkDeviceWaitIdle(device);
	for (int i{}; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, imageAvaliableSemaphores[i], nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	destroySwapchain(device, swapchain.handle, &swapchainImageViews, &swapchainFramebuffers);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	VK_DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
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

std::vector<const char*> getExtensions(SDL_Window* window) {
	std::vector<const char*> enabledExtensions;

	uint32_t SDLExtensionCount{};
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, nullptr);

	std::vector<const char*> SDLExtensions(SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions.data());

	std::vector<const char*> extensions;
	extensions.reserve(SDLExtensionCount);
	for (const auto& extension : SDLExtensions) {
		extensions.emplace_back(extension);
	}
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

	uint32_t avaliableExtensionCount{};
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &avaliableExtensionCount, avaliableExtensions.data());

	for (const auto& extension : extensions) {
		auto itt{
			std::find_if(
					avaliableExtensions.begin(),
					avaliableExtensions.end(),
					[&](const auto& avaliableExtension) {return strcmp(avaliableExtension.extensionName, extension) == 0; }
					)
		};

		if (itt != avaliableExtensions.end()) {
			enabledExtensions.emplace_back(extension);
		}
	}
	if (enabledExtensions.size() < extensions.size()) {
		std::cout << "not all extensions avaliable: " << enabledExtensions.size() << " of " << extensions.size() << std::endl;
	}

	return enabledExtensions;
}

 std::vector<const char*> getDeviceExtensions(VkPhysicalDevice pDevice, const std::vector<const char*>& requiredExtensions) {
	uint32_t avaliableExtensionsCount{};
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &avaliableExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> avaliableExtensions(avaliableExtensionsCount);
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &avaliableExtensionsCount, avaliableExtensions.data());

	std::vector<const char*> extensions{};
	for (const auto& requiredExtension : requiredExtensions) {
		auto itt{
			std::find_if(
					avaliableExtensions.begin(),
					avaliableExtensions.end(),
					[&](const auto& avaliableExtension) {
						return strcmp(avaliableExtension.extensionName, requiredExtension) == 0;
					}
			)
		};

		if (itt != avaliableExtensions.end()) {
			extensions.emplace_back(requiredExtension);
		} else {
			std::cout << requiredExtension << " could not be found" << std::endl;
			return {};
		}
	}

	return extensions;
}

std::vector<const char*> getLayers() {
	std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

	uint32_t layerCount{};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> avaliableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

	std::vector<const char*> enabledLayers;
	for (const auto& validationLayer : validationLayers) {
		auto itt{
			std::find_if(
					avaliableLayers.begin(),
					avaliableLayers.end(),
					[&](const auto& layer) { return strcmp(validationLayer, layer.layerName) == 0; }
					)
		};

		if (itt != avaliableLayers.end()) {
			enabledLayers.emplace_back(validationLayer);
		}
	}
	if (enabledLayers.size() < validationLayers.size()) {
		std::cout << "not all validation layers avaliable: " << enabledLayers.size() << " of " << validationLayers.size() << std::endl;
	}

	return validationLayers;
}


SurfaceSupportDetails querySwapchainSupport(VkPhysicalDevice pDevice, VkSurfaceKHR surface) {
	SurfaceSupportDetails swapchainDetails{};

	uint32_t surfaceFormatsCount{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &surfaceFormatsCount, nullptr);

	if (surfaceFormatsCount) {
		swapchainDetails.surfaceFormats.resize(surfaceFormatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &surfaceFormatsCount, swapchainDetails.surfaceFormats.data());
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice, surface, &swapchainDetails.surfaceCapabilities);

	uint32_t presentModesCount{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, surface, &presentModesCount, nullptr);

	if (presentModesCount) {
		swapchainDetails.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, surface, &presentModesCount, swapchainDetails.presentModes.data());
	}

	return swapchainDetails;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	std::string severityStr{};
	switch(messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severityStr = "INFO";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severityStr = "VERBOSE";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severityStr = "WARNING";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severityStr = "ERROR";
			break;
		default:
			break;
	}

    std::cerr << "\tvalidation layer: " << severityStr << " : " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// create stub
PFN_vkCreateDebugUtilsMessengerEXT load_PFK_vkCreateDebugUtilsMessengerEXT(VkInstance instance) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    return func;
}

PFN_vkDestroyDebugUtilsMessengerEXT load_PFK_vkDestroyDebugUtilsMessengerEXT(VkInstance instance) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDestroyUtilsMessengerEXT"));
    return func;
}

VkDebugUtilsMessengerCreateInfoEXT populateDebugCreateInfo() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	return createInfo;
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
