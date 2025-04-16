#ifndef WATER_SYSTEM_H
#define WATER_SYSTEM_H

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <memory>

class VoxelChunk;
class Application;

// Class to handle water physics in the game
class WaterSystem {
public:
    WaterSystem();
    ~WaterSystem() = default;

    // Initialize water system
    void initialize(int seed);
    
    // Update water flow for a specific chunk
    void updateWaterFlow(VoxelChunk* chunk, const glm::ivec2& chunkPos);
    
    // Apply buoyancy to physics objects in water
    void applyBuoyancy(glm::vec3& position, glm::vec3& velocity, float deltaTime);
    
    // Check if a world position is underwater
    bool isUnderwater(const glm::vec3& worldPos, VoxelChunk* chunk = nullptr);
    
    // Get water level (can vary based on position)
    float getWaterLevel(const glm::vec3& worldPos);
    
    // Set the application reference for getting chunks
    void setApplication(Application* app) { m_application = app; }
    
    // Handle water animation
    void updateAnimation(float deltaTime);
    
    // Get current water animation offset for rendering
    float getWaterAnimationOffset() const { return m_waterAnimationOffset; }

private:
    // Spread water in a given chunk (affects adjacent chunks too)
    void spreadWater(VoxelChunk* chunk, const glm::ivec2& chunkPos, int x, int y, int z);
    
    // Check if a block can be replaced by water
    bool canReplaceWithWater(int blockType) const;
    
    // Water flow properties
    float m_flowRate = 0.5f;           // Rate at which water spreads
    float m_viscosity = 0.8f;          // How much water slows down movement
    float m_buoyancy = 9.8f;           // Upward force in water
    
    // Water animation 
    float m_waterAnimationOffset = 0.0f;
    float m_waterAnimationSpeed = 0.2f;
    
    // Reference to main application (for chunk access)
    Application* m_application = nullptr;
    
    // Random number generator seed
    int m_seed = 0;
    
    // Track pending water updates
    struct WaterUpdate {
        glm::ivec3 position;
        int flowDirection; // 0-5 for six possible directions
        float pressure;    // Higher pressure flows faster/further
    };
    std::vector<WaterUpdate> m_pendingUpdates;
};

#endif // WATER_SYSTEM_H