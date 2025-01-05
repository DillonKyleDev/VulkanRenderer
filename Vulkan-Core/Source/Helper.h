#pragma once
#include "Structs.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vector>
#include <string>


namespace VCore
{
    class Helper
    {
    public:
        static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
        static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        static bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
        static std::vector<std::string> FindAllFilesWithExtension(std::string dir, std::string extension);
        static VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool, LogicalDevice &logicalDevice);
        static void EndSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer, LogicalDevice &logicalDevice);
        static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
        static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
        static bool HasStencilComponent(VkFormat format);
        static std::vector<char> ReadFile(const std::string& filename);
    };
}