#pragma once
#include "GraphicsPipeline.h"
#include "PhysicalDevice.h"
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
		void CleanupGraphicsPipeline(LogicalDevice& logicalDevice);
		void CleanupDescriptorSetLayout(LogicalDevice& logicalDevice);		
		void CleanupTextures(LogicalDevice& logicalDevice);

		void CreateMaterialResources(WinSys& winSystem, VkCommandPool commandPool, RenderPass& renderPass, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void SetVertexPath(std::string path);
		void SetFragmentPath(std::string path);
		void CreateGraphicsPipeline(LogicalDevice& logicalDevice, WinSys& winSystem, RenderPass& renderPass);
		VkPipeline& GetGraphicsPipeline();
		VkPipelineLayout& GetPipelineLayout();
		void CreateDescriptorSetLayout(LogicalDevice& logicalDevice);
		VkDescriptorSetLayout& GetDescriptorSetLayout();
		void CreateDescriptorPool(VkDescriptorPool& descriptorPool, LogicalDevice& logicalDevice);
		void CreateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorPool& descriptorPool, Model& model, LogicalDevice& logicalDevice);
		void AddTexture(std::string path);
		std::vector<Texture>& GetTextures();
		void CreateTextureResources(WinSys& winSystem, VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);

	private:
		GraphicsPipeline m_graphicsPipeline;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<Texture> m_textures;
	};
}