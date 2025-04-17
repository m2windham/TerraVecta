#include "include/GameUI.h"
#include "include/Shader.h"
#include "include/Application.h"
#include "include/TextureAtlas.h"
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem> // For checking file existence
namespace fs = std::filesystem;

GameUI::GameUI()
    : m_application(nullptr), m_quadVAO(0), m_quadVBO(0), 
      m_fontTextureID(0), m_uiTextureID(0),
      m_showDebugInfo(true), m_showInventory(false)
{
}

GameUI::~GameUI() {
    // Clean up OpenGL resources
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_fontTextureID) glDeleteTextures(1, &m_fontTextureID);
    if (m_uiTextureID) glDeleteTextures(1, &m_uiTextureID);
}

bool GameUI::initialize() {
    // Update shader file paths to be relative to the executable
    std::string vertexShaderPath = "shaders/ui.vert";
    std::string fragmentShaderPath = "shaders/ui.frag";

    // Add debug logs to verify file existence using relative paths
    if (!fs::exists(vertexShaderPath)) {
        std::cerr << "ERROR: File not found at relative path: " << vertexShaderPath << std::endl;
    }
    if (!fs::exists(fragmentShaderPath)) {
        std::cerr << "ERROR: File not found at relative path: " << fragmentShaderPath << std::endl;
    }

    // Create UI shader using relative paths
    try {
        m_uiShader = std::make_unique<Shader>(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
    } catch (const std::exception& e) {
        std::cerr << "Failed to load UI shaders: " << e.what() << std::endl;
        return false;
    }
    
    // Set up the quad for UI rendering
    setupQuad();
    
    // Create a simple white texture for UI elements
    glGenTextures(1, &m_uiTextureID);
    glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
    
    // Create a 2x2 white texture
    unsigned char whitePixels[4 * 4] = {
        255, 255, 255, 255,  255, 255, 255, 255,
        255, 255, 255, 255,  255, 255, 255, 255
    };
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Also create a simple font texture (just a placeholder)
    glGenTextures(1, &m_fontTextureID);
    glBindTexture(GL_TEXTURE_2D, m_fontTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    std::cout << "Game UI initialized" << std::endl;
    return true;
}

void GameUI::render(float deltaTime, int fps, int selectedBlock, const glm::vec3& playerPos) {
    if (!m_uiShader) return;
    
    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    
    // Disable depth testing and enable alpha blending for UI
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Activate UI shader
    m_uiShader->use();
    
    // Render UI elements
    renderCrosshair();
    
    if (m_showDebugInfo) {
        renderDebugInfo(fps, playerPos);
    }
    
    renderBlockSelector(selectedBlock);
    
    if (m_showInventory) {
        renderInventory();
    }
    
    // Restore OpenGL state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    // For debug output in console (temporary until UI drawing works)
    static float updateInterval = 1.0f;
    static float timeAccumulator = 0.0f;
    
    timeAccumulator += deltaTime;
    if (timeAccumulator >= updateInterval) {
        timeAccumulator = 0.0f;
        
        // Format debug info
        std::ostringstream ss;
        ss << "FPS: " << fps 
           << " | Block: " << selectedBlock 
           << " | Pos: [" << std::fixed << std::setprecision(1) 
           << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << "]";
           
        // Log debug info
        std::cout << "\r" << ss.str() << std::flush;
    }
}

void GameUI::setupQuad() {
    if (m_quadVAO == 0) {
        // Vertex data for a simple quad (positions and texture coordinates)
        float quadVertices[] = {
            // positions        // texture coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f
        };
        
        // Create buffers
        glGenVertexArrays(1, &m_quadVAO);
        glGenBuffers(1, &m_quadVBO);
        
        // Set up vertex data
        glBindVertexArray(m_quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        
        // Texture coordinate attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glBindVertexArray(0);
    }
}

void GameUI::renderCrosshair() {
    // In a real implementation, this would render a crosshair in the center of the screen
    // For now, we'll just leave this as a stub
}

void GameUI::renderBlockSelector(int selectedBlock) {
    // In a real implementation, this would render the selected block in the UI
    // For now, we'll just leave this as a stub
}

void GameUI::renderDebugInfo(int fps, const glm::vec3& playerPos) {
    // In a real implementation, this would render debug info on screen
    // For now, we'll just leave this as a stub
}

void GameUI::renderInventory() {
    // In a real implementation, this would render the inventory
    // For now, we'll just leave this as a stub
}

// The duplicate setApplication method is removed - it's now defined inline in GameUI.h