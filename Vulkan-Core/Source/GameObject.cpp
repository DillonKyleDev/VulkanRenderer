#include "GameObject.h"



namespace VCore
{
	GameObject::GameObject()
	{
		m_model = Model();
	}

	GameObject::~GameObject()
	{

	}

	void GameObject::Cleanup(LogicalDevice& logicalDevice)
	{
		for (Texture texture : m_textures)
		{
			texture.Cleanup(logicalDevice);
		}
	}

	Model& GameObject::GetModel()
	{
		return m_model;
	}

	Material& GameObject::GetMaterial()
	{
		return m_material;
	}

	void GameObject::AddTexture(std::string path)
	{
		Texture newTexture = Texture();
		newTexture.SetTexturePath(path);
		m_textures.push_back(newTexture);
	}

	std::vector<Texture>& GameObject::GetTextures()
	{
		return m_textures;
	}
}