#ifndef VULKAN_UTILITIES_H
#define VULKAN_UTILITIES_H
#include <vulkan/vulkan.h>

const char* const requiredExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const int MAX_FRAMES_IN_FLIGHT = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator);

void AvailableExtensions();

void constructDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

#define GRAPHICS_BIT  0b00000001
#define PRESENT_BIT   0b00000010
#define TRANSFER_BIT  0b00000011
struct QueueFamilyIndices 
{
    uint8_t bitmask = 0;
    uint32_t graphicsFamily;
    uint32_t presentFamily;
#if TRANSFER_FAMILY
    uint32_t transferFamily;
#endif

    bool isComplete()
    {
        return (bitmask & GRAPHICS_BIT) == GRAPHICS_BIT &&
               (bitmask & PRESENT_BIT) == PRESENT_BIT &&
#if TRANSFER_FAMILY
               (bitmask & TRANSFER_BIT) == TRANSFER_BIT;
#else
                true;
#endif
    }    
};

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formatCount;
    VkPresentModeKHR *presentModes;
    uint32_t presentModeCount;
};

SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

VkSurfaceFormatKHR selectSwapSurfaceFormat(const VkSurfaceFormatKHR* const availableFormats, int availableFormatCount);

VkPresentModeKHR selectSwapPresentMode(const VkPresentModeKHR* const availablePresentModes, int availablePresentModeCount);

VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

VkShaderModule createShaderModule(const VkDevice& device, const char* const shaderPath);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

bool hasStencilComponent(VkFormat format);

VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);
void endCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer cmdBuffer);

void createBuffer(VkDevice device,
                  VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, 
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer& buffer,
                  VkDeviceMemory& bufferMemory);
void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, 
                VkBuffer src, VkBuffer dst, VkDeviceSize size);
void copyImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer texBuffer, VkImage image, uint32_t width, uint32_t height, VkFormat format);

void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, 
                VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                VkImage& image, VkDeviceMemory& memory);
void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageView& view);

#endif /* VULKAN_UTILITIES_H */