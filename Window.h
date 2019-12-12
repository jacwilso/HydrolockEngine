#ifndef WINDOW_H
#define WINDOW_H

struct GLFWwindow;

class Window
{
    private:
    GLFWwindow* m_window;
    int m_windowWidth, m_windowHeight;
    
    void init();
    void cleanup();
    void update();

    public:
    GLFWwindow* const Get() const;

    bool isClosing();
    
    friend class Engine;
};

#endif /* WINDOW_H */