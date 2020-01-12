#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h> // TODO: forward declare
#include "RenderStructs.h"
#include "Math/mat4.h"

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
    uint32_t m_graphicsFamily;
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
    VkDescriptorSetLayout m_compositionDescriptorLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipelineLayout m_compositionPipelineLayout;

    RenderPass m_renderPass;
    Pipeline m_pipeline;
    // Buffers
    VkFramebuffer* m_sceneFramebuffers;
    VkCommandPool m_commandPool;
#if TRANSFER_FAMILY
    VkCommandPool m_transferCommandPool;
#endif
    VkCommandBuffer* m_commandBuffers; // TODO: break into secondary for background + composition? and imgui

    Attachment m_attachments;

    VkBuffer m_vertexIndexBuffer;
    VkDeviceMemory m_vertexIndexBufferMemory;
    
    VkBuffer* m_uniformBuffers;
    VkDeviceMemory* m_uniformBuffersMemory;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet* m_descriptorSets;
    VkDescriptorPool m_compositionDescriptorPool; // TODO: 1 descriptor pool
    VkDescriptorSet* m_compositionDescriptorSets; // TODO: 1 descriptor set with different array indicies
    
    VkImage* m_texImage;
    VkDeviceMemory* m_texImageMemory;
    VkImageView* m_texImageView;
    VkSampler m_texSampler;
    // Synchronization
    VkSemaphore* m_imageAcquired;
    VkSemaphore* m_renderCompleted;
    VkFence* m_framesInFlight;
    VkFence* m_imagesInFlight;
    size_t m_currentFrame;

    // Imgui
    VkDescriptorPool m_imguiDescriptorPool;
    VkFramebuffer* m_imguiFramebuffers;
    VkCommandPool* m_imguiCommandPools;
    VkCommandBuffer* m_imguiCommandBuffers;

    // TODO: remove
    uint32_t m_instances = 2;
    mat4 model[64];
    void loadImageInstance(char filePath[64], float position[3]);
    //

    void init();
    void update();
    void update(uint32_t frameIndex);
    void cleanup();

    void createVulkanInstance();
    void createVulkanDevice();
    void createVulkanSwapChain();
    void createVulkanPipeline();
    void createVulkanBuffers();
    void createVulkanDrawCmds();

    void createImguiContext();
    void cleanupImguiContext();
    void updateImgui(uint32_t frameIndex);
    void renderImgui();
    void recreateImguiSwapChain();

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