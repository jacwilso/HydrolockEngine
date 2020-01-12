#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 modelViewProj;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
    mat4 model;
	int instanceID;
} constants;

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = ubo.modelViewProj * constants.model * vec4(inPosition, 1.0);
}
