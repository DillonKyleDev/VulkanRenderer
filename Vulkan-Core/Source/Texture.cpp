#include "Texture.h"
#include "VulkanManager.h"
#include "Helper.h"

#include <array>
#include <stdexcept>


namespace VCore
{
	Texture::Texture()
	{
        m_texturePath = "";
        m_imageView = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_textureImageMemory = VK_NULL_HANDLE;
        m_textureSampler = VK_NULL_HANDLE;
        m_mipLevels = 1;
	}

	Texture::~Texture()
	{

	}

    void Texture::SetTexturePath(std::string path)
    {
        m_texturePath = path;
    }

    std::string Texture::GetTexturePath()
    {
        return std::string();
    }

    VkImageView Texture::GetImageView()
    {
        return m_imageView;
    }

    VkImage Texture::GetImage()
    {
        return m_image;
    }

    VkDeviceMemory Texture::GetTextureImageMemory()
    {
        return m_textureImageMemory;
    }

    VkSampler Texture::GetTextureSampler()
    {
        return m_textureSampler;
    }

    uint32_t Texture::GetMipLevels()
    {
        return m_mipLevels;
    }

    void Texture::CreateTextureImage(WinSys& winSystem, VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
    {
        VkImage newImage = winSystem.CreateTextureImage(m_texturePath, m_mipLevels, commandPool, physicalDevice, logicalDevice, m_textureImageMemory);
        VkImageView textureImageView = winSystem.CreateImageView(newImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, logicalDevice);
        m_image = newImage;
        m_imageView = textureImageView;
    }

    void Texture::CreateTextureSampler(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
    {
        // Refer to - https://vulkan-tutorial.com/en/Texture_mapping/Image_view_and_sampler

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice.GetDevice(), &properties);

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f; // Optional
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);
        samplerInfo.mipLodBias = 0.0f; // Optional

        if (vkCreateSampler(logicalDevice.GetDevice(), &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void Texture::Cleanup(LogicalDevice& logicalDevice)
    {
        vkDestroySampler(logicalDevice.GetDevice(), m_textureSampler, nullptr);
        vkFreeMemory(logicalDevice.GetDevice(), m_textureImageMemory, nullptr);
        vkDestroyImage(logicalDevice.GetDevice(), m_image, nullptr);
        vkDestroyImageView(logicalDevice.GetDevice(), m_imageView, nullptr);
    }
}