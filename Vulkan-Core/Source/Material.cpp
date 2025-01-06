#include "Material.h"
#include "VulkanManager.h"

#include <array>
#include <stdexcept>


namespace VCore
{
	Material::Material(std::string vertexPath, std::string fragmentPath)
	{
		m_graphicsPipeline = GraphicsPipeline(vertexPath, fragmentPath);
		m_descriptorSetLayout = VK_NULL_HANDLE;
		m_descriptorPool = VK_NULL_HANDLE;
		m_descriptorSets = std::vector<VkDescriptorSet>();
	}

	Material::Material()
	{
	}

	Material::~Material()
	{
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

	void Material::CreateDescriptorSetLayout(std::vector<Texture>& textures, LogicalDevice& logicalDevice)
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

		for (int i = 0; i < textures.size(); i++)
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

	std::vector<VkDescriptorSet>& Material::GetDescriptorSets()
	{
		return m_descriptorSets;
	}

	void Material::CreateDescriptorPool(std::vector<Texture>& textures, LogicalDevice& logicalDevice)
	{
			VkDescriptorPool newPool = VK_NULL_HANDLE;

			// Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
			// And for combined sampler - https://vulkan-tutorial.com/en/Texture_mapping/Combined_image_sampler

			std::vector<VkDescriptorPoolSize> poolSizes{};
			poolSizes.resize(textures.size() + 1);

			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);

			for (int j = 1; j < textures.size() + 1; j++)
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

			m_descriptorPool = newPool;
	}

	void Material::CreateDescriptorSets(std::vector<Texture>& textures, Model& model, LogicalDevice& logicalDevice)
	{
		m_descriptorSets.resize(VM_MAX_FRAMES_IN_FLIGHT);

		std::vector<VkDescriptorSetLayout> layouts(VM_MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		// Allocate descriptor sets
		if (vkAllocateDescriptorSets(logicalDevice.GetDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (int i = 0; i < VM_MAX_FRAMES_IN_FLIGHT; i++)
		{
			// Populate them
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			descriptorWrites.resize(textures.size() + 1);

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = model.GetUniformBuffers()[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			for (size_t j = 0; j < textures.size(); j++)
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = textures[j].GetImageView();
				imageInfo.sampler = textures[j].GetTextureSampler();

				descriptorWrites[j + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j + 1].dstSet = m_descriptorSets[i];
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

	void Material::CleanupDescriptorPool(LogicalDevice& logicalDevice)
	{
		vkDestroyDescriptorPool(logicalDevice.GetDevice(), m_descriptorPool, nullptr);
	}
}