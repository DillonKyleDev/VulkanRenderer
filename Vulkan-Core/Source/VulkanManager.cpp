#pragma once
#include "VulkanManager.h"
#include "Helper.h"

// Refer to - https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer
#include <gtc/matrix_transform.hpp> // Not used currently but might need it later

#include <chrono> // Time keeping


namespace VCore
{
    ValidationLayers VM_validationLayers = ValidationLayers();
    uint32_t VM_currentFrame = 0;

    VulkanManager::VulkanManager()
    {
        m_instance = VK_NULL_HANDLE;
        m_winSystem = WinSys();
        m_physicalDevice = PhysicalDevice();
        m_logicalDevice = LogicalDevice();
        m_renderPass = RenderPass();

        m_vikingRoom = GameObject();
        Material roomMaterial = Material("../Shaders/vert.spv", "../Shaders/frag.spv");
        m_vikingRoom.SetMaterial(roomMaterial);
        m_vikingRoom.GetModel().SetModelPath("../Models/viking_room.obj");
        m_vikingRoom.AddTexture("../Textures/viking_room.png");

        m_ghostHand = GameObject();
        Material handMaterial = Material("../Shaders/vert.spv", "../Shaders/frag.spv");
        m_ghostHand.SetMaterial(handMaterial);
        m_ghostHand.GetModel().SetModelPath("../Models/viking_room.obj");
        m_ghostHand.AddTexture("../Textures/viking_room.png");

        // gpu communication
        m_commandPool = VK_NULL_HANDLE;
        m_imageAvailableSemaphore = std::vector<VkSemaphore>();
        m_renderFinishedSemaphore = std::vector<VkSemaphore>();
        m_inFlightFence = std::vector<VkFence>();
        m_b_framebufferResized = false;
    }

    VulkanManager::~VulkanManager()
    {
    }

    void VulkanManager::Run(bool& _quit)
    {
        m_winSystem.InitWindow();
        InitVulkan();
        MainLoop(_quit);
        Cleanup();
    }

    void VulkanManager::InitVulkan()
    {
        CreateInstance();
        VM_validationLayers.SetupDebugMessenger(m_instance);
        m_winSystem.CreateSurface(m_instance);
        m_physicalDevice.Init(m_instance, m_winSystem.GetSurface());
        m_logicalDevice.Init(m_physicalDevice, m_winSystem.GetSurface());
        m_winSystem.CreateSwapChain(m_physicalDevice, m_logicalDevice);
        m_winSystem.CreateImageViews(m_logicalDevice);
        m_renderPass.CreateRenderPass(m_winSystem, m_physicalDevice, m_logicalDevice);
        m_winSystem.CreateColorResources(m_physicalDevice, m_logicalDevice);
        m_winSystem.CreateDepthResources(m_physicalDevice, m_logicalDevice);
        m_winSystem.CreateFramebuffers(m_logicalDevice, m_renderPass.GetRenderPass());

        CreateCommandPool();

        m_vikingRoom.CreateResources(m_winSystem, m_commandPool, m_renderPass, m_physicalDevice, m_logicalDevice);   
        m_ghostHand.CreateResources(m_winSystem, m_commandPool, m_renderPass, m_physicalDevice, m_logicalDevice);

        CreateSyncObjects();
    }

    void VulkanManager::CreateInstance()
    {
        // Validation layer setup for debugger
        if (b_ENABLE_VALIDATION_LAYERS && !VM_validationLayers.CheckSupport())
        {
            throw std::runtime_error("validation layers requested, but not available.");
        }

        // Some details about our application
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Struct to tell Vulkan which global extensions and validation layers we want to use
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo; // Reference our VkApplicationInfo struct above

        // Must get an extenstion to work with the window because Vulkan is platform agnostic
        uint32_t ui_glfwExtensionCount = 0;
        const char** glfwExtensions;

        // glfws built in function to get extensions needed for our platform
        glfwExtensions = glfwGetRequiredInstanceExtensions(&ui_glfwExtensionCount);

        createInfo.enabledExtensionCount = ui_glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;


        // NOTE FROM THE WIKI: The debugCreateInfo variable is placed outside the if statement to ensure that it is not destroyed before the vkCreateInstance call.By creating an additional debug messenger this way it will automatically be used during vkCreateInstance and vkDestroyInstance and cleaned up after that.
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // DebugUtilsMessenger for CreateInstance and DestroyInstance functions (automatically destroyed by Vulkan when closed)

        // If validation layers are enabled, include validation layer names in the VKInstanceCreateInfo struct
        if (b_ENABLE_VALIDATION_LAYERS)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(VM_validationLayers.Size());
            createInfo.ppEnabledLayerNames = VM_validationLayers.Data();

            // Create debug info for special debug messenger for vkCreateInstance and vkDestroyInstance calls
            // (because the other debug messenger needs them to already have been called to function)
            VM_validationLayers.PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }


