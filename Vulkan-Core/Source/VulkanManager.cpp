#pragma once
#include "VulkanManager.h"

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <set>
#include <fstream>


namespace VCore
{
    // Public
    VulkanManager::VulkanManager()
    {
        m_instance = VK_NULL_HANDLE;
        m_debugMessenger = VK_NULL_HANDLE;
        m_window = VK_NULL_HANDLE;
        m_surface = VK_NULL_HANDLE;
        m_presentQueue = VK_NULL_HANDLE;
        m_physicalDevice = VK_NULL_HANDLE; // implicitly destroyed when VkInstance is destroyed
        m_physicalDeviceProperties = VkPhysicalDeviceProperties();
        m_device = VK_NULL_HANDLE;
        m_graphicsQueue = VK_NULL_HANDLE; // implicitly destroyed when VkDevice is destroyed
        m_swapChain = VK_NULL_HANDLE;
        m_swapChainImageFormat = VkFormat();
        m_swapChainExtent = VkExtent2D();
        m_swapChainImages = std::vector<VkImage>(); // automatically cleaned up once the swap chain has been destroyed
        m_swapChainImageViews = std::vector<VkImageView>();
        m_renderPass = VK_NULL_HANDLE;
        m_pipelineLayout = VK_NULL_HANDLE;
        m_graphicsPipeline = VK_NULL_HANDLE;
        m_swapChainFramebuffers = std::vector<VkFramebuffer>();
        m_commandPool = VK_NULL_HANDLE;
        m_commandBuffer = VK_NULL_HANDLE;
        m_imageAvailableSemaphore = VK_NULL_HANDLE;
        m_renderFinishedSemaphore = VK_NULL_HANDLE;
        m_inFlightFence = VK_NULL_HANDLE;
    }

    VulkanManager::~VulkanManager()
    {
    }

    void VulkanManager::run(bool& _quit)
    {
        initWindow();
        initVulkan();
        mainLoop(_quit);
        cleanup();
    }

    // Private
    void VulkanManager::initWindow()
    {
        // initialize glfw
        glfwInit();

        // Tell glfw not to create OpenGL context with init call
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Disable window resizing for now
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Create our window
        // glfwCreateWindow(width, height, title, monitorToOpenOn, onlyUsedForOpenGL)
        m_window = glfwCreateWindow(m_WIDTH, m_HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void VulkanManager::initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
    }

    void VulkanManager::createInstance()
    {
        // Validation layer setup for debugger
        if (m_b_enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available.");
        }

        // Some details about our application
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Render Triangle";
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
        if (m_b_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            // Create debug info for special debug messenger for vkCreateInstance and vkDestroyInstance calls
            // (because the other debug messenger needs them to already have been called to function)
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }


        // Get extensions for use with debug messenger
        auto extensions = VulkanManager::getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();


        // Now we have everything set up for Vulkan to create an instance and can call ckCreateInstance
        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance...");
        }
        else
        {
            std::cout << "Vulkan instance successfully created." << std::endl;
        }
    }

