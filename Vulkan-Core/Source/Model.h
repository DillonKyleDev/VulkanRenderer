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
		void CleanupUniformBuffers(LogicalDevice& logicalDevice);
		void CleanupIndexBuffers(LogicalDevice& logicalDevice);
		void CleanupVertexBuffers(LogicalDevice& logicalDevice);

		void SetModelPath(std::string path);
		std::string GetModelPath();
		void LoadModel();
		void CreateVertexBuffer(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void CreateIndexBuffer(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void CreateUniformBuffers(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void UpdateUniformBuffer(uint32_t currentImage, WinSys& winSystem, float multiplier);
		std::vector<VkCommandBuffer>& GetCommandBuffer();
		std::vector<VkBuffer>& GetUniformBuffers();
		VkBuffer& GetVertexBuffer();
		VkBuffer& GetIndexBuffer();		
		std::vector<uint32_t>& GetIndices();

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

