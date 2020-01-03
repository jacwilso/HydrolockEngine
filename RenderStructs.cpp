#include "RenderStructs.h"

#include <assert.h>
#include "VulkanUtilities.h"

/// Attachment
void Attachment::create(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat colorFormat)
{
    createImage(device, physicalDevice, extent.width, extent.height, 
        colorFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        color, colorMemory);
    createImageView(device, color, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, colorView);

    VkFormat depthFormat = findDepthFormat(physicalDevice);
    createImage(device, physicalDevice, extent.width, extent.height, 
        depthFormat, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depth, depthMemory);
    createImageView(device, depth, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthView);
}

void Attachment::destroy(VkDevice device)
{
    vkDestroyImageView(device, colorView, nullptr);
    vkDestroyImage(device, color, nullptr);
    vkFreeMemory(device, colorMemory, nullptr);

    vkDestroyImageView(device, depthView, nullptr);
    vkDestroyImage(device, depth, nullptr);
    vkFreeMemory(device, depthMemory, nullptr);
}

/// Pipeline
void Pipeline::create(VkDevice device)
{
    assert( false );
}

void Pipeline::destroy(VkDevice device)
{
    vkDestroyPipeline(device, scene, nullptr);
    vkDestroyPipeline(device, composition, nullptr);
    vkDestroyPipeline(device, imgui, nullptr);
#if EDITOR
    vkDestroyPipeline(device, wireframe, nullptr);
#endif
}

/// Render Pass
void RenderPass::create(VkDevice device)
{
    assert( false );
}

void RenderPass::destroy(VkDevice device)
{
    vkDestroyRenderPass(device, scene, nullptr);
    vkDestroyRenderPass(device, imgui, nullptr);
}