#pragma once
#include "Structs.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "WinSys.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <vector>


namespace VCore
{
	class Model
	{
	public:
		Model();
		~Model();

		void SetModelPath(std::string path);
		std::string GetModelPath();
		void LoadModel();
		void CreateVertexBuffer(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void CreateIndexBuffer(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void CreateCommandBuffers(VkCommandPool commandPool, LogicalDevice& logicalDevice);
		void RecordCommandBuffer(uint32_t imageIndex, VkRenderPass renderPass, WinSys& winSystem, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);
		void ResetCommandBuffer();
		void CreateUniformBuffers(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void UpdateUniformBuffer(uint32_t currentImage, WinSys& winSystem);
		VkCommandBuffer& GetCurrentCommandBuffer();
		std::vector<VkBuffer>& GetUniformBuffers();
		void CleanupUniformBuffers(LogicalDevice& logicalDevice);
		void CleanupIndexBuffers(LogicalDevice& logicalDevice);
		void CleanupVertexBuffers(LogicalDevice& logicalDevice);
		

	private:
		std::string m_modelPath;
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		VkBuffer m_vertexBuffer;
		VkBuffer m_indexBuffer;
		VkDeviceMemory m_vertexBufferMemory;
		VkDeviceMemory m_indexBufferMemory;
		std::vector<VkBuffer> m_uniformBuffers;
		std::vector<VkDeviceMemory> m_uniformBuffersMemory;
		std::vector<void*> m_uniformBuffersMapped;
		std::vector<VkCommandBuffer> m_commandBuffer;
	};
}

