#include "Model.h"
#include "WinSys.h"
#include "VulkanManager.h"

#define TINYOBJLOADER_IMPLEMENTATION // Loading obj files
#include "tiny_obj_loader.h"

#include <stdexcept>
#include <chrono>


namespace VCore
{
	Model::Model()
	{
        m_modelPath = "";
        m_vertices = std::vector<Vertex>();
        m_indices = std::vector<uint32_t>();
        m_vertexBuffer = VK_NULL_HANDLE;
        m_indexBuffer = VK_NULL_HANDLE;
        m_indexBufferMemory = VK_NULL_HANDLE;
        m_vertexBufferMemory = VK_NULL_HANDLE;
        m_uniformBuffers = std::vector<VkBuffer>();
        m_uniformBuffersMemory = std::vector<VkDeviceMemory>();
        m_uniformBuffersMapped = std::vector<void*>();
        m_commandBuffer = std::vector<VkCommandBuffer>();
	}

    Model::~Model()
    {
    }

    void Model::SetModelPath(std::string path)
    {
        m_modelPath = path;
    }

    std::string Model::GetModelPath()
    {
        return m_modelPath;
    }

    void Model::LoadModel()
    {
        // Refer to - https://vulkan-tutorial.com/en/Loading_models

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m_modelPath.c_str()))
        {
            throw std::runtime_error(warn + err);
        }
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = { 1.0f, 1.0f, 1.0f };

                // Keep only unique vertices
                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void Model::CreateVertexBuffer(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
    {
        // Create staging buffer for control from the cpu
        // Refer to - https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer

        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        WinSys::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, physicalDevice, logicalDevice);

        void* data;
        vkMapMemory(logicalDevice.GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(logicalDevice.GetDevice(), stagingBufferMemory);

        // Create device local vertex buffer for actual buffer
        WinSys::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory, physicalDevice, logicalDevice);

        // We can now call copyBuffer function to move the vertex data to the device local buffer:
        WinSys::CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize, commandPool, logicalDevice);

        // After copying the data from the staging buffer to the device buffer, we should clean it up:
        vkDestroyBuffer(logicalDevice.GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), stagingBufferMemory, nullptr);
    }

    void Model::CreateIndexBuffer(VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
    {
        // Refer to - https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer

        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        WinSys::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, physicalDevice, logicalDevice);

        void* data;
        vkMapMemory(logicalDevice.GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_indices.data(), (size_t)bufferSize);
        vkUnmapMemory(logicalDevice.GetDevice(), stagingBufferMemory);

        WinSys::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory, physicalDevice, logicalDevice);

        WinSys::CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize, commandPool, logicalDevice);

        vkDestroyBuffer(logicalDevice.GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), stagingBufferMemory, nullptr);
    }

    void Model::CreateCommandBuffers(VkCommandPool commandPool, LogicalDevice& logicalDevice)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = VM_MAX_FRAMES_IN_FLIGHT;

        m_commandBuffer.resize(VM_MAX_FRAMES_IN_FLIGHT);

        if (vkAllocateCommandBuffers(logicalDevice.GetDevice(), &allocInfo, m_commandBuffer.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void Model::RecordCommandBuffer(uint32_t imageIndex, VkRenderPass &renderPass, WinSys& winSystem, VkPipeline& graphicsPipeline, VkPipelineLayout& pipelineLayout, VkDescriptorSet& descriptorSet)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(m_commandBuffer[VM_currentFrame], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // For Depth buffer
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };        

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = winSystem.GetFrameBuffers()[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = winSystem.GetExtent();
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();        

        // Begin render pass
        vkCmdBeginRenderPass(m_commandBuffer[VM_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // Bind the graphics pipeline
        vkCmdBindPipeline(m_commandBuffer[VM_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


        // Bind the vertex buffer
        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffer[VM_currentFrame], 0, 1, vertexBuffers, offsets);

        // Bind the index buffer
        vkCmdBindIndexBuffer(m_commandBuffer[VM_currentFrame], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);


        // we did specify viewport and scissor state for this pipeline to be dynamic. So we need to set them in the command buffer before issuing our draw command:
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(winSystem.GetExtent().width);
        viewport.height = static_cast<float>(winSystem.GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_commandBuffer[VM_currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = winSystem.GetExtent();
        vkCmdSetScissor(m_commandBuffer[VM_currentFrame], 0, 1, &scissor);

        vkCmdBindDescriptorSets(m_commandBuffer[VM_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // Refer to - https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer
        vkCmdDrawIndexed(m_commandBuffer[VM_currentFrame], static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0); // reusing vertices with index buffers.
        // NOTE FROM THE WIKI: The previous chapter already mentioned that you should allocate multiple resources like buffers from a single memory allocation, but in fact you should go a step further. Driver developers recommend that you also store multiple buffers, like the vertex and index buffer, into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers. The advantage is that your data is more cache friendly in that case, because it's closer together. It is even possible to reuse the same chunk of memory for multiple resources if they are not used during the same render operations, provided that their data is refreshed, of course. This is known as aliasing and some Vulkan functions have explicit flags to specify that you want to do this.

        // End render pass
        vkCmdEndRenderPass(m_commandBuffer[VM_currentFrame]);

        // Finish recording the command buffer
        if (vkEndCommandBuffer(m_commandBuffer[VM_currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void Model::ResetCommandBuffer()
    {
        vkResetCommandBuffer(m_commandBuffer[VM_currentFrame], 0);
    }

    void Model::CreateUniformBuffers(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice) 
    {
        // Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer

        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(VM_MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.resize(VM_MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.resize(VM_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < VM_MAX_FRAMES_IN_FLIGHT; i++)
        {
            WinSys::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i], physicalDevice, logicalDevice);

            vkMapMemory(logicalDevice.GetDevice(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
        }
    }

    void Model::UpdateUniformBuffer(uint32_t currentImage, WinSys& winSystem, float multiplier)
    {
        // This function will generate a new transformation every frame to make the geometry spin around.We need to include two new headers to implement this functionality:
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * multiplier * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), winSystem.GetExtent().width / (float)winSystem.GetExtent().height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    std::vector<VkCommandBuffer>& Model::GetCommandBuffer()
    {
        return m_commandBuffer;
    }

    std::vector<VkBuffer>& Model::GetUniformBuffers()
    {
        return m_uniformBuffers;
    }

    void Model::CleanupUniformBuffers(LogicalDevice &logicalDevice)
    {
        for (size_t i = 0; i < VM_MAX_FRAMES_IN_FLIGHT; i++) 
        {
            vkDestroyBuffer(logicalDevice.GetDevice(), m_uniformBuffers[i], nullptr);
            vkFreeMemory(logicalDevice.GetDevice(), m_uniformBuffersMemory[i], nullptr);
        }
    }

    void Model::CleanupIndexBuffers(LogicalDevice& logicalDevice)
    {
        vkDestroyBuffer(logicalDevice.GetDevice(), m_indexBuffer, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), m_indexBufferMemory, nullptr);
    }

    void Model::CleanupVertexBuffers(LogicalDevice& logicalDevice)
    {
        vkDestroyBuffer(logicalDevice.GetDevice(), m_vertexBuffer, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), m_vertexBufferMemory, nullptr);
    }
}