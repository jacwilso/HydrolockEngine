#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include <vulkan/vulkan.h> // TODO: forward declare

struct Attachment // TODO: should I make attachment its own thing?
{
    VkImage color;
    VkImage depth;

    VkImageView colorView;
    VkImageView depthView;

    VkDeviceMemory colorMemory;
    VkDeviceMemory depthMemory;

    void create(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat colorFormat);
    void destroy(VkDevice device);
};

struct Pipeline
{
    VkPipeline scene;
    VkPipeline composition; // TODO: do I want a background and composition or just 1
    VkPipeline imgui;
#if EDITOR
    VkPipeline wireframe;
#endif

    void create(VkDevice device);
    void destroy(VkDevice device);

    private:
    void createScene();
    void createComposition();
    void createImgui();
#if EDITOR
    void createWireframe();
#endif
};

struct RenderPass
{
    VkRenderPass scene;
    VkRenderPass imgui;

    void create(VkDevice device);
    void destroy(VkDevice device);
};

#endif /* RENDER_STRUCTS_H */