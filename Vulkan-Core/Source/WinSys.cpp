#include "WinSys.h"
#include "WinSys.h"
#include "VulkanManager.h"
#include "Helper.h"

// Image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <array>
#include <algorithm> // Necessary for std::clamp
#include <stdexcept>


namespace VCore
{
    WinSys::WinSys()
	{
        m_window = VK_NULL_HANDLE;
        m_surface = VK_NULL_HANDLE;
        m_windowWidth = 800;
        m_windowHeight = 600;
        m_swapChain = VK_NULL_HANDLE;
        m_swapChainImageFormat = VkFormat();
        m_swapChainExtent = VkExtent2D();
        m_swapChainImages = std::vector<VkImage>(); // automatically cleaned up once the swap chain has been destroyed
        m_swapChainImageViews = std::vector<VkImageView>();
        m_swapChainFramebuffers = std::vector<VkFramebuffer>();

        // antialiasing
        m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        m_colorImage = VK_NULL_HANDLE;
        m_colorImageMemory = VK_NULL_HANDLE;
        m_colorImageView = VK_NULL_HANDLE;

        // depth testing
        m_depthImage = VK_NULL_HANDLE;
        m_depthImageMemory = VK_NULL_HANDLE;
        m_depthImageView = VK_NULL_HANDLE;
	}

    WinSys::~WinSys()
    {
    }

