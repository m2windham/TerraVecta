#ifndef APPLICATION_H
#define APPLICATION_H
#include <glad/glad.h> // For GLuint
#include <memory> // For unique_ptr
#include "Shader.h" // Include Shader header
#include <vector> // For VoxelChunk
#include "VoxelChunk.h" // Include the VoxelChunk header file
#include <unordered_map>
#include <string>
#include <random> // Add include for random number generation
#include <btBulletDynamicsCommon.h>
#include <glm/gtx/hash.hpp> // For glm::ivec2 hashing
#include <glm/glm.hpp>
#include <optional> // For raycast result
#include "Inventory.h"
#include "CraftingSystem.h"
#include "PhysicsEngine.h"
#include "Frustum.h" // Add include for Frustum class
#include "BiomeManager.h" // Include BiomeManager header
#include "TextureAtlas.h" // Include TextureAtlas header
#include "WaterSystem.h" // Include WaterSystem header

// Forward declarations
class Window;
class Shader;
class VoxelChunk;
class GameUI; // Add GameUI forward declaration

// Hash function for glm::ivec2 used in unordered_map
struct ivec2Hash {
    std::size_t operator()(const glm::ivec2& v) const {
        // Combine hashes of x and y components
        // Simple hash combine, consider more robust methods if needed
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};

class Application {
public:
    Application(int windowWidth, int windowHeight, const char* windowTitle);
    ~Application();

    // Prevent copying/assignment
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Main application loop
    void run();

private:
    // Initialize application subsystems
    bool initialize();
    // Update game state
    void update(float deltaTime);
    // Render the scene
    void render(float deltaTime); // Modified to take deltaTime parameter
    // Clean up resources
    void shutdown();
    void processInput(float deltaTime);

    void initializeVoxelSystem();
    void updateChunks(); // Declaration for updateChunks
    bool isChunkInView(const std::unique_ptr<VoxelChunk>& chunk);

    // Camera properties
    glm::vec3 m_cameraPosition{0.0f, 0.0f, 3.0f};
    float m_cameraSpeed = 2.5f;
    float m_cameraYaw = -90.0f;
    float m_cameraPitch = 0.0f;
    float m_mouseSensitivity = 0.1f;
    double m_lastMouseX = 400.0;
    double m_lastMouseY = 300.0;

    // Method for voxel manipulation
    void handleVoxelManipulation();

    std::unique_ptr<Window> m_Window; // Owning pointer to the Window object

    bool m_IsRunning = false;
    float m_LastFrameTime = 0.0f;

    std::unique_ptr<Shader> m_ShaderProgram; // To hold our shader
    GLuint m_textureID = 0; // Texture ID

    // Define constants
    const int CHUNK_SIZE = 16;
    const int RENDER_DISTANCE = 5;

    // Store loaded chunks using their position as key
    std::unordered_map<glm::ivec2, std::unique_ptr<VoxelChunk>, ivec2Hash> m_loadedChunks;

    // Game Systems
    std::unique_ptr<Inventory> m_inventory;
    std::unique_ptr<CraftingSystem> m_craftingSystem;
    std::unique_ptr<PhysicsEngine> m_physicsEngine;

    // Biome system
    std::unique_ptr<BiomeManager> m_biomeManager;

    // View frustum for culling
    Frustum m_viewFrustum;

    // Texture atlas for block textures
    std::unique_ptr<TextureAtlas> m_textureAtlas;

    // Water system
    std::unique_ptr<WaterSystem> m_waterSystem;
    
    // Game UI system for HUD and menus
    std::unique_ptr<GameUI> m_gameUI;

    // Random number generation
    unsigned int m_seed;
    std::mt19937 m_randomGenerator;
    std::uniform_real_distribution<float> m_noiseDistribution;
    
    // Helper method to get texture atlas (for UI)
    TextureAtlas* getTextureAtlas() const { return m_textureAtlas.get(); }

    // Helper method to get a chunk at a specific position
    VoxelChunk* getChunk(const glm::ivec2& chunkPos) {
        auto it = m_loadedChunks.find(chunkPos);
        return (it != m_loadedChunks.end()) ? it->second.get() : nullptr;
    }

    // Raycast result structure
    struct RaycastResult {
        glm::ivec3 voxelPos; // World coordinates of the voxel
        glm::ivec3 faceNormal; // Normal of the face hit (-1, 0, or 1 in each component)
    };
    std::optional<RaycastResult> raycastVoxel(const glm::vec3& start, const glm::vec3& direction, float maxDist);
    VoxelChunk* getChunkFromWorldPos(const glm::ivec3& worldPos, glm::ivec3& localPos);

    // Mouse Button State Tracking
    bool m_leftMouseDown = false;
    bool m_rightMouseDown = false;
};

#endif // APPLICATION_H
