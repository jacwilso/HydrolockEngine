#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D mainTex[64];

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 68) int instanceID;
} constants;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(mainTex[constants.instanceID], texSampler), inUV);
    outColor.xyz *= inColor;
}