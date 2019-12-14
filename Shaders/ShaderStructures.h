#include "Math/vec2.h"
#include "Math/vec3.h"
#include "Math/mat4.h"

#include <glm/mat4x4.hpp>


#define VERTEX_INPUT_ATTRIBUTE(name, _location, _format)        \
    static VkVertexInputAttributeDescription name##Attribute()  \
    {                                                           \
        VkVertexInputAttributeDescription desc = {};            \
        desc.binding = 0;                                       \
        desc.location = _location;                              \
        desc.offset = offsetof(Vertex, name);                   \
        desc.format = _format;                                  \
        return desc;                                            \
    }

struct Vertex {
public:
    vec3 position;
    vec3 color;
    vec2 uv; // TODO: alignas?

    static VkVertexInputBindingDescription bindingDesc()
    {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    VERTEX_INPUT_ATTRIBUTE(position, 0, VK_FORMAT_R32G32B32_SFLOAT)
    VERTEX_INPUT_ATTRIBUTE(color, 1, VK_FORMAT_R32G32B32_SFLOAT)
    VERTEX_INPUT_ATTRIBUTE(uv, 2, VK_FORMAT_R32G32_SFLOAT)
};

struct UniformBufferObject
{
    // TODO: alignas
    // glm::mat4 modelViewProjection;
    mat4 modelViewProjection;
    float time;
};