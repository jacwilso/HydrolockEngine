#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h> // TODO: forward declare

class Renderer {
public:
    // TODO: remove
    float angle = 0;
    int renderMode;
    void cycleMode();

private:
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
    VkDescriptorSetLayout m_descriptorLayout;
    VkPipelineLayout m_pipelineLayout;
    VkRenderPass m_renderPass;
    VkPipeline m_pipeline;
#if EDITOR
    VkPipeline m_pipelineWireframe;
#endif
    // Buffers
    VkFramebuffer* m_swapChainFramebuffers;
    VkCommandPool m_commandPool;
#if TRANSFER_FAMILY
    VkCommandPool m_transferCommandPool;
#endif
    VkCommandBuffer* m_commandBuffers;

    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;

    VkBuffer m_vertexIndexBuffer;
    VkDeviceMemory m_vertexIndexBufferMemory;
    
    VkBuffer* m_uniformBuffers;
    VkDeviceMemory* m_uniformBuffersMemory;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet* m_descriptorSets;
    
    VkImage m_texImage;
    VkDeviceMemory m_texImageMemory;
    VkImageView m_texImageView;
    VkSampler m_texSampler;
    // Synchronization
    VkSemaphore* m_imageAcquired;
    VkSemaphore* m_renderCompleted;
    VkFence* m_framesInFlight;
    VkFence* m_imagesInFlight;
    size_t m_currentFrame;

    void init();
    void update();
    void update(uint32_t currentImage);
    void cleanup();

    void createVulkanInstance();
    void createVulkanDevice();
    void createVulkanSwapChain();
    void createVulkanPipeline();
    void createVulkanBuffers();

    void recreateVulkanSwapChain();
    void cleanupVulkanSwapChain();

    friend class Engine;
};

#endif /* RENDERER_H */


/* TODO:
* overload std::shared_ptr using RAII (Base Code)
* custom allocator
* https://vulkan-tutorial.com/Loading_models
*/