#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Biome.h"
#include "TextureAtlas.h"
#include "BiomeManager.h"

class VoxelChunk {
public:
    // Enum for cube faces
    enum Face {
        Top, 
        Bottom, 
        Right, 
        Left, 
        Back, 
        Front
    };

    explicit VoxelChunk(int size);
    ~VoxelChunk();

    // Prevent copying
    VoxelChunk(const VoxelChunk&) = delete;
    VoxelChunk& operator=(const VoxelChunk&) = delete;

    // Access/modify voxel data
    int getVoxel(int x, int y, int z) const;
    void setVoxel(int x, int y, int z, int type);

    // Mesh generation and management
    void generateTerrain();
    void generateMesh();
    void generateOptimizedMesh();
    void setupMesh();
    void render();
    
    // Mesh update flags
    bool needsMeshUpdate() const;
    void markForMeshUpdate(bool update = true);
    void clearMeshUpdateFlag();

    // Set/get references to game systems
    void setBiomeManager(const BiomeManager* biomeManager);
    void setWorldPosition(const glm::ivec2& position);
    void setTextureAtlas(const TextureAtlas* atlas);
    const glm::ivec2& getWorldPosition() const;
    
    // Set seed for terrain generation
    void setSeed(unsigned int seed) { m_seed = seed; }

private:
    // Helper methods for mesh generation
    void addFace(int x, int y, int z, Face face);
    void addOptimizedFace(int x, int y, int z, Face face, int width, int height, int blockType);

    // Voxel data storage (3D grid)
    std::vector<std::vector<std::vector<int>>> m_voxelData;
    
    // Mesh data
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
    
    // OpenGL object IDs
    GLuint m_VAO, m_VBO, m_EBO;
    unsigned int m_indexCount;
    
    // Chunk properties
    int m_size;
    glm::ivec2 m_worldPosition;
    bool m_needsMeshUpdate;
    
    // References to game systems (not owned by this class)
    const BiomeManager* m_biomeManager;
    const TextureAtlas* m_textureAtlas;
    
    // Seed for terrain generation
    unsigned int m_seed = 42; // Default seed
};