#include "include/WaterSystem.h"
#include "include/Application.h"
#include "include/VoxelChunk.h"
#include <iostream>
#include <queue>
#include <random>
#include <algorithm>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

WaterSystem::WaterSystem() 
    : m_application(nullptr), m_seed(0), m_waterAnimationOffset(0.0f),
      m_flowRate(0.5f), m_viscosity(0.8f), m_buoyancy(9.8f), m_waterAnimationSpeed(0.2f)
{
}

// Removed duplicate destructor definition since it's defined as default in the header

void WaterSystem::initialize(int seed) {
    m_seed = seed;
    m_pendingUpdates.clear();
    m_waterAnimationOffset = 0.0f;
    
    std::cout << "Water System initialized with seed: " << seed << std::endl;
}

void WaterSystem::updateAnimation(float deltaTime) {
    // Update water animation offset
    m_waterAnimationOffset += m_waterAnimationSpeed * deltaTime;
    // Keep within 0-1 range for texture coordinate offset
    if (m_waterAnimationOffset > 1.0f) {
        m_waterAnimationOffset -= 1.0f;
    }
}

// Removed duplicate setApplication method since it's defined inline in the header

bool WaterSystem::isUnderwater(const glm::vec3& worldPos, VoxelChunk* chunk) {
    // Since we can't access Application::getChunkFromWorldPos directly,
    // we'll implement a simplified version that uses the passed-in chunk
    if (!chunk) {
        // If no chunk is provided and we can't access private Application methods,
        // use a simple height-based check instead
        return worldPos.y < getWaterLevel(worldPos);
    }
    
    // Convert world position to local chunk coordinates
    int chunkSize = 16; // Use the chunk size (fixed at 16)
    int localX = static_cast<int>(std::floor(worldPos.x)) % chunkSize;
    if (localX < 0) localX += chunkSize;
    
    int localY = static_cast<int>(std::floor(worldPos.y));
    
    int localZ = static_cast<int>(std::floor(worldPos.z)) % chunkSize;
    if (localZ < 0) localZ += chunkSize;
    
    // Check if Y coordinate is within chunk bounds
    if (localY < 0 || localY >= chunkSize) {
        return false;
    }
    
    // Check if the voxel at this position is water (type 3)
    return chunk->getVoxel(localX, localY, localZ) == 3;
}

float WaterSystem::getWaterLevel(const glm::vec3& worldPos) {
    // For simplicity, use a constant water level for now
    // Could be enhanced with noise functions for waves or biome-specific levels
    return 16.0f * 0.3f; // About 1/3 of the chunk height
}

void WaterSystem::applyBuoyancy(glm::vec3& position, glm::vec3& velocity, float deltaTime) {
    // Check if the position is underwater
    if (isUnderwater(position)) {
        // Apply buoyancy force (upward)
        velocity.y += m_buoyancy * deltaTime;
        
        // Apply water resistance/viscosity (slows down movement)
        velocity *= (1.0f - m_viscosity * deltaTime);
    }
}

void WaterSystem::updateWaterFlow(VoxelChunk* chunk, const glm::ivec2& chunkPos) {
    if (!chunk) return;
    
    int chunkSize = 16; // Fixed chunk size (instead of using getSize())
    m_pendingUpdates.clear();
    
    // Scan the chunk for water blocks
    for (int x = 0; x < chunkSize; x++) {
        for (int z = 0; z < chunkSize; z++) { // Fixed: was using x < chunkSize instead of z < chunkSize
            for (int y = 0; y < chunkSize; y++) {
                if (chunk->getVoxel(x, y, z) == 3) { // If this is a water block
                    spreadWater(chunk, chunkPos, x, y, z);
                }
            }
        }
    }
    
    // Process all pending water updates
    bool chunkModified = false;
    for (const auto& update : m_pendingUpdates) {
        // Convert to local chunk coordinates
        int chunkX = update.position.x / chunkSize;
        int chunkZ = update.position.z / chunkSize;
        
        int localX = ((update.position.x % chunkSize) + chunkSize) % chunkSize;
        int localY = update.position.y;
        int localZ = ((update.position.z % chunkSize) + chunkSize) % chunkSize;
        
        // Get the correct chunk for this update
        VoxelChunk* targetChunk = chunk;
        if (chunkX != chunkPos.x || chunkZ != chunkPos.y) {
            // This update affects a different chunk
            // We can't use m_application->getChunk directly, so we'll skip updates to different chunks
            continue;
        }
        
        if (targetChunk && localY >= 0 && localY < chunkSize) {
            int currentBlock = targetChunk->getVoxel(localX, localY, localZ);
            if (canReplaceWithWater(currentBlock)) {
                targetChunk->setVoxel(localX, localY, localZ, 3); // Set to water
                chunkModified = true;
            }
        }
    }
    
    // If the chunk was modified, mark it for mesh update
    if (chunkModified) {
        chunk->markForMeshUpdate();
    }
}

void WaterSystem::spreadWater(VoxelChunk* chunk, const glm::ivec2& chunkPos, int x, int y, int z) {
    if (!chunk) return;
    
    int chunkSize = 16; // Fixed chunk size
    
    // Calculate world position for this water block
    int worldX = chunkPos.x * chunkSize + x;
    int worldY = y;
    int worldZ = chunkPos.y * chunkSize + z;
    
    // Directions: down, north, east, south, west
    static const int dx[] = {0, 0, 1, 0, -1};
    static const int dy[] = {-1, 0, 0, 0, 0};
    static const int dz[] = {0, -1, 0, 1, 0};
    
    // Always try to flow downward first
    for (int dir = 0; dir < 5; dir++) {
        int nx = worldX + dx[dir];
        int ny = worldY + dy[dir];
        int nz = worldZ + dz[dir];
        
        // Calculate local position in chunk
        int nChunkX = nx / chunkSize;
        int nChunkZ = nz / chunkSize;
        
        int nLocalX = ((nx % chunkSize) + chunkSize) % chunkSize;
        int nLocalY = ny;
        int nLocalZ = ((nz % chunkSize) + chunkSize) % chunkSize;
        
        // Check if we need to access a different chunk
        VoxelChunk* targetChunk = chunk;
        if (nChunkX != chunkPos.x || nChunkZ != chunkPos.y) {
            // We can't use m_application->getChunk directly,
            // so we'll skip updates to different chunks for now
            continue;
        }
        
        // Check if target position is valid and can be replaced with water
        if (targetChunk && nLocalY >= 0 && nLocalY < chunkSize) {
            int targetBlock = targetChunk->getVoxel(nLocalX, nLocalY, nLocalZ);
            if (canReplaceWithWater(targetBlock)) {
                // Schedule this block to be replaced with water
                float pressure = 1.0f;
                if (dir == 0) pressure = 1.5f; // Higher pressure for downward flow
                
                m_pendingUpdates.push_back({glm::ivec3(nx, ny, nz), dir, pressure});
                
                // If flowing downward, no need to check other directions
                if (dir == 0) break;
            }
        }
    }
}

bool WaterSystem::canReplaceWithWater(int blockType) const {
    // Air (0) can always be replaced with water
    // Implement other rules as needed (e.g., certain plants)
    return blockType == 0;
}