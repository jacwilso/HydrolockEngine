#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PER_OBJECT
{
	int instanceID;
} constants;

void main()
{
    outColor = vec4(0.1f, 0.1f, 0.1f, 1.0);
}