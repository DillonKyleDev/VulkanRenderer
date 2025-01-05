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
		m_descriptorPools = std::vector<VkDescriptorPool>();
		m_descriptorSets = std::vector<std::vector<VkDescriptorSet>>();
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

	void Material::CreateDescriptorSetLayout(LogicalDevice& logicalDevice)
	{
		// Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
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

	std::vector<std::vector<VkDescriptorSet>> Material::GetDescriptorSets()
	{
		return m_descriptorSets;
	}

	void Material::CreateDescriptorPools(std::vector<Texture> textures, LogicalDevice& logicalDevice)
	{
		for (Texture texture : textures)
		{
			VkDescriptorPool newPool = VK_NULL_HANDLE;

			// Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
			// And for combined sampler - https://vulkan-tutorial.com/en/Texture_mapping/Combined_image_sampler

			std::array<VkDescriptorPoolSize, 2> poolSizes{};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);

			if (vkCreateDescriptorPool(logicalDevice.GetDevice(), &poolInfo, nullptr, &newPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor pool!");
			}

			m_descriptorPools.push_back(newPool);
		}
	}

	void Material::CreateDescriptorSets(std::vector<Texture> textures, Model& model, LogicalDevice& logicalDevice)
	{
		m_descriptorSets.resize(textures.size());

		for (int i = 0; i < textures.size(); i++)
		{
			VkDescriptorSet newSet = VK_NULL_HANDLE;

			std::vector<VkDescriptorSetLayout> layouts(VM_MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descriptorPools[i];
			allocInfo.descriptorSetCount = static_cast<uint32_t>(VM_MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();

			// Allocate descriptor sets
			m_descriptorSets[i].resize(VM_MAX_FRAMES_IN_FLIGHT);
			if (vkAllocateDescriptorSets(logicalDevice.GetDevice(), &allocInfo, m_descriptorSets[i].data()) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor sets!");
			}

			// Populate them
			for (size_t j = 0; j < VM_MAX_FRAMES_IN_FLIGHT; j++)
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = model.GetUniformBuffers()[j];
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(UniformBufferObject);

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = textures[i].GetImageView();
				imageInfo.sampler = textures[i].GetTextureSampler();

				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_descriptorSets[i][j];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_descriptorSets[i][j];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &imageInfo;

				vkUpdateDescriptorSets(logicalDevice.GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
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

	void Material::CleanupDescriptorPools(LogicalDevice& logicalDevice)
	{
		for (VkDescriptorPool descriptorPool : m_descriptorPools)
		{
			vkDestroyDescriptorPool(logicalDevice.GetDevice(), descriptorPool, nullptr);
		}
	}
}