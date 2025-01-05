#include "RenderPass.h"
#include "Helper.h"

#include <stdexcept>


namespace VCore
{
	RenderPass::RenderPass()
	{
		m_renderPass = VK_NULL_HANDLE;
	}

	RenderPass::~RenderPass()
	{
	}

    void RenderPass::CreateRenderPass(WinSys& winSystem, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice)
    {
        // More info here - https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
        // In our case we'll have just a single color buffer attachment represented by one of the images from the swap chain
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = winSystem.GetImageFormat();
        colorAttachment.samples = winSystem.GetMsaa();
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // vv determine what to do with the data in the attachment before rendering and after rendering
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // specifies which layout the image will have before the render pass begins
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // changed from the setting below for antialiasing multisampling. refer to - https://vulkan-tutorial.com/Multisampling for explanation why
        //colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // specifies the layout to automatically transition to when the render pass finishes
        // ^^ images need to be transitioned to specific layouts that are suitable for the operation that they're going to be involved in next.
        // Color attach still
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // (layout = 0)
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth attachment
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = Helper::FindDepthFormat(physicalDevice.GetDevice());
        depthAttachment.samples = winSystem.GetMsaa();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        // Depth still
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Resolve attachment for MSAA
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = winSystem.GetImageFormat();
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // Resolve still
        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        // Create Dependency
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


        // Create render pass
        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        // Dependencies
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(logicalDevice.GetDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    VkRenderPass& RenderPass::GetRenderPass()
    {
        return m_renderPass;
    }

    void RenderPass::Cleanup(LogicalDevice& logicalDevice)
    {
        vkDestroyRenderPass(logicalDevice.GetDevice(), m_renderPass, nullptr);
    }
}
