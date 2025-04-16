#ifndef WINDOW_H
#define WINDOW_H

// Forward declaration to avoid including glfw3.h in the header
struct GLFWwindow;

class Window {
public:
    // Constructor: Initializes GLFW and creates the window and OpenGL context
    Window(int width, int height, const char* title);
    // Destructor: Cleans up GLFW resources
    ~Window();

    // Prevent copying/assignment
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Checks if the window should close (e.g., user clicked the close button)
    bool shouldClose() const;

    // Processes events (keyboard, mouse, resize, etc.)
    void pollEvents() const;

    // Swaps the front and back buffers to display the rendered frame
    void swapBuffers() const;

    void setShouldClose(bool value); // <--- ADD THIS DECLARATION

    // Getters
    GLFWwindow* getNativeWindow() const { return m_Window; } // <--- DEFINITION IN HEADER
    int getWidth() const { return m_Width; }                 // <--- DEFINITION IN HEADER
    int getHeight() const { return m_Height; }               // <--- DEFINITION IN HEADER

private:
    GLFWwindow* m_Window = nullptr;
    int m_Width;
    int m_Height;

    // Static GLFW error callback function
    static void errorCallback(int error, const char* description);
    // Static GLFW framebuffer resize callback function
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    // Initialization helper
    bool initializeGLFW();
    // Window creation helper
    bool createWindow(const char* title);
};

#endif // WINDOW_H
 