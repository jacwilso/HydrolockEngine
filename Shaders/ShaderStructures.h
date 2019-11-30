#include "Math/vec2.h"
#include "Math/vec3.h"

struct Vertex {
private:
    inline static VkVertexInputAttributeDescription attribDesc[2];

public:
    vec3 position;
    vec3 color;

    static VkVertexInputBindingDescription bindingDesc()
    {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    const static uint32_t attributeDescCount = 2;
    static VkVertexInputAttributeDescription* attributeDesc()
    {
        Vertex::attribDesc[0].binding = 0;
        Vertex::attribDesc[0].location = 0;
        Vertex::attribDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        Vertex::attribDesc[0].offset = offsetof(Vertex, position);

        Vertex::attribDesc[1].binding = 0;
        Vertex::attribDesc[1].location = 1;
        Vertex::attribDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        Vertex::attribDesc[1].offset = offsetof(Vertex, color);
        return Vertex::attribDesc;
    }
};