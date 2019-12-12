#include "Input.h"

#include <GLFW/glfw3.h>
#include "Math/vec2.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{    
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
}

void Input::init(GLFWwindow* const window)
{
    this->m_window = window;
    // glfwSetKeyCallback(window, keyCallback);
    // glfwSetMouseButtonCallback(window, mouseCallback);
}

void Input::cleanup()
{
    glfwSetKeyCallback(m_window, nullptr);
}

void Input::update()
{
}

//////////
// Public
//////////

bool Input::isKeyPressed(int key)
{
    int state = glfwGetKey(m_window, key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::isMousePressed(int btn)
{
    return glfwGetMouseButton(m_window, btn) == GLFW_PRESS;
}

vec2 Input::mousePosition()
{
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    return vec2(x, y);
}

