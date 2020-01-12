#include "SpriteRenderer.h"

uint16_t SpriteRenderer::activeSprites = 0;

SpriteRenderer::SpriteRenderer(vec2 position, vec2 rotation, vec2 scale, const char* file)
{
    /*
    renderers[activeSprites++] = this;

        int texWidth, texHeight, texChannels;
    stbi_uc* texPixels = stbi_load("Resources/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize texSize = texWidth * texHeight * 4;
    assert( texPixels );
    createBuffer(m_device, m_physicalDevice,
                texSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                BufferMemoryProperty,
                stagingBuffer, stagingBufferMemory);
    vkMapMemory(m_device, stagingBufferMemory, 0, texSize, 0, &data);
    memcpy(data, texPixels, texSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    stbi_image_free(texPixels);

    createImage(m_device, m_physicalDevice, texWidth, texHeight, 
        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
        m_texImage, m_texImageMemory);

#if TRANSFER_FAMILY
    copyImage(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_texImage, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
#else
    copyImage(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_texImage, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
#endif
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    createImageView(m_device, m_texImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_texImageView);
    */
}
