#pragma once

#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#include <functional>
#include <tuple>

#include "Cleanup.h"

class DeletionQueue;

enum class QueueFamily : uint32_t {
  graphics = 0,
  transfer,
  presentation,
};

struct Queues {
  VkQueue graphicsQueue;
  VkQueue presentationQueue;
  VkQueue transferQueue;
};

struct QueueFamilyIndices {
  uint32_t graphicsIndex{static_cast<uint32_t>(-1)};
  uint32_t transferIndex{static_cast<uint32_t>(-1)};
  uint32_t presentationIndex{static_cast<uint32_t>(-1)};

  std::unordered_map<QueueFamily, uint32_t> indices;

  std::unordered_map<uint32_t, std::vector<QueueFamily>> uniqueIndices;
};

struct Device {
  VkDevice logical;
  VkPhysicalDevice physical;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceMemoryProperties memProperties;

  QueueFamilyIndices queueFamilyIndices;
};

namespace VulkanRenderer {
Device createDevice(const VkSurfaceKHR &surface, const VkInstance &instance,
                    DeletionQueue &deletionQueue);
}

Queues getDeviceQueues(const Device &device);
