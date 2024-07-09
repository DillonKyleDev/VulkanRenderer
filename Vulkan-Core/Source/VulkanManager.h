#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

// standard library
#include <iostream>
#include <vector>
#include <optional>

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
        void createImageViews();
        void createFramebuffers();

        void createRenderPass();
        void createGraphicsPipeline();
        VkShaderModule createShaderModule(const std::vector<char>& code);
        static std::vector<char> readFile(const std::string& filename);

        void createCommandPool();
        void createCommandBuffer();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncObjects();
        void drawFrame();

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
        VkRenderPass m_renderPass;
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_graphicsPipeline;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;
        VkCommandPool m_commandPool;
        VkCommandBuffer m_commandBuffer;
        VkSemaphore m_imageAvailableSemaphore;
        VkSemaphore m_renderFinishedSemaphore;
        VkFence m_inFlightFence;

 
        const int m_WIDTH = 800;
        const int m_HEIGHT = 600;

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
