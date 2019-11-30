#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h> // TODO: forward declare

struct GLFWwindow;

class Renderer {
public:
    void run();

private:
    GLFWwindow* m_window;
    int m_windowWidth, m_windowHeight;

    // Instance
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    // Device
    VkPhysicalDevice m_physicalDevice; // TODO: only needed on init
    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkQueue m_graphicsQueue; // TODO: only needed on init
    VkQueue m_presentQueue; // TODO: only needed on init
#if TRANSFER_FAMILY
    VkQueue m_transferQueue; // TODO: only needed on init
#endif
    // Swap Chain
    VkSwapchainKHR m_swapChain;
    VkExtent2D m_swapChainExtent; // TODO: only needed on init
    VkFormat m_swapChainImageFormat; // TODO: only needed on init
    uint32_t m_swapChainImageCount;
    VkImage* m_swapChainImages;
    VkImageView* m_swapChainImageViews;
    // Pipeline
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    // Buffers
    VkFramebuffer* m_swapChainFramebuffers;
    VkCommandPool m_commandPool;
#if TRANSFER_FAMILY
    VkCommandPool m_transferCommandPool;
#endif
    VkCommandBuffer* m_commandBuffers;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    // VkBuffer m_indexBuffer;
    // VkDeviceMemory m_indexBufferMemory;
    // Synchronization
    VkSemaphore* m_imageAcquired;
    VkSemaphore* m_renderCompleted;
    VkFence* m_framesInFlight;
    VkFence* m_imagesInFlight;
    size_t m_currentFrame;

    void init();
    void mainLoop();
    void render();
    void cleanup();

    void createVulkanInstance();
    void createVulkanDevice();
    void createVulkanSwapChain();
    void createVulkanPipeline();
    void createVulkanBuffers();

    void recreateVulkanSwapChain();
    void cleanupVulkanSwapChain();
};

#endif /* RENDERER_H */


/* TODO:
* overload std::shared_ptr using RAII (Base Code)
* custom allocator
*/