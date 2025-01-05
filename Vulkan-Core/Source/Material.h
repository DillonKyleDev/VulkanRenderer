#pragma once
#include "GraphicsPipeline.h"
#include "LogicalDevice.h"
#include "WinSys.h"
#include "RenderPass.h"
#include "Texture.h"
#include "Model.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <string>
#include <vector>


namespace VCore
{
	class Material
	{
	public:
		Material(std::string vertexPath, std::string fragmentPath);
		Material();
		~Material();

		void SetVertexPath(std::string path);
		void SetFragmentPath(std::string path);
		void CreateGraphicsPipeline(LogicalDevice& logicalDevice, WinSys& winSystem, RenderPass& renderPass);
		VkPipeline& GetGraphicsPipeline();
		VkPipelineLayout& GetPipelineLayout();
		void CreateDescriptorSetLayout(LogicalDevice& logicalDevice);
		VkDescriptorSetLayout& GetDescriptorSetLayout();
		std::vector<std::vector<VkDescriptorSet>> GetDescriptorSets();
		void CreateDescriptorPools(std::vector<Texture> textures, LogicalDevice& logicalDevice);
		void CreateDescriptorSets(std::vector<Texture> textures, Model& model, LogicalDevice& logicalDevice);
		void CleanupGraphicsPipeline(LogicalDevice& logicalDevice);
		void CleanupDescriptorSetLayout(LogicalDevice& logicalDevice);
		void CleanupDescriptorPools(LogicalDevice& logicalDevice);

	private:
		GraphicsPipeline m_graphicsPipeline;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<VkDescriptorPool> m_descriptorPools;
		std::vector<std::vector<VkDescriptorSet>> m_descriptorSets;
	};
}