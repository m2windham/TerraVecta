#ifndef GAME_UI_H
#define GAME_UI_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

class Shader;
class Application;

// Class to handle the game's user interface
class GameUI {
public:
    GameUI();
    ~GameUI();

    // Initialize the UI system
    bool initialize();
    
    // Render the UI
    void render(float deltaTime, int fps, int selectedBlock, const glm::vec3& playerPos);
    
    // Set the application reference
    void setApplication(Application* app) { m_application = app; }
    
    // Toggle UI elements
    void toggleDebugInfo() { m_showDebugInfo = !m_showDebugInfo; }
    void toggleInventory() { m_showInventory = !m_showInventory; }

private:
    // Helper methods
    void setupQuad(); // Setup the quad for rendering UI elements
    void renderCrosshair(); // Render the crosshair
    void renderBlockSelector(int selectedBlock); // Render the selected block
    void renderDebugInfo(int fps, const glm::vec3& playerPos); // Render debug information
    void renderInventory(); // Render the inventory
    
    // OpenGL objects
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    GLuint m_fontTextureID = 0;
    GLuint m_uiTextureID = 0;
    
    // Shader for UI rendering
    std::unique_ptr<Shader> m_uiShader;
    
    // UI state
    bool m_showDebugInfo = true;
    bool m_showInventory = false;
    
    // Reference to main application
    Application* m_application = nullptr;
};

#endif // GAME_UI_H