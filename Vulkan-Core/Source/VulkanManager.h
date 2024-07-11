#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <glm.hpp>

// standard library
#include <iostream>
#include <vector>
#include <optional>
#include <array>


namespace VCore
{
    // SEQUENCE OF EVENTS //
    /*
    // Create a GLFW window
    // Initialize Vulkan
    // Create Vulkan instance
    // Set up Validation layers debugging
    // Create window surface using GLFW
    // Pick physical GPU device
    // Create logical device to interface with that GPU
    // Create swapchain to draw to images
    // Set up images to be drawn to (VkImageViews object)
    // Create and setup graphics pipeline configuration (lengthy) - VVV
  
    // Graphics Pipeline =
    // Shader stages : the shader modules that define the functionality of the programmable stages of the graphics pipeline
    // Fixed-function state : all of the structures that define the fixed - function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    // Pipeline layout : the uniform and push values referenced by the shader that can be updated at draw time
    // Render pass : the attachments referenced by the pipeline stages and their usage
     

    // Set up a command pool
    // Set up command buffers for imageViews 
  
    // Once the setup above is complete, steps to draw frame:

    // Wait for the previous frame to finish
    // Acquire an image from the swap chain
    // Record a command buffer which draws the scene onto that image
    // Submit the recorded command buffer
    // Present the swap chain image


    // Events to order explicitly because the happen on the GPU:

    // Acquire an image from the swap chain
    // Execute commands that draw onto the acquired image
    // Present that image to the screen for presentation, returning it to the swapchain

    
    // Other things yet to come
    // ...
    // 
    // Cleanup and destroy all resources created
    // Quit
    */

    class VulkanManager
    {
    public:
        struct QueueFamilyIndices {
            // Because graphics family indices can have all values an uint32_t can have, use optional as a way to check if it has been initialized: graphicsFamily.has_value()
            std::optional<uint32_t> graphicsFamily; // Make sure graphics can be rendered
            std::optional<uint32_t> presentFamily; // Make sure device can present images to the surface we created

            bool isComplete() {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        struct SwapChainSupportDetails {
            // Creating a swap chain involves a lot more settings than instance and device creation, so we need to query for some more details from our device before we're able to proceed.
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        // Refer to - https://vulkan-tutorial.com/en/Vertex_buffers/Vertex_input_description
        struct Vertex {
            glm::vec2 pos;
            glm::vec3 color;

            static VkVertexInputBindingDescription getBindingDescription()
            {
                // The binding parameter specifies the index of the binding in the array of bindings. The stride parameter specifies the number of bytes from one entry to the next
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            };

            static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
                // describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description
                std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);

                return attributeDescriptions;
            };
        };

        // Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
        // And for bit alignment - https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
        // https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };

        VulkanManager();
        ~VulkanManager();
        void run(bool &_quit);

    private:
        void initWindow();
        void initVulkan();
        void createInstance();
        void createSurface();
        void mainLoop(bool& _quit);

        void pickPhysicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        static int rateDeviceSuitability(VkPhysicalDevice device);
        void createLogicalDevice();

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        // NOTE FROM WIKI: If the swapChainAdequate conditions were met then the support is definitely sufficient, but there may still be many different modes of varying optimality. We'll now write a couple of functions to find the right settings for the best possible swap chain. There are three types of settings to determine:
        // Surface format (color depth)
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        // Presentation mode (conditions for "swapping" images to the screen)
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        // Swap extent (resolution of images in swap chain)
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void createSwapChain();
        void recreateSwapChain();
        void cleanupSwapChain();
        void createImageViews();
        void createFramebuffers();

        void createRenderPass();
        void createGraphicsPipeline();
        VkShaderModule createShaderModule(const std::vector<char>& code);
        static std::vector<char> readFile(const std::string& filename);

        void createCommandPool();
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void createVertexBuffer();
        void createIndexBuffer();
        void createCommandBuffers();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncObjects();
        void drawFrame();

        void createTextureImage();
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        void createDescriptorSetLayout();
        void createUniformBuffers();
        void updateUniformBuffer(uint32_t currentImage);
        void createDescriptorPool();
        void createDescriptorSets();

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void cleanup();


        ///////////////////////
        // Validation layers // 
        ///////////////////////
        void setupDebugMessenger();
        bool checkValidationLayerSupport();
        std::vector<const char*> getRequiredExtensions(); // Get extensions for debug validation layers
        // Create info for debug messenger so we don't have to repeat code for debug messenger for vkCreateInstance and vkDestroyInstance calls
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        ///////////////////////


        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        GLFWwindow* m_window;
        VkSurfaceKHR m_surface;
        VkQueue m_presentQueue;

        VkPhysicalDevice m_physicalDevice;
        VkPhysicalDeviceProperties m_physicalDeviceProperties; // type, name, etc...
        VkDevice m_device; // logical device to interface with m_physicalDevice

        VkQueue m_graphicsQueue;
        VkSwapchainKHR m_swapChain;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImage> m_swapChainImages;
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        VkRenderPass m_renderPass;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorPool m_descriptorPool;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_graphicsPipeline;

        VkBuffer m_vertexBuffer;
        VkBuffer m_indexBuffer;
        VkDeviceMemory m_vertexBufferMemory;
        VkDeviceMemory m_indexBufferMemory;

        std::vector<VkBuffer> m_uniformBuffers;
        std::vector<VkDeviceMemory> m_uniformBuffersMemory;
        std::vector<void*> m_uniformBuffersMapped;

        VkImage m_textureImage;
        VkDeviceMemory m_textureImageMemory;

        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffer;
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFence;
        uint32_t m_currentFrame;

        bool m_b_framebufferResized;

        const int m_MAX_FRAMES_IN_FLIGHT = 2;
        const int m_WIDTH = 800;
        const int m_HEIGHT = 600;



        const std::vector<Vertex> m_vertices = 
        {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };

        const std::vector<uint16_t> m_indices = {
            0, 1, 2, 2, 3, 0
        };



        // For device extensions required to present images to the window system (swap chain usage)
        const std::vector<const char*> m_deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        // For our validation layers
        const std::vector<const char*> m_validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        #ifdef NDEBUG
            const bool m_b_enableValidationLayers = false;
        #else
            const bool m_b_enableValidationLayers = true;
        #endif
    };
}
