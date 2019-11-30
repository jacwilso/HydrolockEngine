#include "VulkanUtilities.h"

#include <assert.h>
#include <iostream>
#include <fstream>

#include "Utilities.h"

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

bool isDeviceSutiable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
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

    // TODO: don't care about GPU right now
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
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

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dst, VkDeviceSize size)
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

    VkBufferCopy copy = {};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copy);
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue); // TODO: use a fence to schedule multiple transfers
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
}