#include "VulkanUtilities.h"

#include <assert.h>
#include <iostream>
#include <fstream>

#include "Utilities/Defines.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

void AvailableExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    VkExtensionProperties extensions[extensionCount];
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
    std::cout << "Available extensions:" << std::endl;
    for (const auto& extension : extensions)
    {
        std::cout << "\t" << extension.extensionName << std::endl;
    }
}

void constructDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    QueueFamilyIndices indices; // TODO: pass in like variable

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            indices.bitmask |= GRAPHICS_BIT;
        } 
#if TRANSFER_FAMILY
        else if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
        {
            indices.transferFamily = i;
            indices.bitmask |= TRANSFER_BIT;
        }
#endif
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
            indices.bitmask |= PRESENT_BIT;
        }

        if (indices.isComplete()) break;
        i++;
    }

    return indices;
}

SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, nullptr);
    if (details.formatCount > 0)
    {
        details.formats = new VkSurfaceFormatKHR[details.formatCount];
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
    } else
    {
        details.formats = nullptr;
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, nullptr);
    if (details.presentModeCount > 0)
    {
        details.presentModes = new VkPresentModeKHR[details.presentModeCount];
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);
    } else
    {
        details.presentModes = nullptr;
    }

    return details;
}

bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if (!supportedFeatures.samplerAnisotropy) return false; // TODO: Enum properties
#if EDITOR
    if (!supportedFeatures.fillModeNonSolid) return false; // TODO: Enum properties
#endif

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);
    for (const auto& required : requiredExtensions)
    {
        bool found = false;
        for (const auto& available : availableExtensions)
        {
            if (strcmp(required, available.extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    return indices.isComplete() && swapChainSupport.presentModeCount > 0;
}

VkSurfaceFormatKHR selectSwapSurfaceFormat(const VkSurfaceFormatKHR* const availableFormats, int availableFormatCount)
{
    for (int i = 0; i < availableFormatCount; ++i)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8_UNORM &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats[i];
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR selectSwapPresentMode(const VkPresentModeKHR* const availablePresentModes, int availablePresentModeCount)
{
    for (int i = 0; i < availablePresentModeCount; ++i)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    VkExtent2D extent = {width, height};
    extent.width = MAX(capabilities.minImageExtent.width, 
        MIN(capabilities.maxImageExtent.width, width));
    extent.height = MAX(capabilities.minImageExtent.height, 
        MIN(capabilities.maxImageExtent.height, height));
    return extent;
}

VkShaderModule createShaderModule(const VkDevice& device, const char* const shaderPath)
{
    std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
    assert( file.is_open() );
    size_t size = (size_t)file.tellg();
    // TODO: https://stackoverflow.com/questions/6320264/how-to-align-pointer#comment62262961_6320314
    char* buffer = new char[size];
    // std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer, size);
    // file.read(buffer.data(), size);
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    // TODO: this will cause errors https://stackoverflow.com/questions/22770255/can-i-cast-an-unsigned-char-to-an-unsigned-int
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    // createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    VkShaderModule module;
    assert( vkCreateShaderModule(device, &createInfo, nullptr, &module) == VK_SUCCESS );

    delete[] buffer;
    return module;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    assert( false && "Failed to find suitable memory type" );
}

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, VkFormat* const candidates, uint32_t candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (int i = 0; i < candidateCount; ++i)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, candidates[i], &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return candidates[i];
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return candidates[i];
    }
    assert( false && "Failed to find supported format");
    return VK_FORMAT_UNDEFINED;
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
{
    VkFormat depthFormats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return findSupportedFormat(physicalDevice, 
        depthFormats, sizeof(depthFormats) / sizeof(VkFormat),
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createBuffer(VkDevice device,
                  VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, 
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer& buffer,
                  VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
#if TRANSFER_FAMILY
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
#else
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
#endif
    // bufferCreateInfo.flags = 0;
    assert( vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) == VK_SUCCESS );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memRequirements.size;
    // TODO: VK_MEMORY_PROPERTY_HOST_COHERENT_BIT worse performance. use flushing
    memAllocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    // TODO: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    assert( vkAllocateMemory(device, &memAllocInfo, nullptr, &bufferMemory) == VK_SUCCESS );
    vkBindBufferMemory(device, buffer, bufferMemory, 0); // TODO: should I assert on != success?
}

VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    return cmdBuffer;
}

void endCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer cmdBuffer)
{
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue); // TODO: use a fence to schedule multiple transfers
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
}

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer cmdBuffer = beginCommandBuffer(device, commandPool);

    VkBufferCopy copy = {};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copy);

    endCommandBuffer(device, commandPool, queue, cmdBuffer);
}

void copyImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, 
    VkBuffer texBuffer, VkImage image, uint32_t width, uint32_t height, VkFormat format)
{
    // TODO: combine operations into single command buffer + execute async (setupCmdBuffer + flushCmdBuffer)
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Transition CPU Write
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    
    VkCommandBuffer cmdBuffer = beginCommandBuffer(device, commandPool);
    vkCmdPipelineBarrier(cmdBuffer, 
        srcStage, dstStage, 
        0, 
        0, nullptr,
        0, nullptr,
        1, &barrier);
    endCommandBuffer(device, commandPool, queue, cmdBuffer);

    // Copy Image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    cmdBuffer = beginCommandBuffer(device, commandPool);
    vkCmdCopyBufferToImage(cmdBuffer, texBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endCommandBuffer(device, commandPool, queue, cmdBuffer);

    // Transition GPU Read
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    cmdBuffer = beginCommandBuffer(device, commandPool);
    vkCmdPipelineBarrier(cmdBuffer, 
        srcStage, dstStage, 
        0, 
        0, nullptr,
        0, nullptr,
        1, &barrier);
    endCommandBuffer(device, commandPool, queue, cmdBuffer);
}

void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, 
                VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                VkImage& image, VkDeviceMemory& memory)
{
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = format;
    createInfo.tiling = tiling;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.flags = 0;
    assert( vkCreateImage(device, &createInfo, nullptr, &image) == VK_SUCCESS );

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);
    assert( vkAllocateMemory(device, &allocInfo, nullptr, &memory) == VK_SUCCESS );
    vkBindImageMemory(device, image, memory, 0);
}

void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageView& view)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    viewCreateInfo.subresourceRange.aspectMask = aspect;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    assert( vkCreateImageView(device, &viewCreateInfo, nullptr, &view) == VK_SUCCESS );
}