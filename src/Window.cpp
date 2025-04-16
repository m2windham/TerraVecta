// h:\Repo\TerraVecta\src\Window.cpp

#include "include/Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept> // For std::runtime_error

// --- Static Callback Implementations ---

// Called by GLFW when an error occurs.
void Window::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// Called by GLFW when the window's framebuffer is resized.
void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Retrieve the Window instance associated with the GLFWwindow
    Window* windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (windowInstance) {
        // Update the stored dimensions
        windowInstance->m_Width = width;
        windowInstance->m_Height = height;

        // Set the OpenGL viewport to match the new window size
        // This tells OpenGL how to map normalized device coordinates to window coordinates.
        glViewport(0, 0, width, height);

        std::cout << "Window resized to " << width << "x" << height << std::endl;
        // If you have projection matrices or other size-dependent resources,
        // you might want to signal an update here.
    }
}

// --- Constructor ---
// Initializes GLFW, creates the window, sets up OpenGL context, and loads GLAD.
Window::Window(int width, int height, const char* title)
    : m_Width(width), m_Height(height), m_Window(nullptr) { // Initialize m_Window to nullptr

    if (!initializeGLFW()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    if (!createWindow(title)) {
        glfwTerminate(); // Clean up GLFW if window creation fails
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Set the user pointer for this GLFWwindow to point to this Window instance.
    // This is crucial for retrieving the 'this' pointer in static callbacks.
    glfwSetWindowUserPointer(m_Window, this);

    // Register GLFW callbacks
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);
    // Add other callbacks here as needed (e.g., key input, mouse input)
    // glfwSetKeyCallback(m_Window, keyCallback);
    // glfwSetCursorPosCallback(m_Window, mouseCallback);

    // Make the OpenGL context of the created window current on this thread.
    // All subsequent OpenGL calls will affect this context.
    glfwMakeContextCurrent(m_Window);

    // Initialize GLAD: Load all OpenGL function pointers.
    // This MUST be done AFTER making the context current.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_Window); // Clean up window
        glfwTerminate();             // Clean up GLFW
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Set the initial OpenGL viewport size based on window dimensions.
    glViewport(0, 0, m_Width, m_Height);

    // Configure global OpenGL state if needed (optional here)
    // e.g., glEnable(GL_DEPTH_TEST);

    // Optional: Enable VSync (1 = sync to monitor refresh rate, 0 = immediate swap)
    glfwSwapInterval(1);

    // Print diagnostic information
    std::cout << "Window and OpenGL Context created successfully." << std::endl;
    std::cout << "  OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "  GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << std::endl;
}

// --- Destructor ---
// Destroys the GLFW window and terminates GLFW.
Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr; // Prevent dangling pointer use
    }
    // Terminate GLFW, cleaning up all its resources.
    // This should generally be called only once when the application exits.
    glfwTerminate();
    std::cout << "Window destroyed and GLFW terminated." << std::endl;
}

// --- Private Helper Methods ---

// Initializes the GLFW library and sets window hints.
bool Window::initializeGLFW() {
    // Set the error callback first so initialization errors are reported.
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Set window hints for the next glfwCreateWindow call.
    // These configure properties like OpenGL version and profile.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Request OpenGL 3.x
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Request OpenGL 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use the Core profile (no deprecated functions)
#ifdef __APPLE__
    // Required on macOS to use Core profile features.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // Add other hints if needed (e.g., GLFW_RESIZABLE, GLFW_SAMPLES for MSAA)
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return true;
}

// Creates the actual GLFW window.
bool Window::createWindow(const char* title) {
    // Create the window object.
    // Parameters: width, height, title, monitor (for fullscreen), share (for context sharing)
    m_Window = glfwCreateWindow(m_Width, m_Height, title, nullptr, nullptr);
    if (!m_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return false;
    }
    return true;
}

// --- Public Methods ---

// Checks if the window has been flagged to close (e.g., by user clicking 'X').
bool Window::shouldClose() const {
    if (m_Window) {
        return glfwWindowShouldClose(m_Window);
    }
    return true; // If window doesn't exist, act as if it should close.
}

// *** NEW METHOD ***
// Sets the window's should-close flag.
void Window::setShouldClose(bool value) {
    if (m_Window) {
        // Use GL_TRUE/GL_FALSE constants for boolean values in GLFW/OpenGL C APIs
        glfwSetWindowShouldClose(m_Window, value ? GL_TRUE : GL_FALSE);
    } else {
        // Log a warning if trying to operate on a non-existent window
        std::cerr << "Warning: Attempted to call setShouldClose on a null window pointer." << std::endl;
    }
}

// Processes pending window events (input, resize, etc.).
void Window::pollEvents() const {
    if (m_Window) {
        // Processes events that have already been received and triggers callbacks.
        // Returns immediately if no events are pending.
        glfwPollEvents();
        // For more responsive input, consider glfwWaitEvents() or glfwWaitEventsTimeout()
        // if the application doesn't need to render continuously.
    }
}

// Swaps the front and back buffers of the window (displays the rendered frame).
void Window::swapBuffers() const {
    if (m_Window) {
        glfwSwapBuffers(m_Window);
    }
}