        // Get extensions for use with debug messenger
        auto extensions = VM_validationLayers.GetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();


        // Now we have everything set up for Vulkan to create an instance and can call ckCreateInstance
        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance...");
        }
    }

    void VulkanManager::MainLoop(bool& _quit)
    {
        // Window is completely controlled by glfw, including closing using the "x" button on top right (we need event handling)
        while (!glfwWindowShouldClose(m_winSystem.GetWindow()))
        {
            glfwPollEvents();
            DrawFrame();
        }

        //_quit = glfwWindowShouldClose(m_winSystem.GetWindow());
        vkDeviceWaitIdle(m_logicalDevice.GetDevice());
    }

    void VulkanManager::CreateCommandPool()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
        // Need a command pool to store all commands to send in bulk for Vulkan to batch process together
        QueueFamilyIndices queueFamilyIndices = Helper::FindQueueFamilies(m_physicalDevice.GetDevice(), m_winSystem.GetSurface());

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        // Command buffers are executed by submitting them on one of the device queues. Each command pool can only allocate command buffers that are submitted on a single type of queue. We're going to record commands for drawing, which is why we've chosen the graphics queue family.
        if (vkCreateCommandPool(m_logicalDevice.GetDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void VulkanManager::CreateSyncObjects()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
       
        // Create semaphore info
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        // Create fence info
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start it signaled so the very first frame doesn't block indefinitely

        m_imageAvailableSemaphore.resize(VM_MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphore.resize(VM_MAX_FRAMES_IN_FLIGHT);
        m_inFlightFence.resize(VM_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < VM_MAX_FRAMES_IN_FLIGHT; i++)
        {
            // Create semaphores and fences
            if (vkCreateSemaphore(m_logicalDevice.GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_logicalDevice.GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore[i]) != VK_SUCCESS ||
                vkCreateFence(m_logicalDevice.GetDevice(), &fenceInfo, nullptr, &m_inFlightFence[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }

    void VulkanManager::DrawFrame()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
        
        // At the start of the frame, we want to wait until the previous frame has finished, so that the command buffer and semaphores are available to use. To do that, we call vkWaitForFences:
        vkWaitForFences(m_logicalDevice.GetDevice(), 1, &m_inFlightFence[VM_currentFrame], VK_TRUE, UINT64_MAX);

        // acquire an image from the swap chain
        uint32_t imageIndex;
        //vkDeviceWaitIdle(m_logicalDevice.GetDevice()); // Waits for the gpu to be done with current frame
        VkResult result = vkAcquireNextImageKHR(m_logicalDevice.GetDevice(), m_winSystem.GetSwapChain(), UINT64_MAX, m_imageAvailableSemaphore[VM_currentFrame], VK_NULL_HANDLE, &imageIndex);

        // More details here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Swap_chain_recreation
        // Check on swap chain integrity after image access
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_b_framebufferResized)
        {
            m_b_framebufferResized = false;
            m_winSystem.RecreateSwapChain(m_logicalDevice, m_physicalDevice, m_renderPass.GetRenderPass()); // THIS IS BROKEN AND SEMAPHORE DOESN'T WORK ON NEXT AQUIREIMAGEKHR CALL ON RESIZE
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
        {
            throw std::runtime_error("failed to acquire swap chain image.");
        }

        // After waiting && checking the integrity/recreating the swap chain if needed, we need to manually reset the fence to the unsignaled state with the vkResetFences call:
        vkResetFences(m_logicalDevice.GetDevice(), 1, &m_inFlightFence[VM_currentFrame]);

        // 
        // Record the command buffer
        // Reset to make sure it is able to be recorded
        m_vikingRoom.GetModel().ResetCommandBuffer();
        m_ghostHand.GetModel().ResetCommandBuffer();
        m_vikingRoom.GetModel().RecordCommandBuffer(imageIndex, m_renderPass.GetRenderPass(), m_winSystem, m_vikingRoom.GetMaterial().GetGraphicsPipeline(), m_vikingRoom.GetMaterial().GetPipelineLayout(), m_vikingRoom.GetMaterial().GetDescriptorSets()[VM_currentFrame]);
        m_ghostHand.GetModel().RecordCommandBuffer(imageIndex, m_renderPass.GetRenderPass(), m_winSystem, m_vikingRoom.GetMaterial().GetGraphicsPipeline(), m_vikingRoom.GetMaterial().GetPipelineLayout(), m_ghostHand.GetMaterial().GetDescriptorSets()[VM_currentFrame]);
        m_vikingRoom.GetModel().UpdateUniformBuffer(VM_currentFrame, m_winSystem, 0);
        m_ghostHand.GetModel().UpdateUniformBuffer(VM_currentFrame, m_winSystem, 1.0f);

        VkCommandBuffer commandBuffers[2] = { m_vikingRoom.GetModel().GetCommandBuffer()[VM_currentFrame], m_ghostHand.GetModel().GetCommandBuffer()[VM_currentFrame] };
        // Submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[VM_currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 2;
        submitInfo.pCommandBuffers = commandBuffers;

        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore[VM_currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;


        if (vkQueueSubmit(m_logicalDevice.GetGraphicsQueue(), 1, &submitInfo, m_inFlightFence[VM_currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer.");
        }


        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_winSystem.GetSwapChain() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        // Present!
        vkQueuePresentKHR(m_logicalDevice.GetPresentQueue(), &presentInfo);

        // Check on swap chain integrity after present
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_winSystem.RecreateSwapChain(m_logicalDevice, m_physicalDevice, m_renderPass.GetRenderPass());
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to present swap chain image.");
        }
        // Advance the VM_currentFrame
        VM_currentFrame = (VM_currentFrame + 1) % VM_MAX_FRAMES_IN_FLIGHT;
    }


    void VulkanManager::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        // Called when GLFW detects the window has been resized.  It has a pointer to our app that we gave it in initWindow that we use here to set our m_b_framebufferResized member to true
        auto app = reinterpret_cast<VulkanManager*>(glfwGetWindowUserPointer(window));
        app->m_b_framebufferResized = true;
    }


    void VulkanManager::Cleanup()
    {
        // Make sure to cleanup all other resources BEFORE the Vulkan m_instance is destroyed (in order of creation if possible)

        // Semaphores and Fences
        for (size_t i = 0; i < VM_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(m_logicalDevice.GetDevice(), m_imageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(m_logicalDevice.GetDevice(), m_renderFinishedSemaphore[i], nullptr);
            vkDestroyFence(m_logicalDevice.GetDevice(), m_inFlightFence[i], nullptr);
        }
        
        // Swap Chain resources
        m_winSystem.CleanupSwapChain(m_logicalDevice);

        // Texture resources
        for (Texture &texture : m_vikingRoom.GetTextures())
        {
            texture.Cleanup(m_logicalDevice);
        }
       
        m_vikingRoom.GetModel().CleanupUniformBuffers(m_logicalDevice);

        // Descriptor Pool & sets implicitly  
        m_vikingRoom.GetMaterial().CleanupDescriptorPool(m_logicalDevice);
        m_vikingRoom.GetMaterial().CleanupDescriptorSetLayout(m_logicalDevice);
       
        m_vikingRoom.GetModel().CleanupIndexBuffers(m_logicalDevice);
        m_vikingRoom.GetModel().CleanupVertexBuffers(m_logicalDevice);

        // Command Pool
        vkDestroyCommandPool(m_logicalDevice.GetDevice(), m_commandPool, nullptr);

        m_vikingRoom.GetMaterial().CleanupGraphicsPipeline(m_logicalDevice);

        m_renderPass.Cleanup(m_logicalDevice);
        m_logicalDevice.Cleanup();
        m_physicalDevice.Cleanup();
        VM_validationLayers.Cleanup(m_instance);
        m_winSystem.Cleanup(m_instance);
 
        // Destroy Vulkan instance
        vkDestroyInstance(m_instance, nullptr);
    }
}