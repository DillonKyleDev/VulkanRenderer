#include "RenderPass.h"
#include "Helper.h"
#include "VulkanManager.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include <stdexcept>
#include <chrono>


namespace VCore
{
	RenderPass::RenderPass()
	{
		m_renderPass = VK_NULL_HANDLE;
        m_commandBuffers = std::vector<VkCommandBuffer>();
	}

	RenderPass::~RenderPass()
	{
	}

    void RenderPass::Cleanup(LogicalDevice& logicalDevice)
    {
        vkDestroyRenderPass(logicalDevice.GetDevice(), m_renderPass, nullptr);
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
        colorAttachmentResolve.samples = winSystem.GetMsaa();
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

    void RenderPass::BeginRenderPass(uint32_t imageIndex, WinSys& winSystem)
    {
        // Reset to make sure it is able to be recorded
        vkResetCommandBuffer(m_commandBuffers[VM_currentFrame], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(m_commandBuffers[VM_currentFrame], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // For Depth buffer
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.2f, 0.2f, 0.2f, 0.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = winSystem.GetFrameBuffers()[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = winSystem.GetExtent();
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // we did specify viewport and scissor state for this pipeline to be dynamic. So we need to set them in the command buffer before issuing our draw command:
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(winSystem.GetExtent().width);
        viewport.height = static_cast<float>(winSystem.GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_commandBuffers[VM_currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = winSystem.GetExtent();
        vkCmdSetScissor(m_commandBuffers[VM_currentFrame], 0, 1, &scissor);

        // Begin render pass
        vkCmdBeginRenderPass(m_commandBuffers[VM_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void RenderPass::EndRenderPass()
    {
        // End render pass
        vkCmdEndRenderPass(m_commandBuffers[VM_currentFrame]);

        // Finish recording the command buffer
        if (vkEndCommandBuffer(m_commandBuffers[VM_currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void RenderPass::RecordCommandBuffer(uint32_t imageIndex, WinSys& winSystem, GameObject& object)
    {
        VkPipeline& graphicsPipeline = object.GetMaterial()->GetGraphicsPipeline();
        VkPipelineLayout& pipelineLayout = object.GetMaterial()->GetPipelineLayout();
        VkDescriptorSet& descriptorSet = object.GetDescriptorSets()[VM_currentFrame];
        VkBuffer& vertexBuffer = object.GetModel().GetVertexBuffer();
        VkBuffer& indexBuffer = object.GetModel().GetIndexBuffer();
        std::vector<uint32_t>& indices = object.GetModel().GetIndices();

        // Push constants
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        float data[3] = { time }; // where sizeof(float) == 4 bytes
        uint32_t offset = 0;
        uint32_t size = 12;
        vkCmdPushConstants(m_commandBuffers[VM_currentFrame], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);        

        // Bind the graphics pipeline
        vkCmdBindPipeline(m_commandBuffers[VM_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Bind the vertex buffer
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffers[VM_currentFrame], 0, 1, vertexBuffers, offsets);

        // Bind the index buffer
        vkCmdBindIndexBuffer(m_commandBuffers[VM_currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(m_commandBuffers[VM_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // Refer to - https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer
        vkCmdDrawIndexed(m_commandBuffers[VM_currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0); // reusing vertices with index buffers.
        // NOTE FROM THE WIKI: The previous chapter already mentioned that you should allocate multiple resources like buffers from a single memory allocation, but in fact you should go a step further. Driver developers recommend that you also store multiple buffers, like the vertex and index buffer, into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers. The advantage is that your data is more cache friendly in that case, because it's closer together. It is even possible to reuse the same chunk of memory for multiple resources if they are not used during the same render operations, provided that their data is refreshed, of course. This is known as aliasing and some Vulkan functions have explicit flags to specify that you want to do this.
    }

    VkRenderPass& RenderPass::GetRenderPass()
    {
        return m_renderPass;
    }

    void RenderPass::CreateCommandBuffers(VkCommandPool commandPool, LogicalDevice& logicalDevice)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = VM_MAX_FRAMES_IN_FLIGHT;

        m_commandBuffers.resize(VM_MAX_FRAMES_IN_FLIGHT);

        if (vkAllocateCommandBuffers(logicalDevice.GetDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    std::vector<VkCommandBuffer>& RenderPass::GetCommandBuffers()
    {
        return m_commandBuffers;
    }
}