    void WinSys::InitWindow()
    {
        // initialize glfw
        glfwInit();

        // Tell glfw not to create OpenGL context with init call
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Disable window resizing for now
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        // Create our window
        // glfwCreateWindow(width, height, title, monitorToOpenOn, onlyUsedForOpenGL)
        m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Vulkan", nullptr, nullptr);

        // resize window callback function - see recreating swapchain section - https://vulkan-tutorial.com/en/Drawing_a_triangle/Swap_chain_recreation
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, VulkanManager::framebufferResizeCallback);
    }

    void WinSys::CreateSurface(VkInstance instance)
    {
        if (glfwCreateWindowSurface(instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface.");
        }
    }

    GLFWwindow* WinSys::GetWindow()
    {
        return m_window;
    }

    VkSurfaceKHR WinSys::GetSurface()
    {
        return m_surface;
    }

    void WinSys::Cleanup(VkInstance instance)
    {
        vkDestroySurfaceKHR(instance, m_surface, nullptr);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    VkSurfaceFormatKHR WinSys::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // Refer to https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain for colorSpace info
        for (const auto& availableFormat : availableFormats)
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

    VkPresentModeKHR WinSys::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
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

    VkExtent2D WinSys::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

    void WinSys::CreateSwapChain(PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice)
    {
        // NOTE FROM WIKI: Now that we have all of these helper functions assisting us with the choices we have to make at runtime, we finally have all the information that is needed to create a working swap chain.
        SwapChainSupportDetails swapChainSupport = Helper::QuerySwapChainSupport(physicalDevice.GetDevice(), m_surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

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
        QueueFamilyIndices indices = Helper::FindQueueFamilies(physicalDevice.GetDevice(), m_surface);
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


        if (vkCreateSwapchainKHR(logicalDevice.GetDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swapchain.");
        }
        else
        {
            // Save these swapchain values for later use
            m_swapChainImageFormat = surfaceFormat.format;
            m_swapChainExtent = extent;

            // Get swap chain image handles (like most other Vulkan retrieval operations)
            vkGetSwapchainImagesKHR(logicalDevice.GetDevice(), m_swapChain, &ui_imageCount, nullptr); // Get number of images in swapchain
            m_swapChainImages.resize(ui_imageCount); // Resize m_swapChainImages to appropriate size
            vkGetSwapchainImagesKHR(logicalDevice.GetDevice(), m_swapChain, &ui_imageCount, m_swapChainImages.data()); // Retrieve available images
        }
    }

    void WinSys::RecreateSwapChain(LogicalDevice &logicalDevice, PhysicalDevice &physicalDevice, VkRenderPass renderPass)
    {
        int width = 0, height = 0;
        while (width == 0 || height == 0) 
        {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(logicalDevice.GetDevice());

        CleanupSwapChain(logicalDevice);

        CreateSwapChain(physicalDevice, logicalDevice);
        CreateImageViews(logicalDevice);
        CreateColorResources(physicalDevice, logicalDevice);
        CreateDepthResources(physicalDevice, logicalDevice);
        CreateFramebuffers(logicalDevice, renderPass);
    }

    void WinSys::CleanupSwapChain(LogicalDevice &logicalDevice)
    {
        // Antialiasing (msaa) color samples
        vkDestroyImageView(logicalDevice.GetDevice(), m_colorImageView, nullptr);
        vkDestroyImage(logicalDevice.GetDevice(), m_colorImage, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), m_colorImageMemory, nullptr);

        // Depth buffer testing
        vkDestroyImageView(logicalDevice.GetDevice(), m_depthImageView, nullptr);
        vkDestroyImage(logicalDevice.GetDevice(), m_depthImage, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), m_depthImageMemory, nullptr);

        // Image views
        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(logicalDevice.GetDevice(), imageView, nullptr);
        }
        // Framebuffers
        for (VkFramebuffer framebuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(logicalDevice.GetDevice(), framebuffer, nullptr);
        }

        // Swapchain
        vkDestroySwapchainKHR(logicalDevice.GetDevice(), m_swapChain, nullptr);
    }

    void WinSys::CreateFramebuffers(LogicalDevice &logicalDevice, VkRenderPass renderPass)
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Framebuffers

        // A framebuffer object references all of the VkImageView objects that represent the attachments.
        // Get the number of swapChainImageViews and use that number to resize the swapChainBuffers vector
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

        // Iterate through the imageViews and create framebuffers for each
        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments =
            {
                 m_colorImageView,
                 m_depthImageView,
                 m_swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice.GetDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void WinSys::CreateImageViews(LogicalDevice &logicalDevice)
    {
        // Get number of swapChainImages and use it to set the swapChainImageViews
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++)
        {
            uint32_t singleMipLevel = 1;
            m_swapChainImageViews[i] = CreateImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, singleMipLevel, logicalDevice);
        }
    }

    VkImageView WinSys::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, LogicalDevice &logicalDevice)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(logicalDevice.GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void WinSys::CreateColorResources(PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice) {
        // Refer to - https://vulkan-tutorial.com/Multisampling

        VkFormat colorFormat = m_swapChainImageFormat;

        CreateImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, m_msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory, physicalDevice, logicalDevice);
        m_colorImageView = CreateImageView(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, logicalDevice);
    }


    void WinSys::CreateDepthResources(PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice)
    {
        // Refer to - https://vulkan-tutorial.com/en/Depth_buffering
        // and for msaa - https://vulkan-tutorial.com/Multisampling

        VkFormat depthFormat = Helper::FindDepthFormat(physicalDevice.GetDevice());
        uint32_t singleMipLevel = 1;

        CreateImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, m_msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory, physicalDevice, logicalDevice);
        m_depthImageView = CreateImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, singleMipLevel, logicalDevice);
    }

    VkFormat WinSys::GetImageFormat()
    {
        return m_swapChainImageFormat;
    }

    VkSampleCountFlagBits WinSys::GetMsaa()
    {
        return m_msaaSamples;
    }

    VkExtent2D WinSys::GetExtent()
    {
        return m_swapChainExtent;
    }

    std::vector<VkFramebuffer> &WinSys::GetFrameBuffers()
    {
        return m_swapChainFramebuffers;
    }

    VkSwapchainKHR& WinSys::GetSwapChain()
    {
        return m_swapChain;
    }

    void WinSys::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(logicalDevice.GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(logicalDevice.GetDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = physicalDevice.FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logicalDevice.GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(logicalDevice.GetDevice(), image, imageMemory, 0);
    }

    void WinSys::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkCommandPool commandPool, LogicalDevice &logicalDevice)
    {
        // Refer to - https://vulkan-tutorial.com/en/Texture_mapping/Images

        VkCommandBuffer commandBuffer = Helper::BeginSingleTimeCommands(commandPool, logicalDevice);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = 0;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage{};
        VkPipelineStageFlags destinationStage{};

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        Helper::EndSingleTimeCommands(commandPool, commandBuffer, logicalDevice);
    }

    void WinSys::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandPool commandPool, LogicalDevice &logicalDevice)
    {
        VkCommandBuffer commandBuffer = Helper::BeginSingleTimeCommands(commandPool, logicalDevice);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        Helper::EndSingleTimeCommands(commandPool, commandBuffer, logicalDevice);
    }

    void WinSys::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VkCommandPool commandPool, PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice)
    {
        // Refer to - https://vulkan-tutorial.com/en/Generating_Mipmaps

         // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice.GetDevice(), imageFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = Helper::BeginSingleTimeCommands(commandPool, logicalDevice);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // Because the last mip level was never transitioned by the above loop
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        Helper::EndSingleTimeCommands(commandPool, commandBuffer, logicalDevice);
    }

    VkImage WinSys::CreateTextureImage(std::string path, uint32_t mipLevels, VkCommandPool commandPool, PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice, VkDeviceMemory textureImageMemory)
    {
        // Refer to - https://vulkan-tutorial.com/en/Texture_mapping/Images
        // And refer to - https://vulkan-tutorial.com/en/Generating_Mipmaps

        VkImage newImage = VK_NULL_HANDLE;

        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels)
        {
            throw std::runtime_error("failed to load texture image!");
        }

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, physicalDevice, logicalDevice);

        void* data{};
        vkMapMemory(logicalDevice.GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice.GetDevice(), stagingBufferMemory);

        // Cleanup pixel array
        stbi_image_free(pixels);


        CreateImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newImage, textureImageMemory, physicalDevice, logicalDevice);

        TransitionImageLayout(newImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, commandPool, logicalDevice);
        CopyBufferToImage(stagingBuffer, newImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), commandPool, logicalDevice);
        // transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);
        //  Removed this call ^^ because we are transiting to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps instead now
        GenerateMipmaps(newImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels, commandPool, physicalDevice, logicalDevice);

        vkDestroyBuffer(logicalDevice.GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), stagingBufferMemory, nullptr);

        return newImage;
    }

    void WinSys::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, PhysicalDevice& physicalDevice, LogicalDevice &logicalDevice)
    {
        // Refer to - https://vulkan-tutorial.com/en/Vertex_buffers/Vertex_buffer_creation
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice.GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(logicalDevice.GetDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = physicalDevice.FindMemoryType(memRequirements.memoryTypeBits, properties);

        // NOTE FROM THE WIKI: It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory for every individual buffer. The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit
        if (vkAllocateMemory(logicalDevice.GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(logicalDevice.GetDevice(), buffer, bufferMemory, 0);
    }

    void WinSys::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool, LogicalDevice &logicalDevice)
    {
        VkCommandBuffer commandBuffer = Helper::BeginSingleTimeCommands(commandPool, logicalDevice);

        // We're only going to use the command buffer once and wait with returning from the function until the copy operation has finished executing. It's good practice to tell the driver about our intent using 
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        Helper::EndSingleTimeCommands(commandPool, commandBuffer, logicalDevice);
    }
}