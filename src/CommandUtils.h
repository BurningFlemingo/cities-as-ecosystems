#pragma once

#include "pch.h"
#include "Device.h"

namespace VkUtils {

VkCommandBuffer beginTransientCommands(const Device& device, VkCommandPool commandPool);
void endTransientCommands(const Device& device, VkCommandPool pool, VkCommandBuffer cmdBuffer, VkQueue queue);

}
