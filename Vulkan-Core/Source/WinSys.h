#pragma once
#include "PhysicalDevice.h"
#include "LogicalDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <glm.hpp>

#include <vector>
#include <string>


namespace VCore
{
	class WinSys
	{
	public:
		WinSys();
		~WinSys();
		void CleanupSystem(VkInstance instance);
		void CleanupSwapChain(LogicalDevice& logicalDevice);

		void InitWindow();
		void CreateSurface(VkInstance instance);		
		GLFWwindow* GetWindow();
		VkSurfaceKHR GetSurface();
		// NOTE FROM WIKI: If the swapChainAdequate conditions were met then the support is definitely sufficient, but there may still be many different modes of varying optimality. We'll now write a couple of functions to find the right settings for the best possible swap chain. There are three types of settings to determine:
		// Surface format (color depth)
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		// Presentation mode (conditions for "swapping" images to the screen)
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		// Swap extent (resolution of images in swap chain)
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		void CreateSwapChain(PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice);
		void RecreateSwapChain(LogicalDevice &logicalDevice, PhysicalDevice &physicalDevice, VkRenderPass renderPass);
		void CreateFramebuffers(LogicalDevice &logicalDevice, VkRenderPass renderPass);
		void CreateImageViews(LogicalDevice &logicalDevice);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, LogicalDevice &logicalDevice);

		// textures
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkCommandPool commandPool, LogicalDevice &logicalDevice);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCommandPool commandPool, LogicalDevice &logicalDevice);
		void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VkCommandPool commandPool, PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice);
		VkImage CreateTextureImage(std::string path, uint32_t mipLevels, VkCommandPool commandPool, PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice, VkDeviceMemory textureImageMemory);

		void CreateColorResources(PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice);
		void CreateDepthResources(PhysicalDevice &physicalDevice, LogicalDevice &logicalDevice); // depth testing

		static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, PhysicalDevice& physicalDevice, LogicalDevice &logicalDevice);
		static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool, LogicalDevice &logicalDevice);

		VkFormat GetImageFormat();
		VkSampleCountFlagBits GetMsaa();
		VkExtent2D GetExtent();
		std::vector<VkFramebuffer> &GetFrameBuffers();
		VkSwapchainKHR &GetSwapChain();

	private:
		GLFWwindow* m_window;
		VkSurfaceKHR m_surface;
		int m_windowWidth;
		int m_windowHeight;
		VkSwapchainKHR m_swapChain;
		VkFormat m_swapChainImageFormat;
		VkExtent2D m_swapChainExtent;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;

		// antialiasing
		VkSampleCountFlagBits m_msaaSamples;
		VkImage m_colorImage;
		VkDeviceMemory m_colorImageMemory;
		VkImageView m_colorImageView;

		// depth testing
		VkImage m_depthImage;
		VkDeviceMemory m_depthImageMemory;
		VkImageView m_depthImageView;
	};
}

