#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include <vulkan/vulkan.h> // TODO: forward declare

struct Attachment { // TODO: should I make attachment its own thing?
    VkImage color;
    VkImage depth;

    VkImageView colorView;
    VkImageView depthView;

    VkDeviceMemory colorMemory;
    VkDeviceMemory depthMemory;

    void create(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat colorFormat);
    void destroy(VkDevice device);
};

#endif /* RENDER_STRUCTS_H */