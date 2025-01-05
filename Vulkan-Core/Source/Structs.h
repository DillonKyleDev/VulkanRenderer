#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/hash.hpp>

#include <array>
#include <optional>
#include <vector>

namespace VCore
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily; // Make sure graphics can be rendered
        std::optional<uint32_t> presentFamily; // Make sure device can present images to the surface we created

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        // Creating a swap chain involves a lot more settings than instance and device creation, so we need to query for some more details from our device before we're able to proceed.
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        glm::int16 shaderIndex;

        bool operator==(const Vertex& other) const
        {
            return pos == other.pos && color == other.color && texCoord == other.texCoord && shaderIndex == other.shaderIndex;
        }

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, shaderIndex);

            return attributeDescriptions;
        }
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
}

// Refer to - https://vulkan-tutorial.com/en/Loading_models
template<> struct std::hash<VCore::Vertex> {
    size_t operator()(VCore::Vertex const& vertex) const {
        return ((std::hash<glm::vec3>()(vertex.pos) ^
            (std::hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
            (std::hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};