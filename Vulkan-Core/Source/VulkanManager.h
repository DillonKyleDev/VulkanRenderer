#pragma once
#include "Structs.h"
#include "ValidationLayers.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "WinSys.h"
#include "RenderPass.h"
#include "GameObject.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <glm.hpp>

#include <vector>
#include <map>


namespace VCore
{
    // For device extensions required to present images to the window system (swap chain usage)
    const std::vector<const char*> DEVICE_EXTENSIONS = 
    {
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
        void Run(bool &_quit);

        // Was private, moved to public for WinSys
        static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

    private:
        void InitVulkan();
        void CreateInstance();
        void MainLoop(bool& _quit);
        void CreateCommandPool();
        void CreateSyncObjects();
        void DrawFrame();
        void Cleanup();

        std::vector<GameObject> m_gameObjects;
        std::map<std::string, std::shared_ptr<Material>> m_materials;
        VkInstance m_instance;
        WinSys m_winSystem;
        PhysicalDevice m_physicalDevice;
        LogicalDevice m_logicalDevice;
        RenderPass m_renderPass;
        bool m_b_framebufferResized;
        VkCommandPool m_commandPool;
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFence;
    };
}