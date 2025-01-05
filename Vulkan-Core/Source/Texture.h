#pragma once
#include "LogicalDevice.h"
#include "WinSys.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <string>


namespace VCore
{
	class Texture
	{
	public:
		Texture();
		~Texture();

		void SetTexturePath(std::string path);
		std::string GetTexturePath();
		VkImageView GetImageView();
		VkImage GetImage();
		VkDeviceMemory GetTextureImageMemory();
		VkSampler GetTextureSampler();
		uint32_t GetMipLevels();
		void CreateTextureImage(WinSys& winSystem, VkCommandPool commandPool, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void CreateTextureSampler(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice);
		void Cleanup(LogicalDevice& logicalDevice);

	private:
		std::string m_texturePath;
		VkImageView m_imageView;
		VkImage m_image;
		VkDeviceMemory m_textureImageMemory;
		VkSampler m_textureSampler;
		uint32_t m_mipLevels;
	};
}