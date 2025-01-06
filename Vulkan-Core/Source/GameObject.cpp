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

	void GameObject::SetModel(Model model)
	{
		m_model = model;
	}

	Model& GameObject::GetModel()
	{
		return m_model;
	}

	void GameObject::CreateModelResources(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
	{
		m_model.LoadModel();
		m_model.CreateVertexBuffer(commandPool, physicalDevice, logicalDevice);
		m_model.CreateIndexBuffer(commandPool, physicalDevice, logicalDevice);
		m_model.CreateUniformBuffers(physicalDevice, logicalDevice);
		m_model.CreateCommandBuffers(commandPool, logicalDevice);
	}

	void GameObject::SetMaterial(Material material)
	{
		m_material = material;
	}

	Material& GameObject::GetMaterial()
	{
		return m_material;
	}

	void GameObject::CreateMaterialResources(LogicalDevice& logicalDevice, WinSys& winSystem, RenderPass& renderPass)
	{
		m_material.CreateDescriptorSetLayout(m_textures, logicalDevice);
		m_material.CreateGraphicsPipeline(logicalDevice, winSystem, renderPass);
		m_material.CreateDescriptorPool(m_textures, logicalDevice);
		m_material.CreateDescriptorSets(m_textures, m_model, logicalDevice);
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

	void GameObject::CreateTextureResources(WinSys& winSystem, VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
	{
		for (Texture& texture : m_textures)
		{
			texture.CreateTextureImage(winSystem, commandPool, physicalDevice, logicalDevice);
			texture.CreateTextureSampler(physicalDevice, logicalDevice);
		}
	}

	void GameObject::CreateResources(WinSys& winSystem, VkCommandPool commandPool, RenderPass& renderPass, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
	{
		CreateTextureResources(winSystem, commandPool, physicalDevice, logicalDevice);
		CreateModelResources(commandPool, physicalDevice, logicalDevice);
		CreateMaterialResources(logicalDevice, winSystem, renderPass);
	}
}