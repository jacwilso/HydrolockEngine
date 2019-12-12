#ifndef INPUT_H
#define INPUT_H

class Input 
{
    public:
    bool isKeyPressed(int key); // TODO: maybe go to subscribtion system?
    bool isMousePressed(int button);
    class vec2 mousePosition();

    private:
    void init(struct GLFWwindow* const window);
    void cleanup();
    void update();

    struct GLFWwindow* m_window;

    friend class Engine;
};

#endif /* INPUT_H */