#include "Engine.h"

#include "Window.h"
#include "Input.h"
#include "Renderer.h"

Window Engine::m_window;
Input Engine::m_input;
Renderer Engine::m_renderer;

void Engine::init()
{
    m_window.init();
    m_input.init(m_window.Get());
    m_renderer.init();
}

void Engine::cleanup()
{
    m_renderer.cleanup();
    m_input.cleanup();
    m_window.cleanup();
}

void Engine::update()
{
    m_window.update();
    m_input.update();
    m_renderer.update();
    
    // TODO: remove
    if (m_input.isKeyPressed(65))
    {
        m_renderer.angle += 0.1;
    } else if (m_input.isKeyPressed(68))
    {
        m_renderer.angle -= 0.1;
    }
}

void Engine::run()
{
    init();
    while(!m_window.isClosing())
    {
        update();
    }
    cleanup();
}
