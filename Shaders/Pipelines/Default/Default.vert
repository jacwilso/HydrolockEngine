#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 modelViewProj;
    float time;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
    mat4 model;
	int instanceID;
} constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

void main()
{
    gl_Position = ubo.modelViewProj * constants.model * vec4(inPosition, 1.0);

    outColor = inColor;
    outUV = inUV;
}
