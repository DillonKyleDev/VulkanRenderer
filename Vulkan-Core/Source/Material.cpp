#include "Material.h"
#include "VulkanManager.h"

#include <stdexcept>


namespace VCore
{
	Material::Material(std::string vertexPath, std::string fragmentPath)
	{
		m_graphicsPipeline = GraphicsPipeline(vertexPath, fragmentPath);
		m_descriptorSetLayout = VK_NULL_HANDLE;
		m_textures = std::vector<Texture>();
	}

	Material::Material()
	{
	}

	Material::~Material()
	{
	}

	void Material::CleanupTextures(LogicalDevice& logicalDevice)
	{
		for (Texture texture : m_textures)
		{
			texture.Cleanup(logicalDevice);
		}
	}


	void Material::CreateMaterialResources(WinSys& winSystem, VkCommandPool commandPool, RenderPass& renderPass, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
	{
		CreateTextureResources(winSystem, commandPool, physicalDevice, logicalDevice);
		CreateDescriptorSetLayout(logicalDevice);
		CreateGraphicsPipeline(logicalDevice, winSystem, renderPass);
		// Will need to create descriptor sets (and pools?) when the material is applied to a new model. These are stored on the GameObject
		// CreateDescriptorPool(logicalDevice);
		// CreateDescriptorSets(model, logicalDevice);
	}

	void Material::SetVertexPath(std::string path)
	{
		m_graphicsPipeline.SetVertexPath(path);
	}

	void Material::SetFragmentPath(std::string path)
	{
		m_graphicsPipeline.SetFragmentPath(path);
	}

	void Material::CreateGraphicsPipeline(LogicalDevice& logicalDevice, WinSys& winSystem, RenderPass& renderPass)
	{
		m_graphicsPipeline.CreateGraphicsPipeline(logicalDevice, winSystem, renderPass, m_descriptorSetLayout);
	}

	VkPipeline& Material::GetGraphicsPipeline()
	{
		return m_graphicsPipeline.GetGraphicsPipeline();
	}

	VkPipelineLayout& Material::GetPipelineLayout()
	{
		return m_graphicsPipeline.GetPipelineLayout();
	}

	void Material::CreateDescriptorSetLayout(LogicalDevice& logicalDevice)
	{
		// Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
		std::vector<VkDescriptorSetLayoutBinding> bindings{};

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		bindings.push_back(uboLayoutBinding);

		for (int i = 0; i < m_textures.size(); i++)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = i + 1;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			bindings.push_back(samplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(logicalDevice.GetDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	VkDescriptorSetLayout& Material::GetDescriptorSetLayout()
	{
		return m_descriptorSetLayout;
	}

	void Material::CreateDescriptorPool(VkDescriptorPool& descriptorPool, LogicalDevice& logicalDevice)
	{
		VkDescriptorPool newPool = VK_NULL_HANDLE;

		// Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
		// And for combined sampler - https://vulkan-tutorial.com/en/Texture_mapping/Combined_image_sampler

		std::vector<VkDescriptorPoolSize> poolSizes{};
		poolSizes.resize(m_textures.size() + 1);

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);

		for (int j = 1; j < m_textures.size() + 1; j++)
		{
			poolSizes[j].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[j].descriptorCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(logicalDevice.GetDevice(), &poolInfo, nullptr, &newPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}

		descriptorPool = newPool;
	}

	void Material::CreateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorPool& descriptorPool, Model& model, LogicalDevice& logicalDevice)
	{
		descriptorSets.resize(VM_MAX_FRAMES_IN_FLIGHT);

		std::vector<VkDescriptorSetLayout> layouts(VM_MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		// Allocate descriptor sets
		if (vkAllocateDescriptorSets(logicalDevice.GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (int i = 0; i < VM_MAX_FRAMES_IN_FLIGHT; i++)
		{
			// Populate them
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			descriptorWrites.resize(m_textures.size() + 1);

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = model.GetUniformBuffers()[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			for (size_t j = 0; j < m_textures.size(); j++)
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = m_textures[j].GetImageView();
				imageInfo.sampler = m_textures[j].GetTextureSampler();

				descriptorWrites[j + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j + 1].dstSet = descriptorSets[i];
				descriptorWrites[j + 1].dstBinding = j + 1;
				descriptorWrites[j + 1].dstArrayElement = 0;
				descriptorWrites[j + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[j + 1].descriptorCount = 1;
				descriptorWrites[j + 1].pImageInfo = &imageInfo;
			}

			vkUpdateDescriptorSets(logicalDevice.GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void Material::CleanupGraphicsPipeline(LogicalDevice& logicalDevice)
	{
		m_graphicsPipeline.Cleanup(logicalDevice);
	}

	void Material::CleanupDescriptorSetLayout(LogicalDevice& logicalDevice)
	{
		vkDestroyDescriptorSetLayout(logicalDevice.GetDevice(), m_descriptorSetLayout, nullptr);
	}

	void Material::AddTexture(std::string path)
	{
		Texture newTexture = Texture();
		newTexture.SetTexturePath(path);
		m_textures.push_back(newTexture);
	}

	std::vector<Texture>& Material::GetTextures()
	{
		return m_textures;
	}

	void Material::CreateTextureResources(WinSys& winSystem, VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
	{
		for (Texture& texture : m_textures)
		{
			texture.CreateTextureImage(winSystem, commandPool, physicalDevice, logicalDevice);
			texture.CreateTextureSampler(physicalDevice, logicalDevice);
		}
	}
}