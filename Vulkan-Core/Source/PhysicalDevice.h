#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>


namespace VCore
{
    class PhysicalDevice
    {
    public:
        PhysicalDevice();
        ~PhysicalDevice();

        VkPhysicalDevice GetDevice();
        void Init(VkInstance instance, VkSurfaceKHR surface);
        void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        int RateDeviceSuitability(VkPhysicalDevice device);
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void Cleanup();

    private:
        VkPhysicalDevice m_physicalDevice;
        VkPhysicalDeviceProperties m_physicalDeviceProperties;
        VkSampleCountFlagBits m_msaaSamples;
    };
}