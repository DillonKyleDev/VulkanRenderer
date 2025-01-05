#pragma once
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "WinSys.h"
#include "RenderPass.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vector>
#include <string>


namespace VCore
{
	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(std::string vertexPath, std::string fragmentPath);
		GraphicsPipeline();
		~GraphicsPipeline();

		void SetVertexPath(std::string path);
		void SetFragmentPath(std::string path);
		VkShaderModule CreateShaderModule(const std::vector<char>& code, LogicalDevice& logicalDevice);
		void CreateGraphicsPipeline(LogicalDevice& logicalDevice, WinSys& winSystem, RenderPass& renderPass, VkDescriptorSetLayout descriptorSetLayout);
		VkPipeline& GetGraphicsPipeline();
		VkPipelineLayout& GetPipelineLayout();
		void Cleanup(LogicalDevice& logicalDevice);

	private:
		VkPipeline m_graphicsPipeline;
		VkPipelineLayout m_pipelineLayout;
		std::string m_vertexPath;
		std::string m_fragmentPath;
	};
}