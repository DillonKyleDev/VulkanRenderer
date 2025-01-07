#pragma once
#include "Model.h"
#include "Material.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "RenderPass.h"
#include "WinSys.h"

#include <vector>
#include <string>
#include <memory>


namespace VCore
{
	class GameObject
	{
	public:
		GameObject();
		~GameObject();
		void CleanupDescriptorPool(LogicalDevice& logicalDevice);

		void SetModel(Model model);
		Model& GetModel();
		void CreateModelResources(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void SetMaterial(std::shared_ptr<Material> material);
		std::shared_ptr<Material> GetMaterial();	
		void CreateResources(WinSys& winSystem, VkCommandPool commandPool, RenderPass& renderPass, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		std::vector<VkDescriptorSet>& GetDescriptorSets();

	private:
		Model m_model;
		std::shared_ptr<Material> m_material;
		VkDescriptorPool m_descriptorPool;
		std::vector<VkDescriptorSet> m_descriptorSets;
	};
}