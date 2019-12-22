#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inTex;

layout(location = 0) out vec4 outColor;

void main()
{
    // outColor = vec4(inColor, 1.0);
    outColor = subpassLoad(inTex);
}