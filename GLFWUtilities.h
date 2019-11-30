#ifndef GLFW_UTILITIES_H
#define GLFW_UTILITIES_H
#include <GLFW/glfw3.h>

bool framebufferResized = false; // TODO: remove when just fullscreen 
static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    framebufferResized = true;
}

#endif /* GLFW_UTILITIES_H */