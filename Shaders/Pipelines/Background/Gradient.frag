#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const int ColorCount = 2;
layout(constant_id = 1) const bool Vertical = true; // otherwise horizontal

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inColor;

// layout(binding = 0) uniform Uniform // TODO: not enabled
// {
//     vec3 colors[ColorCount];
// };
vec3 colors[] = vec3[]
(
    vec3(1.0,0.0,0.0),
    vec3(0.0,0.0,1.0)
);

// Azur Lane
// vec3 colors[] = vec3[]
// (
//     vec3(127.0f/255.0f,127.0f/255.0f,213.0f/255.0f),
//     vec3(134.0f/255.0f,168.0f/255.0f,231.0f/255.0f),
//     vec3(145.0f/255.0f,234.0f/255.0f,228.0f/255.0f)
// );

// Evening Night
// vec3 colors[] = vec3[]
// (
//     vec3(000.0f/255.0f,090.0f/255.0f,167.0f/255.0f),
//     vec3(255.0f/255.0f,253.0f/255.0f,228.0f/255.0f)
// );

// Zinc
// vec3 colors[] = vec3[]
// (
//     vec3(255.0f/255.0f,253.0f/255.0f,228.0f/255.0f),
//     vec3(242.0f/255.0f,242.0f/255.0f,242.0f/255.0f),
//     vec3(219.0f/255.0f,219.0f/255.0f,219.0f/255.0f),
//     vec3(234.0f/255.0f,234.0f/255.0f,234.0f/255.0f)
// );

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = subpassLoad(inColor);

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

    outColor = color.a * color + (1.0f - color.a) * vec4(lerpColor, 1.0);
}