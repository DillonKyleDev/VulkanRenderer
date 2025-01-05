#pragma once
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "WinSys.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>


namespace VCore
{
	class RenderPass
	{
	public:
		RenderPass();
		~RenderPass();

		void CreateRenderPass(WinSys& winSystem, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		VkRenderPass& GetRenderPass();
		void Cleanup(LogicalDevice& logicalDevice);

	private:
		VkRenderPass m_renderPass;
	};
}


