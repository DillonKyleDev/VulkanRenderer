#pragma once
#include "Structs.h"
#include "ValidationLayers.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "WinSys.h"
#include "Model.h"
#include "RenderPass.h"
#include "Material.h"
#include "GameObject.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <glm.hpp>

#include <vector>
#include <string>


namespace VCore
{
    // For device extensions required to present images to the window system (swap chain usage)
    const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    #ifdef NDEBUG
        const bool b_ENABLE_VALIDATION_LAYERS = false;
    #else
        const bool b_ENABLE_VALIDATION_LAYERS = true;
    #endif

    extern ValidationLayers VM_validationLayers;
    const int VM_MAX_FRAMES_IN_FLIGHT = 2;
    extern uint32_t VM_currentFrame;

    class VulkanManager
    {
    public:
        VulkanManager();
        ~VulkanManager();
        void run(bool &_quit);

        // Was private, moved to public for WinSys
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    private:
        void initVulkan();
        void createInstance();
        void mainLoop(bool& _quit);

        void createCommandPool();
        void createSyncObjects();
        void drawFrame();

        void cleanup();


        GameObject m_vikingRoom;

        VkInstance m_instance;
        WinSys m_winSystem;
        
        // devices
        PhysicalDevice m_physicalDevice;
        LogicalDevice m_logicalDevice;
        RenderPass m_renderPass;

        bool m_b_framebufferResized;
        
        // gpu communication
        VkCommandPool m_commandPool;
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFence;

        const std::string m_MODEL_PATH = "../Models/viking_room.obj";
        const std::string m_MODEL_PATH2 = "../Models/ssb.obj";
        const std::string m_TEXTURE_PATH = "../Textures/viking_room.png";
        const std::string m_TEXTURE_PATH2 = "../Textures/ssb.jpeg";
    };
}