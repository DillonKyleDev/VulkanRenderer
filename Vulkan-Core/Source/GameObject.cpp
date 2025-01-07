#include "GameObject.h"


namespace VCore
{
	GameObject::GameObject()
	{
		m_model = Model();
		m_material = std::make_shared<Material>();
		m_descriptorPool = VK_NULL_HANDLE;
		m_descriptorSets = std::vector<VkDescriptorSet>();
	}

	GameObject::~GameObject()
	{
	}

	void GameObject::CleanupDescriptorPool(LogicalDevice& logicalDevice)
	{
		vkDestroyDescriptorPool(logicalDevice.GetDevice(), m_descriptorPool, nullptr);
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
	}

	void GameObject::SetMaterial(std::shared_ptr<Material> material)
	{
		m_material = material;
	}

	std::shared_ptr<Material> GameObject::GetMaterial()
	{
		return m_material;
	}

	void GameObject::CreateResources(WinSys& winSystem, VkCommandPool commandPool, RenderPass& renderPass, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
	{
		CreateModelResources(commandPool, physicalDevice, logicalDevice);
		m_material->CreateDescriptorPool(m_descriptorPool, logicalDevice);
		m_material->CreateDescriptorSets(m_descriptorSets, m_descriptorPool, m_model, logicalDevice);
	}

	std::vector<VkDescriptorSet>& GameObject::GetDescriptorSets()
	{
		return m_descriptorSets;
	}
}