#include "Renderer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_glfw.h>

#include "Engine.h"
#include "Window.h"
#include "VulkanUtilities.h"

static bool show_demo_window = true;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static const int g_minImageCount = 2;

void Renderer::createImguiContext()
{
    VkResult err;

    // Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_imguiDescriptorPool);
        // check_vk_result(err);
    }

    // Render Pass
    {
        VkAttachmentDescription attachmentDesc = {};
        attachmentDesc.format = m_swapChainImageFormat;
        attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference attachmentRef = {};
        attachmentRef.attachment = 0;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDesc = {};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &attachmentRef;

        VkSubpassDependency subpassDependency = {};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachmentDesc;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDesc;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &subpassDependency;
        assert( vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass.imgui) == VK_SUCCESS );
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(Engine::m_window.Get(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physicalDevice;
    init_info.Device = m_device;
    init_info.QueueFamily = (uint32_t)-1; // TODO: not used
    init_info.Queue = m_graphicsQueue;
    init_info.PipelineCache = nullptr; // TODO:
    init_info.DescriptorPool = m_imguiDescriptorPool; // TODO: separate pools?
    init_info.Allocator = nullptr; // TODO:
    init_info.MinImageCount = g_minImageCount; // TODO: 2?
    init_info.ImageCount = m_swapChainImageCount;
    init_info.CheckVkResultFn = nullptr; // TODO: check_vk_result
    ImGui_ImplVulkan_Init(&init_info, m_renderPass.imgui);

    // Upload Fonts
    {
        VkCommandBuffer commandBuffer = beginCommandBuffer(m_device, m_commandPool);
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        endCommandBuffer(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    m_imguiCommandPools = new VkCommandPool[m_swapChainImageCount];
    m_imguiCommandBuffers = new VkCommandBuffer[m_swapChainImageCount];

    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = m_graphicsFamily;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = 1;

    for (int i=0; i<m_swapChainImageCount; ++i)
    {
        assert( vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_imguiCommandPools[i]) == VK_SUCCESS );
        cmdBufferAllocInfo.commandPool = m_imguiCommandPools[i];
        assert( vkAllocateCommandBuffers(m_device, &cmdBufferAllocInfo, &m_imguiCommandBuffers[i]) == VK_SUCCESS );
    }

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = m_renderPass.imgui;
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.width = m_swapChainExtent.width;
    framebufferCreateInfo.height = m_swapChainExtent.height;
    framebufferCreateInfo.layers = 1;
    m_imguiFramebuffers = new VkFramebuffer[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        framebufferCreateInfo.pAttachments = &m_swapChainImageViews[i];
        assert( vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &m_imguiFramebuffers[i]) == VK_SUCCESS );
    }
}

void Renderer::cleanupImguiContext()
{
    for (int i=0; i<m_swapChainImageCount; ++i)
    {
        vkFreeCommandBuffers(m_device, m_imguiCommandPools[i], 1, &m_imguiCommandBuffers[i]);
        vkDestroyCommandPool(m_device, m_imguiCommandPools[i], nullptr);
        vkDestroyFramebuffer(m_device, m_imguiFramebuffers[i], nullptr);
    }
    delete[] m_imguiCommandPools;
    delete[] m_imguiCommandBuffers;
    delete[] m_imguiFramebuffers;

    vkDestroyDescriptorPool(m_device, m_imguiDescriptorPool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // CleanupVulkanWindow();
    // CleanupVulkan();
}

void Renderer::renderImgui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    {
        static char buf[64];
        static float pos[3];

        ImGui::Begin("Test");

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Image File");
        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        ImGui::InputText("", buf, 64);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            loadImageInstance(buf, pos);
        }

        ImGui::InputFloat3("Position", pos, 2);

        ImGui::End();
    }

    ImGui::Render();
    // memcpy(&wd->ClearValue.color.float32[0], &clear_color, 4 * sizeof(float));
}

void Renderer::updateImgui(uint32_t frameIndex)
{
    {
        VkResult err;
        {
            err = vkResetCommandPool(m_device, m_imguiCommandPools[frameIndex], 0);
            // check_vk_result(err); // TODO: implement for all
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(m_imguiCommandBuffers[frameIndex], &info);
            // check_vk_result(err);
        }
        {
            VkClearValue clearColor = {0.0f};
            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_renderPass.imgui;
            info.framebuffer = m_imguiFramebuffers[frameIndex];
            info.renderArea.offset =  {0, 0};
            info.renderArea.extent = m_swapChainExtent;
            info.clearValueCount = 1;
            info.pClearValues = &clearColor;
            vkCmdBeginRenderPass(m_imguiCommandBuffers[frameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Record Imgui Draw Data and draw funcs into command buffer
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_imguiCommandBuffers[frameIndex]);

        // Submit command buffer
        vkCmdEndRenderPass(m_imguiCommandBuffers[frameIndex]);
        err = vkEndCommandBuffer(m_imguiCommandBuffers[frameIndex]);
        /*
        {
            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &image_acquired_semaphore;
            info.pWaitDstStageMask = &wait_stage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &m_imguiCommandBuffers[frameIndex];
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &render_complete_semaphore;

            err = vkEndCommandBuffer(m_imguiCommandBuffers[frameIndex]);
            // check_vk_result(err);
            err = vkQueueSubmit(m_graphicsQueue, 1, &info, fd->Fence);
            // check_vk_result(err);
        }
        //*/
    }

    // FramePresent(wd);
}

// void Renderer::renderImgui()
// {

// }

void Renderer::recreateImguiSwapChain()
{
    assert( false );
    ImGui_ImplVulkan_SetMinImageCount(g_minImageCount);
    ImGui_ImplVulkanH_CreateWindow(m_instance, m_physicalDevice, m_device, nullptr, 
            (uint32_t)-1, nullptr, m_swapChainExtent.width, m_swapChainExtent.height, g_minImageCount);
}