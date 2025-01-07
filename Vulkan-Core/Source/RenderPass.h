#pragma once
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "WinSys.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vector>


namespace VCore
{
	class GameObject;

	class RenderPass
	{
	public:
		RenderPass();
		~RenderPass();
		void Cleanup(LogicalDevice& logicalDevice);

		void CreateRenderPass(WinSys& winSystem, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		VkRenderPass& GetRenderPass();
		void CreateCommandBuffers(VkCommandPool commandPool, LogicalDevice& logicalDevice);
		std::vector<VkCommandBuffer>& GetCommandBuffers();
		void RecordCommandBuffer(uint32_t imageIndex, WinSys& winSystem, GameObject& object);
		void BeginRenderPass(uint32_t imageIndex, WinSys& winSystem);
		void EndRenderPass();

	private:
		VkRenderPass m_renderPass;
		std::vector<VkCommandBuffer> m_commandBuffers;
	};
}


