#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// #include "GLFWUtilities.h"

void Window::init()
{
    /// GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    m_windowWidth = 800;
    m_windowHeight = 600;
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Bending", nullptr, nullptr); // TODO: full screen only
    glfwSetWindowUserPointer(m_window, this);
    // glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback); // TODO: do I want to a resizable window?
}

void Window::cleanup()
{
    // GLFW
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::isClosing()
{
    return glfwWindowShouldClose(m_window);
}

GLFWwindow* const Window::Get() const
{
    return m_window;
}

void Window::update()
{
    glfwPollEvents();
}