    void VulkanManager::createSurface()
    {
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface.");
        }
        else
        {
            std::cout << "Window surface successfully created." << std::endl;
        }
    }

    void VulkanManager::mainLoop(bool& _quit)
    {
        // Window is completely controlled by glfw, including closing using the "x" button on top right (we need event handling)
        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            drawFrame();
        }

        //_quit = glfwWindowShouldClose(m_window);
        vkDeviceWaitIdle(m_device);
    }


    void VulkanManager::pickPhysicalDevice()
    {
        // Get number of physical devices (graphics cards available)
        uint32_t ui_deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &ui_deviceCount, nullptr);

        if (ui_deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support.");
        }
        else
        {
            // Collect all available physical devices into devices vector
            std::vector<VkPhysicalDevice> devices(ui_deviceCount);
            vkEnumeratePhysicalDevices(m_instance, &ui_deviceCount, devices.data());

            // Check if any of the devices are suitable for our needs
            for (const auto& device : devices)
            {
                // Here we are just taking the very first suitable device and going with it
                if (isDeviceSuitable(device))
                {
                    vkGetPhysicalDeviceProperties(device, &m_physicalDeviceProperties);
                    std::cout << "Suitable GPU found: " << m_physicalDeviceProperties.deviceName << std::endl;
                    m_physicalDevice = device;
                    break;
                }
            }

            if (m_physicalDevice == VK_NULL_HANDLE)
            {
                throw std::runtime_error("failed to find a suitable GPU.");
            }

            // For use when ranking physical devices using rateDeviceSuitability
            // Use an ordered map to automatically sort candidates by increasing score
            //std::multimap<int, VkPhysicalDevice> candidates;

            //for (const auto& device : devices) {
            //    int score = rateDeviceSuitability(device);
            //    candidates.insert(std::make_pair(score, device));
            //}

            //// Check if the best candidate is suitable at all
            //if (candidates.rbegin()->first > 0) {
            //    m_physicalDevice = candidates.rbegin()->second;
            //}
            //else {
            //    throw std::runtime_error("failed to find a suitable GPU!");
            //}
        }
    }

    bool VulkanManager::isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device); // Get QueueFamilies

        bool b_extensionsSupported = checkDeviceExtensionSupport(device); // Check for extension support

        bool b_swapChainAdequate = false;
        if (b_extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            b_swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && b_extensionsSupported && b_swapChainAdequate; // taking advantage of the std::optional<> here



        ///////////////////////// Not used at the moment
        //VkPhysicalDeviceProperties deviceProperties; // type, name, etc...
        //vkGetPhysicalDeviceProperties(device, &deviceProperties);
        //VkPhysicalDeviceFeatures deviceFeatures; // texture compression, 64 bit floats, multi viewport rendering, etc...
        //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        //// NOTE FROM WIKI: As an example, let's say we consider our application only usable for dedicated graphics cards that support geometry shaders. Then the isDeviceSuitable function would look like this:
        //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
    }

    VulkanManager::QueueFamilyIndices VulkanManager::findQueueFamilies(VkPhysicalDevice device)
    {
        // NOTE FROM WIKI: Almost every operation in Vulkan, anything from drawing to uploading textures, requires commands to be submitted to a queue. There are different types of queues that originate from different queue families and each family of queues allows only a subset of commands.

        QueueFamilyIndices indices;

        uint32_t ui_queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &ui_queueFamilyCount, nullptr); // Get number of families

        std::vector<VkQueueFamilyProperties> queueFamilies(ui_queueFamilyCount); // Use that number to initialize a container for familyProperties
        vkGetPhysicalDeviceQueueFamilyProperties(device, &ui_queueFamilyCount, queueFamilies.data()); // Put queue families into new container
        
        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            // Graphics
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) // Check this queue family flag for graphics and grabs it if it is
            {
                indices.graphicsFamily = i;
            }
            // Surface Presenting
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport); // checks if support for our specific surface is available in the queuefamily
            if (presentSupport) // Check this queue family flag for present support and grabs it if it is
            {
                indices.presentFamily = i;
            }

            // If we find a suitable card, exit early
            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    bool VulkanManager::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        // Checking for the ability to use the swap chain
        // NOTE FROM WIKI: Since image presentation is heavily tied into the window system and the surfaces associated with windows, it is not actually part of the Vulkan core. You have to enable the VK_KHR_swapchain device extension after querying for its support.

        uint32_t ui_extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &ui_extensionCount, nullptr); // Get number of available extensions

        std::vector<VkExtensionProperties> availableExtensions(ui_extensionCount);// Use that number to initialize a container
        vkEnumerateDeviceExtensionProperties(device, nullptr, &ui_extensionCount, availableExtensions.data()); // Fill that container with available extensions for this device

        std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end()); // Get list of extensions required for presenting images to window

        // Go through available extensions and if it is a required extension, erase it from requiredExtensions (like checking it off a list) Any extensions left in required after are not available on the device
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    int VulkanManager::rateDeviceSuitability(VkPhysicalDevice device) 
    {
        // For use later when picking desirable features
        int score = 0;

        VkPhysicalDeviceProperties deviceProperties; // type, name, etc...
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures; // texture compression, 64 bit floats, multi viewport rendering, etc...
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // Application can't function without geometry shaders
        if (!deviceFeatures.geometryShader) {
            return 0;
        }

        return score;
    }

    void VulkanManager::createLogicalDevice() 
    {
        // Create logical device to interface with the m_phyiscalDevice and queues for the device

        // Setup just requires filling a struct with information about the specific physical device we want to control
        QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

        // For each queue family, (graphics, presenting, etc..), we need to create a queue and get a queue handle for it. Create collection of queueFamilies to do this programatically
        // We create unique createInfos for each of these queueFamilies
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };


        float f_queuePriority = 1.0f; // Vulkan lets you assign priorities to queues to influence the scheduling of command buffer execution using floating point numbers between 0.0 and 1.0
        for (uint32_t ui_queueFamily : uniqueQueueFamilies)
        {
            // Create info struct with info to pass to the logical device later
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = ui_queueFamily;
            queueCreateInfo.queueCount = 1; // Number of queues, will only ever really need 1 because of multi threaded command buffers
            queueCreateInfo.pQueuePriorities = &f_queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Create features struct with info to pass to the logical device later
        VkPhysicalDeviceFeatures deviceFeatures{}; // Leave empty for now

        // Now we can start filling out the VkDeviceCreateInfo structure for our logical device with the structs created above
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;


        // NOTE FROM WIKI: The remainder of the information bears a resemblance to the VkInstanceCreateInfo struct and requires you to specify extensions and validation layers. The difference is that these are device specific this time.
        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size()); // give number of enabled extensions
        createInfo.ppEnabledExtensionNames = m_deviceExtensions.data(); // give names of extensions enabled (ie. VK_KHR_swapchain)

        if (m_b_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Actually create the logical device with queue and feature usage info stucts created above
        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device.");
        }
        else
        {
            std::cout << "Logical device successfully created." << std::endl;

            // Get queuehandles for our newly created device queues to be able to use them to send commands to
            vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue); // Graphics queue handle
            // ^^ NOTE FROM WIKI: The parameters are the logical device, queue family, queue index and a pointer to the variable to store the queue handle in. Because we're only creating a single queue from this family, we'll simply use index 0.
            vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue); // Present queue handle
            // ^^ NOTE FROM WIKI: In case the queue families are the same, the two handles will most likely have the same value now. 
        }
    }


    VulkanManager::SwapChainSupportDetails VulkanManager::querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        // Capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

        // Formats
        uint32_t ui_formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &ui_formatCount, nullptr); // Get number of formats
        if (ui_formatCount != 0)
        {
            details.formats.resize(ui_formatCount); // resize our details.format member with that number
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &ui_formatCount, details.formats.data()); // Fill details.formats with available formats on device
        }

        // Present Modes
        uint32_t ui_presentModesCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &ui_presentModesCount, nullptr); // Get number of present modes
        if (ui_presentModesCount != 0)
        {
            details.presentModes.resize(ui_presentModesCount); // resize our details.presentModes member with that number
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &ui_presentModesCount, details.presentModes.data()); //  Fill details.presentModes with available formats on device
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // Refer to https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain for colorSpace info
        for (const auto &availableFormat : availableFormats)
        {
            // These color formats are pretty much the standard for image rendering
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        // If the one we want isn't available, just go with the first available colorSpace (easy)
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        // Refer to https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain for colorSpace info
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        // If the one we want isn't available, this mode is guaranteed to be available so we'll choose to send that one instead
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        // We have to check this because screen resolution and screen space are glfws two different units of measuring sizes and we need to make sure they are set to proportional values (most of the time they are with the exception of some very high dpi screens)
        // if the width property is set to the maximum allowed by uint32_t, it indicates that the resolution is not equal to the screen coordinates and we must set the width and height explicitly
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            // We must calculate width and height ourselves
            int width;
            int height;
            glfwGetFramebufferSize(m_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // NOTE FROM WIKI: The clamp function is used here to bound the values of width and height between the allowed minimum and maximum extents that are supported by the implementation.
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void VulkanManager::createSwapChain()
    {
        // NOTE FROM WIKI: Now that we have all of these helper functions assisting us with the choices we have to make at runtime, we finally have all the information that is needed to create a working swap chain.
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // NOTE FROM WIKI:  Decide how many images we would like to have in the swap chain. Sticking to the default minimum means that we may sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. Therefore it is recommended to request at least one more image than the minimum
        uint32_t ui_imageCount = swapChainSupport.capabilities.minImageCount;
        if (ui_imageCount + 1 <= swapChainSupport.capabilities.maxImageCount || swapChainSupport.capabilities.maxImageCount == 0)
        {
            ui_imageCount += 1;
        } 

        // Create structure used for instantiating our swap chain (classic Vulkan)
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = ui_imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // Specifies the amount of layers each image consists of
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Specifies what kind of operations we'll use the images in the swap chain for.
        // ^^ NOTE FROM WIKI FOR ABOVE: It is also possible that you'll render images to a separate image first to perform operations like post-processing. In that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap chain image.

        // We need to specify how to handle swap chain images that will be used across multiple queue families.
        QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
            
        // NOTE FROM WIKI: If the graphics queue family and presentation queue family are the same, which will be the case on most hardware, then we should stick to exclusive mode, because concurrent mode requires you to specify at least two distinct queue families.
        if (indices.graphicsFamily != indices.presentFamily) // If the queue families are not the same, use concurrent sharing mode (more lenient, less performant)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else  // If the queue families are the same, use exclusive mode (best performance)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        // To specify that you do not want any transformation, simply specify the current transformation.
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE; // Don't care about pixel colors that are obscured behind other image pixels
        createInfo.oldSwapchain = VK_NULL_HANDLE; // Worry about this later


        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swapchain.");
        }
        else
        {
            std::cout << "Swapchain successfully created." << std::endl;

            // Save these swapchain values for later use
            m_swapChainImageFormat = surfaceFormat.format;
            m_swapChainExtent = extent;

            // Get swap chain image handles (like most other Vulkan retrieval operations)
            vkGetSwapchainImagesKHR(m_device, m_swapChain, &ui_imageCount, nullptr); // Get number of images in swapchain
            m_swapChainImages.resize(ui_imageCount); // Resize m_swapChainImages to appropriate size
            vkGetSwapchainImagesKHR(m_device, m_swapChain, &ui_imageCount, m_swapChainImages.data()); // Retrieve available images
        }
    }

    void VulkanManager::createImageViews()
    {
        // Get number of swapChainImages and use it to set the swapChainImageViews
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            // NOTE FROM WIKI: The parameters for image view creation are specified in a VkImageViewCreateInfo structure. The first few parameters are straightforward.
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
            createInfo.image = m_swapChainImages[i]; 
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
            createInfo.format = m_swapChainImageFormat; // specify how the image data should be interpreted
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // vv allows you to swizzle the color channels around
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // vv describes what the image's purpose is and which part of the image should be accessed
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;


            if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image view.");
            }
            else
            {
                std::cout << "Image view successfully created." << std::endl;
            }
        }
    }

    void VulkanManager::createFramebuffers()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Framebuffers
        // A framebuffer object references all of the VkImageView objects that represent the attachments.
        // Get the number of swapChainImageViews and use that number to resize the swapChainBuffers vector
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

        // Iterate through the imageViews and create framebuffers for each
        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                m_swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
            else
            {
                std::cout << "Framebuffer successfully created." << std::endl;
            }
        }
    }


    void VulkanManager::createRenderPass()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
        // In our case we'll have just a single color buffer attachment represented by one of the images from the swap chain
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // vv determine what to do with the data in the attachment before rendering and after rendering
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specifies which layout the image will have before the render pass begins
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // specifies the layout to automatically transition to when the render pass finishes
        // ^^ images need to be transitioned to specific layouts that are suitable for the operation that they're going to be involved in next.

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // (layout = 0)
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // Create Dependency
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Create render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        // Dependencies
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
        else
        {
            std::cout << "Render pass successfully created." << std::endl;
        }
    }

    void VulkanManager::createGraphicsPipeline()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        // Load the bytecode of the shaders
        auto vertShaderCode = readFile("../Shaders/vert.spv");
        auto fragShaderCode = readFile("../Shaders/frag.spv");

        // The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn't happen until the graphics pipeline is created, so we make them local and destroy them immediately after pipeline creation is finished
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // Need to assign shaders to whichever pipeline stage we want through this struct used to create the pipeline
        // VERT SHADER CODE
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Stage to inject shader code
        vertShaderStageInfo.module = vertShaderModule; // Shader to invoke
        vertShaderStageInfo.pName = "main"; // Entry point
        vertShaderStageInfo.pSpecializationInfo = nullptr; // Allows you to specify values for shader constants to be plugged in at shader runtime
        // FRAG SHADER CODE
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Finish by defining an array that contains these two structs which we will use later when defining the pipeline
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // VkPipelineVertexInputStateCreateInfo Describes the format of the vertex data that will be passed to the vertex shader in these two ways:
        // 1. Bindings: spacing between data and whether the data is per - vertex or per - instance(see instancing)
        // 2. Attribute descriptions : type of the attributes passed to the vertex shader, which binding to load them from and at which offset
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional


        // Describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
        // Options:
        // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        // VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;


        // Describe viewport dimensions
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_swapChainExtent.width;
        viewport.height = (float)m_swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Any pixels outside the scissor rectangles will be discarded by the rasterizer. 
        // If we want to draw to the entire framebuffer, we specify a scissor rectangle that covers it entirely:
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;


        // For creating dynamic pipeline that doesn't need to be fully recreated for certain values (ie. viewport size, line width, and blend constants)
        // Configuration of these values will be ignored and you will be able (and required) to specify the data at drawing time.
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();


        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;


        // Rasterizer - more info found here: https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        // NOTE FROM WIKI: The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering).
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


        // Multisampling - one of the ways to perform anti-aliasing - Enabling it requires enabling a GPU feature.
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional


        // Color blending - After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer.
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        // Alpha blending: where we want the new color to be blended with the old color based on its opacity
            // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
            // finalColor.a = newAlpha.a;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional


        // Pipeline Layout config
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
        else
        {
            std::cout << "Pipeline Layout successfully created." << std::endl;
        }


        // CREATE GRAPHICS PIPELINE
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion

        // We start by referencing the array of VkPipelineShaderStageCreateInfo structs from way above.
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        // Then we reference all of the structures describing the fixed-function stage.
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        // After that comes the pipeline layout, which is a Vulkan handle rather than a struct pointer.
        pipelineInfo.layout = m_pipelineLayout;
        // And finally we have the reference to the render pass and the index of the sub pass where this graphics pipeline will be used
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        // Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline (not using this)
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional


        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline.");
        }
        else
        {
            std::cout << "Graphics pipeline successfully created." << std::endl;
        }


        // Cleanup when pipeline is finished being created
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    }

    VkShaderModule VulkanManager::createShaderModule(const std::vector<char>& code)
    {
        // Before we can pass the code to the pipeline, we have to wrap it in a VkShaderModule object.
        // We need to specify a pointer to the buffer with the bytecode and the length of it.
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module.");
        }

        return shaderModule;
    }

    std::vector<char> VulkanManager::readFile(const std::string& filename)
    {
        // ate : Start reading at the end of the file - used to determine the size of the file to allocate a buffer
        // binary : Read the file as binary file(avoid text transformations)
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file.");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        // Once we have the buffer size needed, go back to the beginning of the file and read again
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }


    void VulkanManager::createCommandPool()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Command_buffers
        // Need a command pool to store all commands to send in bulk for Vulkan to batch process together
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        // Command buffers are executed by submitting them on one of the device queues. Each command pool can only allocate command buffers that are submitted on a single type of queue. We're going to record commands for drawing, which is why we've chosen the graphics queue family.
        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create command pool!");
        }
        else
        {
            std::cout << "Command pool successfully created." << std::endl;
        }
    }

    void VulkanManager::createCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
        else
        {
            std::cout << "Command Buffer successfully created." << std::endl;
        }
    }

    void VulkanManager::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        else
        {
            std::cout << "Command Buffer successfully began recording." << std::endl;

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_renderPass;
            renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = m_swapChainExtent;
            VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            // Begin render pass
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            // Bind the graphics pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

            // we did specify viewport and scissor state for this pipeline to be dynamic. So we need to set them in the command buffer before issuing our draw command:
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_swapChainExtent.width);
            viewport.height = static_cast<float>(m_swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = m_swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // Draw command for triangle
            // Parameters:
            // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
            // instanceCount : Used for instanced rendering, use 1 if you're not doing that.
            // firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
            // firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            // End render pass
            vkCmdEndRenderPass(commandBuffer);

            // Finish recording the command buffer
            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to record command buffer!");
            }
            else
            {
                std::cout << "Command buffer end recording." << std::endl;
            }
        }
    }

    void VulkanManager::createSyncObjects()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
       
        // Create semaphore info
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        // Create fence info
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start it signaled so the very first frame doesn't block indefinitely

        // Create semaphores and fence
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create semaphores!");
        }
        else
        {
            std::cout << "Semaphores and Fence successfully created." << std::endl;
        }
    }

    void VulkanManager::drawFrame()
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
        
        // At the start of the frame, we want to wait until the previous frame has finished, so that the command buffer and semaphores are available to use. To do that, we call vkWaitForFences:
        vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);

        // After waiting, we need to manually reset the fence to the unsignaled state with the vkResetFences call:
        vkResetFences(m_device, 1, &m_inFlightFence);

        // acquire an image from the swap chain
        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        // Record the command buffer
        // Reset to make sure it is able to be recorded
        vkResetCommandBuffer(m_commandBuffer, 0);

        recordCommandBuffer(m_commandBuffer, imageIndex);

        // Submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer;

        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        else
        {
            std::cout << "Draw command successfully submitted to command buffer." << std::endl;

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = { m_swapChain };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr; // Optional

            // Present!
            vkQueuePresentKHR(m_presentQueue, &presentInfo);
        }
    }


    void VulkanManager::cleanup()
    {
        // Make sure to cleanup all other resources BEFORE the Vulkan m_instance is destroyed (in order of creation if possible)

        // Semaphores and Fence
        vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
        vkDestroyFence(m_device, m_inFlightFence, nullptr);

        // Command Pool
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        // Framebuffers
        for (VkFramebuffer framebuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }

        // Graphics Pipeline
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        // Pipeline Layout
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        // Render pass
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);

        // Image views
        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
        // Swapchain
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        // Logical device
        vkDestroyDevice(m_device, nullptr);

        // Destroy validation messenger with specific vkDestroyDebugUtilsMessengerEXT looked up by following function (system specific)
        if (m_b_enableValidationLayers)
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

        // Destroy window surface
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        // Destroy Vulkan instance
        vkDestroyInstance(m_instance, nullptr);
        // Destroy glfw window
        glfwDestroyWindow(m_window);

        // Quit glfw
        glfwTerminate();
    }


    ///////////////////////
    // Validation layers // 
    ///////////////////////
    bool VulkanManager::checkValidationLayerSupport()
    {
        // Get list of available layers
        uint32_t ui_layerCount;
        vkEnumerateInstanceLayerProperties(&ui_layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(ui_layerCount);
        vkEnumerateInstanceLayerProperties(&ui_layerCount, availableLayers.data());

        // Check if all requested layers exist within those layers
        for (const char* layerName : m_validationLayers)
        {
            bool b_layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    b_layerFound = true;
                    break;
                }
            }

            if (!b_layerFound)
                return false;
        }

        return true;
    }
    void VulkanManager::setupDebugMessenger()
    {
        if (!m_b_enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        // Create instance of debugMessenger
        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger.");
        }
    }
    // Get extensions for debug validation layers
    std::vector<const char*> VulkanManager::getRequiredExtensions() {
        uint32_t ui_glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&ui_glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + ui_glfwExtensionCount);

        if (m_b_enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    // Create info for debug messenger so we don't have to repeat code for debug messenger for vkCreateInstance and vkDestroyInstance calls
    void VulkanManager::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
    // Because the function vkCreateDebugUtilsMessengerEXT for creating debug messenger object is an extention function (specific to each environment),
    // it must be looked up manually per system - This function does that
    VkResult VulkanManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    // Because the function DestroyDebugUtilsMessengerEXT for creating debug messenger object is an extention function (specific to each environment),
    // it must be looked up manually per system - This function does that
    void VulkanManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
    // NOTE FROM WIKI: If you want to see which call triggered a message, you can add a breakpoint to the message callback and look at the stack trace.
    // Custom function to be called by the validation layer system when an error occurs
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanManager::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            // Message is important enough to show
        }

        return VK_FALSE;
    }
    ///////////////////////
}