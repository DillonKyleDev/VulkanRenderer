#pragma once
#include "Model.h"
#include "Texture.h"
#include "Material.h"
#include "LogicalDevice.h"

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
		Model& GetModel();
		Material& GetMaterial();
		void AddTexture(std::string path);
		std::vector<Texture>& GetTextures();

	private:
		Model m_model;
		Material m_material;
		std::vector<Texture> m_textures;
	};
}