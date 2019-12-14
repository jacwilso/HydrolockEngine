#ifndef MANAGER_H
#define MANAGER_H

class Engine
{
    private:
    static class Window m_window;
    static class Input m_input;
    static class Renderer m_renderer;

    void init();
    void update();
    void cleanup();

    void gameUpdate();

    public:
    void run();

    friend class Input;
    friend class Window;
    friend class Renderer;
};

#endif /* MANAGER_H */