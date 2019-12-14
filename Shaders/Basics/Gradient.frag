#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const int ColorCount = 2;
layout(constant_id = 1) const bool Vertical = true; // otherwise horizontal

layout(binding = 0) uniform Uniform
{
    vec3 colors[ColorCount];
};

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
    float val = 0;
    if (Vertical)
    {
        val = inUV.y;
    } else
    {
        val = inUV.x;
    }
    float percent = (ColorCount - 1) * val;
    int prev = int(floor(percent));
    int next = int(ceil(percent));
    outColor = vec4(0.5f * (colors[prev] + colors[next]), 1.0f);
}