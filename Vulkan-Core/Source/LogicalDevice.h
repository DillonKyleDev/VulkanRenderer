#pragma once
#include "PhysicalDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>


namespace VCore
{
	class LogicalDevice
	{
	public:
		LogicalDevice();
		~LogicalDevice();

		VkDevice& GetDevice();
		VkQueue& GetGraphicsQueue();
		VkQueue& GetPresentQueue();
		void Init(PhysicalDevice& physicalDevice, VkSurfaceKHR surface);
		void Cleanup();

	private:
		VkDevice m_device;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;
	};
}

