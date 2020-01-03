#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const int ColorCount = 2;
layout(constant_id = 1) const bool Vertical = true; // otherwise horizontal

// layout(binding = 0) uniform Uniform
// {
//     vec3 colors[ColorCount];
// };
vec3 colors[] = vec3[]
(
    vec3(1.0,0.0,0.0),
    vec3(0.0,0.0,1.0)
);

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
    float t = (ColorCount - 1.0f) * val;
    int prev = int(floor(t));
    int next = int(ceil(t));
    t = (next - t) / (next - prev);
    vec3 lerpColor = colors[prev] * t + colors[next] * (1 - t);
    outColor = vec4(lerpColor, 1);
}