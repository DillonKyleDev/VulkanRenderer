#pragma once
#include "Model.h"
#include "Texture.h"
#include "Material.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "RenderPass.h"
#include "WinSys.h"

#include <vector>
#include <string>


namespace VCore
{
	class GameObject
	{
	public:
		GameObject();
		~GameObject();

		void Cleanup(LogicalDevice& logicalDevice);
		void SetModel(Model model);
		Model& GetModel();
		void CreateModelResources(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void SetMaterial(Material material);
		Material& GetMaterial();
		void CreateMaterialResources(LogicalDevice& logicalDevice, WinSys& winSystem, RenderPass& renderPass);
		void AddTexture(std::string path);
		std::vector<Texture>& GetTextures();
		void CreateTextureResources(WinSys& winSystem, VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void CreateResources(WinSys& winSystem, VkCommandPool commandPool, RenderPass& renderPass, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);

	private:
		Model m_model;
		Material m_material;
		std::vector<Texture> m_textures;
	};
}