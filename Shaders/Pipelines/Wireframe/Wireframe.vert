#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 modelViewProj;
} ubo;

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = ubo.modelViewProj * vec4(inPosition, 1.0);
}
