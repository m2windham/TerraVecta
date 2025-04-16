#include "include/VoxelChunk.h"
#include "include/Biome.h"
#include "include/BiomeManager.h"
#include "include/TextureAtlas.h"
#include <glad/glad.h>
#include <glm/gtc/noise.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

// --- VoxelChunk Constructor & Destructor ---
VoxelChunk::VoxelChunk(int size)
    : m_size(size),
      m_VAO(0), m_VBO(0), m_EBO(0), m_indexCount(0),
      m_needsMeshUpdate(false),
      m_biomeManager(nullptr),
      m_textureAtlas(nullptr),
      m_worldPosition(0, 0)
{
    // Resize voxel data
    m_voxelData.resize(size, std::vector<std::vector<int>>(size, std::vector<int>(size, 0)));
}

VoxelChunk::~VoxelChunk() {
    // Clean up OpenGL buffers
    if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);
    if (m_EBO != 0) glDeleteBuffers(1, &m_EBO);
}

// --- Public Methods Implementations ---

void VoxelChunk::render() {
    if (m_VAO == 0 || m_indexCount == 0) {
        return; // Nothing to render
    }

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void VoxelChunk::setupMesh() {
    if (m_vertices.empty() || m_indices.empty()) {
        // If mesh becomes empty after modification, clean up buffers
        if (m_VAO != 0) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
        if (m_VBO != 0) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
        if (m_EBO != 0) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
        m_indexCount = 0;
        std::cout << "WARNING: Empty mesh, nothing to setup!" << std::endl;
        return; // Nothing to setup
    }

    // Generate buffers if they don't exist, otherwise reuse them
    if (m_VAO == 0) glGenVertexArrays(1, &m_VAO);
    if (m_VBO == 0) glGenBuffers(1, &m_VBO);
    if (m_EBO == 0) glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // Use GL_DYNAMIC_DRAW because chunk meshes can be updated frequently
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    // Use unsigned int for indices size
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_DYNAMIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // Vertex Texture Coords
    glEnableVertexAttribArray(2); // Assuming location 2 for tex coords in shader
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0); // Unbind VAO
    
    // Debug information
    std::cout << "Mesh setup complete: VAO=" << m_VAO << ", VBO=" << m_VBO 
              << ", EBO=" << m_EBO << ", Vertices=" << m_vertices.size()/8 
              << ", Indices=" << m_indexCount << std::endl;
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL Error in setupMesh: " << error << std::endl;
    }
}

int VoxelChunk::getVoxel(int x, int y, int z) const {
    // Basic bounds check
    if (x < 0 || x >= m_size || y < 0 || y >= m_size || z < 0 || z >= m_size) {
        return 0; // Treat out-of-bounds as air
    }
    return m_voxelData[x][y][z];
}

void VoxelChunk::setVoxel(int x, int y, int z, int type) {
    if (x >= 0 && x < m_size && y >= 0 && y < m_size && z >= 0 && z < m_size) {
        if (m_voxelData[x][y][z] != type) { // Only update if different
            m_voxelData[x][y][z] = type;
            markForMeshUpdate();
        }
    }
}

bool VoxelChunk::needsMeshUpdate() const {
    return m_needsMeshUpdate;
}

void VoxelChunk::markForMeshUpdate(bool update) {
    m_needsMeshUpdate = update;
}

void VoxelChunk::clearMeshUpdateFlag() {
    m_needsMeshUpdate = false;
}

void VoxelChunk::setBiomeManager(const BiomeManager* biomeManager) {
    this->m_biomeManager = biomeManager;
}

void VoxelChunk::setWorldPosition(const glm::ivec2& position) {
    m_worldPosition = position;
}

void VoxelChunk::setTextureAtlas(const TextureAtlas* atlas) {
    this->m_textureAtlas = atlas;
}

const glm::ivec2& VoxelChunk::getWorldPosition() const {
    return m_worldPosition;
}

// --- Mesh Generation Implementations ---

void VoxelChunk::generateMesh() {
    // Clear existing mesh data
    m_vertices.clear();
    m_indices.clear();
    m_indexCount = 0; // Reset index count

    // Helper lambda to check if a voxel coordinate is valid and is air/transparent
    auto is_air_or_outside = [&](int x, int y, int z) {
        if (x < 0 || x >= m_size || y < 0 || y >= m_size || z < 0 || z >= m_size) {
            return true; // Outside bounds is considered air for face generation
        }
        // TODO: Add check for transparent block types if needed
        return m_voxelData[x][y][z] == 0; // 0 represents air
    };

    for (int x = 0; x < m_size; ++x) {
        for (int y = 0; y < m_size; ++y) { // Iterate y second
            for (int z = 0; z < m_size; ++z) { // Iterate z last
                if (m_voxelData[x][y][z] != 0) { // If voxel is solid
                    // Check neighbors to determine visible faces
                    if (is_air_or_outside(x - 1, y, z)) addFace(x, y, z, Face::Left);
                    if (is_air_or_outside(x + 1, y, z)) addFace(x, y, z, Face::Right);
                    if (is_air_or_outside(x, y - 1, z)) addFace(x, y, z, Face::Bottom);
                    if (is_air_or_outside(x, y + 1, z)) addFace(x, y, z, Face::Top);
                    if (is_air_or_outside(x, y, z - 1)) addFace(x, y, z, Face::Front);
                    if (is_air_or_outside(x, y, z + 1)) addFace(x, y, z, Face::Back);
                }
            }
        }
    }
    setupMesh(); // Setup buffers after generating mesh data
}

// Implementation for the generateOptimizedMesh method using greedy meshing algorithm
void VoxelChunk::generateOptimizedMesh() {
    // Clear existing mesh data
    m_vertices.clear();
    m_indices.clear();
    m_indexCount = 0;
    
    int size = m_voxelData.size();
    
    // Helper lambda to check if a voxel coordinate is valid and is air/transparent
    auto is_air_or_outside = [&](int x, int y, int z) {
        if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size) {
            return true; // Outside bounds is considered air for face generation
        }
        return m_voxelData[x][y][z] == 0; // 0 represents air
    };
    
    // Process each face direction separately
    // We'll process the 6 possible face directions: +X, -X, +Y, -Y, +Z, -Z
    // This implementation focuses on Y-faces (top and bottom) as an example
    
    // -- TOP FACES (Y+) --
    // For each column, find the topmost solid voxel
    for (int y = size - 1; y >= 0; y--) {
        // Create a 2D grid to track which faces we've already processed
        std::vector<std::vector<bool>> processed(size, std::vector<bool>(size, false));
        
        for (int x = 0; x < size; x++) {
            for (int z = 0; z < size; z++) {
                // Skip if this position has been processed or is not a top face
                if (processed[x][z] || m_voxelData[x][y][z] == 0 || !is_air_or_outside(x, y + 1, z)) {
                    continue;
                }
                
                // Get the block type at this position
                int blockType = m_voxelData[x][y][z];
                
                // Try to expand in the X direction
                int width = 1;
                while (x + width < size && 
                       !processed[x + width][z] && 
                       m_voxelData[x + width][y][z] == blockType &&
                       is_air_or_outside(x + width, y + 1, z)) {
                    width++;
                }
                
                // Try to expand in the Z direction
                int height = 1;
                bool canExpand = true;
                
                while (canExpand && z + height < size) {
                    // Check if the entire row can be expanded
                    for (int dx = 0; dx < width; dx++) {
                        if (processed[x + dx][z + height] || 
                            m_voxelData[x + dx][y][z + height] != blockType ||
                            !is_air_or_outside(x + dx, y + 1, z + height)) {
                            canExpand = false;
                            break;
                        }
                    }
                    
                    if (canExpand) {
                        height++;
                    }
                }
                
                // Mark all these positions as processed
                for (int dx = 0; dx < width; dx++) {
                    for (int dz = 0; dz < height; dz++) {
                        processed[x + dx][z + dz] = true;
                    }
                }
                
                // Add the optimized face
                addOptimizedFace(x, y, z, Face::Top, width, height, blockType);
            }
        }
    }
    
    // -- BOTTOM FACES (Y-) --
    for (int y = 0; y < size; y++) {
        std::vector<std::vector<bool>> processed(size, std::vector<bool>(size, false));
        
        for (int x = 0; x < size; x++) {
            for (int z = 0; z < size; z++) {
                // Skip if this position has been processed or is not a bottom face
                if (processed[x][z] || m_voxelData[x][y][z] == 0 || !is_air_or_outside(x, y - 1, z)) {
                    continue;
                }
                
                int blockType = m_voxelData[x][y][z];
                
                // Expand in X direction
                int width = 1;
                while (x + width < size && 
                       !processed[x + width][z] && 
                       m_voxelData[x + width][y][z] == blockType &&
                       is_air_or_outside(x + width, y - 1, z)) {
                    width++;
                }
                
                // Expand in Z direction
                int height = 1;
                bool canExpand = true;
                
                while (canExpand && z + height < size) {
                    for (int dx = 0; dx < width; dx++) {
                        if (processed[x + dx][z + height] || 
                            m_voxelData[x + dx][y][z + height] != blockType ||
                            !is_air_or_outside(x + dx, y - 1, z + height)) {
                            canExpand = false;
                            break;
                        }
                    }
                    
                    if (canExpand) {
                        height++;
                    }
                }
                
                // Mark processed
                for (int dx = 0; dx < width; dx++) {
                    for (int dz = 0; dz < height; dz++) {
                        processed[x + dx][z + dz] = true;
                    }
                }
                
                // Add face
                addOptimizedFace(x, y, z, Face::Bottom, width, height, blockType);
            }
        }
    }
    
    // -- FRONT FACES (Z-) --
    for (int z = 0; z < size; z++) {
        std::vector<std::vector<bool>> processed(size, std::vector<bool>(size, false));
        
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                if (processed[x][y] || m_voxelData[x][y][z] == 0 || !is_air_or_outside(x, y, z - 1)) {
                    continue;
                }
                
                int blockType = m_voxelData[x][y][z];
                
                // Expand in X direction
                int width = 1;
                while (x + width < size && 
                       !processed[x + width][y] && 
                       m_voxelData[x + width][y][z] == blockType &&
                       is_air_or_outside(x + width, y, z - 1)) {
                    width++;
                }
                
                // Expand in Y direction
                int height = 1;
                bool canExpand = true;
                
                while (canExpand && y + height < size) {
                    for (int dx = 0; dx < width; dx++) {
                        if (processed[x + dx][y + height] || 
                            m_voxelData[x + dx][y + height][z] != blockType ||
                            !is_air_or_outside(x + dx, y + height, z - 1)) {
                            canExpand = false;
                            break;
                        }
                    }
                    
                    if (canExpand) {
                        height++;
                    }
                }
                
                // Mark processed
                for (int dx = 0; dx < width; dx++) {
                    for (int dy = 0; dy < height; dy++) {
                        processed[x + dx][y + dy] = true;
                    }
                }
                
                addOptimizedFace(x, y, z, Face::Front, width, height, blockType);
            }
        }
    }
    
    // -- BACK FACES (Z+) --
    for (int z = size - 1; z >= 0; z--) {
        std::vector<std::vector<bool>> processed(size, std::vector<bool>(size, false));
        
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                if (processed[x][y] || m_voxelData[x][y][z] == 0 || !is_air_or_outside(x, y, z + 1)) {
                    continue;
                }
                
                int blockType = m_voxelData[x][y][z];
                
                // Expand in X direction
                int width = 1;
                while (x + width < size && 
                       !processed[x + width][y] && 
                       m_voxelData[x + width][y][z] == blockType &&
                       is_air_or_outside(x + width, y, z + 1)) {
                    width++;
                }
                
                // Expand in Y direction
                int height = 1;
                bool canExpand = true;
                
                while (canExpand && y + height < size) {
                    for (int dx = 0; dx < width; dx++) {
                        if (processed[x + dx][y + height] || 
                            m_voxelData[x + dx][y + height][z] != blockType ||
                            !is_air_or_outside(x + dx, y + height, z + 1)) {
                            canExpand = false;
                            break;
                        }
                    }
                    
                    if (canExpand) {
                        height++;
                    }
                }
                
                // Mark processed
                for (int dx = 0; dx < width; dx++) {
                    for (int dy = 0; dy < height; dy++) {
                        processed[x + dx][y + dy] = true;
                    }
                }
                
                addOptimizedFace(x, y, z, Face::Back, width, height, blockType);
            }
        }
    }
    
    // -- RIGHT FACES (X+) --
    for (int x = size - 1; x >= 0; x--) {
        std::vector<std::vector<bool>> processed(size, std::vector<bool>(size, false));
        
        for (int y = 0; y < size; y++) {
            for (int z = 0; z < size; z++) {
                if (processed[y][z] || m_voxelData[x][y][z] == 0 || !is_air_or_outside(x + 1, y, z)) {
                    continue;
                }
                
                int blockType = m_voxelData[x][y][z];
                
                // Expand in Y direction
                int width = 1;
                while (y + width < size && 
                       !processed[y + width][z] && 
                       m_voxelData[x][y + width][z] == blockType &&
                       is_air_or_outside(x + 1, y + width, z)) {
                    width++;
                }
                
                // Expand in Z direction
                int height = 1;
                bool canExpand = true;
                
                while (canExpand && z + height < size) {
                    for (int dy = 0; dy < width; dy++) {
                        if (processed[y + dy][z + height] || 
                            m_voxelData[x][y + dy][z + height] != blockType ||
                            !is_air_or_outside(x + 1, y + dy, z + height)) {
                            canExpand = false;
                            break;
                        }
                    }
                    
                    if (canExpand) {
                        height++;
                    }
                }
                
                // Mark processed
                for (int dy = 0; dy < width; dy++) {
                    for (int dz = 0; dz < height; dz++) {
                        processed[y + dy][z + dz] = true;
                    }
                }
                
                addOptimizedFace(x, y, z, Face::Right, width, height, blockType);
            }
        }
    }
    
    // -- LEFT FACES (X-) --
    for (int x = 0; x < size; x++) {
        std::vector<std::vector<bool>> processed(size, std::vector<bool>(size, false));
        
        for (int y = 0; y < size; y++) {
            for (int z = 0; z < size; z++) {
                if (processed[y][z] || m_voxelData[x][y][z] == 0 || !is_air_or_outside(x - 1, y, z)) {
                    continue;
                }
                
                int blockType = m_voxelData[x][y][z];
                
                // Expand in Y direction
                int width = 1;
                while (y + width < size && 
                       !processed[y + width][z] && 
                       m_voxelData[x][y + width][z] == blockType &&
                       is_air_or_outside(x - 1, y + width, z)) {
                    width++;
                }
                
                // Expand in Z direction
                int height = 1;
                bool canExpand = true;
                
                while (canExpand && z + height < size) {
                    for (int dy = 0; dy < width; dy++) {
                        if (processed[y + dy][z + height] || 
                            m_voxelData[x][y + dy][z + height] != blockType ||
                            !is_air_or_outside(x - 1, y + dy, z + height)) {
                            canExpand = false;
                            break;
                        }
                    }
                    
                    if (canExpand) {
                        height++;
                    }
                }
                
                // Mark processed
                for (int dy = 0; dy < width; dy++) {
                    for (int dz = 0; dz < height; dz++) {
                        processed[y + dy][z + dz] = true;
                    }
                }
                
                addOptimizedFace(x, y, z, Face::Left, width, height, blockType);
            }
        }
    }
    
    // If no vertices were generated, generate at least one face to avoid rendering issues
    if (m_vertices.empty()) {
        // Add a dummy face that won't be visible (at position 0,0,0 facing upwards)
        addOptimizedFace(0, 0, 0, Face::Top, 1, 1, 0);
    }
}

// Correct implementation for addOptimizedFace
void VoxelChunk::addOptimizedFace(int x, int y, int z, Face face, int width, int height, int voxelType) {
    // Define the base vertices for a unit cube centered at origin
    static const float baseVertices[] = {
        // Positions            // Normals              // TexCoords
        -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     0.0f, 0.0f,  // Front Face 0
         0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     1.0f, 0.0f,  // 1
         0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     1.0f, 1.0f,  // 2
        -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     0.0f, 1.0f,  // 3
        
        -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,     0.0f, 0.0f,  // Back Face 4
         0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,     1.0f, 0.0f,  // 5
         0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,     1.0f, 1.0f,  // 6
        -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,     0.0f, 1.0f,  // 7
        
        -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,     1.0f, 0.0f,  // Left Face 8 (-X)
        -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,     1.0f, 1.0f,  // 9
        -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,     0.0f, 1.0f,  // 10
        -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,     0.0f, 0.0f,  // 11
        
         0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,     1.0f, 0.0f,  // Right Face 12 (+X)
         0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,     1.0f, 1.0f,  // 13
         0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,     0.0f, 1.0f,  // 14
         0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,     0.0f, 0.0f,  // 15
        
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,     0.0f, 1.0f,  // Bottom Face 16 (-Y)
         0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,     1.0f, 1.0f,  // 17
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,     1.0f, 0.0f,  // 18
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,     0.0f, 0.0f,  // 19
        
        -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,     0.0f, 1.0f,  // Top Face 20 (+Y)
         0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,     1.0f, 1.0f,  // 21
         0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,     1.0f, 0.0f,  // 22
        -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,     0.0f, 0.0f   // 23
    };

    // Get texture coordinates for this block/face
    glm::vec4 texCoordsRect(0.0f, 0.0f, 1.0f, 1.0f);
    if (this->m_textureAtlas) {
        texCoordsRect = this->m_textureAtlas->getTexCoords(voxelType, static_cast<int>(face));
    }
    float minU = texCoordsRect.x;
    float minV = texCoordsRect.y;
    float maxU = texCoordsRect.z;
    float maxV = texCoordsRect.w;

    // Determine the starting vertex index for this face
    unsigned int vertexOffset = m_vertices.size() / 8; // 8 floats per vertex (pos, norm, tex)

    // Define the four vertices of the quad based on face, width, height
    glm::vec3 v1, v2, v3, v4;
    glm::vec3 normal;
    glm::vec2 uv1, uv2, uv3, uv4;

    switch (face) {
        case Face::Front: // -Z
            v1 = glm::vec3(x + width, y, z);
            v2 = glm::vec3(x, y, z);
            v3 = glm::vec3(x, y + height, z);
            v4 = glm::vec3(x + width, y + height, z);
            normal = glm::vec3(0, 0, -1);
            uv1 = glm::vec2(minU, minV); uv2 = glm::vec2(maxU, minV); uv3 = glm::vec2(maxU, maxV); uv4 = glm::vec2(minU, maxV);
            break;
        case Face::Back: // +Z
            v1 = glm::vec3(x, y, z + 1);
            v2 = glm::vec3(x + width, y, z + 1);
            v3 = glm::vec3(x + width, y + height, z + 1);
            v4 = glm::vec3(x, y + height, z + 1);
            normal = glm::vec3(0, 0, 1);
            uv1 = glm::vec2(minU, minV); uv2 = glm::vec2(maxU, minV); uv3 = glm::vec2(maxU, maxV); uv4 = glm::vec2(minU, maxV);
            break;
        case Face::Right: // +X
            v1 = glm::vec3(x + 1, y, z);
            v2 = glm::vec3(x + 1, y, z + width);
            v3 = glm::vec3(x + 1, y + height, z + width);
            v4 = glm::vec3(x + 1, y + height, z);
            normal = glm::vec3(1, 0, 0);
            uv1 = glm::vec2(minU, minV); uv2 = glm::vec2(maxU, minV); uv3 = glm::vec2(maxU, maxV); uv4 = glm::vec2(minU, maxV);
            break;
        case Face::Left: // -X
            v1 = glm::vec3(x, y, z + width);
            v2 = glm::vec3(x, y, z);
            v3 = glm::vec3(x, y + height, z);
            v4 = glm::vec3(x, y + height, z + width);
            normal = glm::vec3(-1, 0, 0);
            uv1 = glm::vec2(minU, minV); uv2 = glm::vec2(maxU, minV); uv3 = glm::vec2(maxU, maxV); uv4 = glm::vec2(minU, maxV);
            break;
        case Face::Top: // +Y
            v1 = glm::vec3(x, y + 1, z);
            v2 = glm::vec3(x + width, y + 1, z);
            v3 = glm::vec3(x + width, y + 1, z + height);
            v4 = glm::vec3(x, y + 1, z + height);
            normal = glm::vec3(0, 1, 0);
            uv1 = glm::vec2(minU, minV); uv2 = glm::vec2(maxU, minV); uv3 = glm::vec2(maxU, maxV); uv4 = glm::vec2(minU, maxV);
            break;
        case Face::Bottom: // -Y
            v1 = glm::vec3(x, y, z + height);
            v2 = glm::vec3(x + width, y, z + height);
            v3 = glm::vec3(x + width, y, z);
            v4 = glm::vec3(x, y, z);
            normal = glm::vec3(0, -1, 0);
            uv1 = glm::vec2(minU, minV); uv2 = glm::vec2(maxU, minV); uv3 = glm::vec2(maxU, maxV); uv4 = glm::vec2(minU, maxV);
            break;
    }

    // Add vertices (Position, Normal, TexCoords)
    m_vertices.insert(m_vertices.end(), {v1.x, v1.y, v1.z, normal.x, normal.y, normal.z, uv1.x, uv1.y});
    m_vertices.insert(m_vertices.end(), {v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, uv2.x, uv2.y});
    m_vertices.insert(m_vertices.end(), {v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, uv3.x, uv3.y});
    m_vertices.insert(m_vertices.end(), {v4.x, v4.y, v4.z, normal.x, normal.y, normal.z, uv4.x, uv4.y});

    // Add indices for the two triangles forming the quad
    m_indices.push_back(vertexOffset + 0);
    m_indices.push_back(vertexOffset + 1);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset + 0);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset + 3);

    // Increment index count - this is safe within a member function
    this->m_indexCount += 6;
}


// Helper function to add a face to the mesh with texture atlas support
void VoxelChunk::addFace(int x, int y, int z, Face face) {
    // Base index for the new vertices
    unsigned int baseIndex = static_cast<unsigned int>(m_vertices.size() / 8); // 8 floats per vertex

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float fz = static_cast<float>(z);
    
    // Get the voxel type for texture coordinates
    int voxelType = m_voxelData[x][y][z];
    
    // Get texture coordinates from atlas if available
    glm::vec4 texCoordsRect(0.0f, 0.0f, 1.0f, 1.0f); // Default fallback
    if (this->m_textureAtlas) {
        int faceIndex = static_cast<int>(face);
        texCoordsRect = this->m_textureAtlas->getTexCoords(voxelType, faceIndex);
    }
    
    // Extract texture coordinates
    float minU = texCoordsRect.x;
    float minV = texCoordsRect.y;
    float maxU = texCoordsRect.z;
    float maxV = texCoordsRect.w;

    switch (face) {
        case Face::Top: // +Y (CCW from above)
            m_vertices.insert(m_vertices.end(), {fx,     fy + 1, fz,     0, 1, 0,  minU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy + 1, fz,     0, 1, 0,  maxU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy + 1, fz + 1, 0, 1, 0,  maxU, maxV});
            m_vertices.insert(m_vertices.end(), {fx,     fy + 1, fz + 1, 0, 1, 0,  minU, maxV});
            break;
        case Face::Bottom: // -Y (CCW from below)
            m_vertices.insert(m_vertices.end(), {fx,     fy,     fz,     0,-1, 0,  minU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy,     fz,     0,-1, 0,  maxU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy,     fz + 1, 0,-1, 0,  maxU, maxV});
            m_vertices.insert(m_vertices.end(), {fx,     fy,     fz + 1, 0,-1, 0,  minU, maxV});
            break;
        case Face::Right: // +X (CCW from right)
            m_vertices.insert(m_vertices.end(), {fx + 1, fy,     fz,     1, 0, 0,  minU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy + 1, fz,     1, 0, 0,  minU, maxV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy + 1, fz + 1, 1, 0, 0,  maxU, maxV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy,     fz + 1, 1, 0, 0,  maxU, minV});
            break;
        case Face::Left: // -X (CCW from left)
            m_vertices.insert(m_vertices.end(), {fx,     fy,     fz + 1, -1, 0, 0, minU, minV});
            m_vertices.insert(m_vertices.end(), {fx,     fy + 1, fz + 1, -1, 0, 0, minU, maxV});
            m_vertices.insert(m_vertices.end(), {fx,     fy + 1, fz,     -1, 0, 0, maxU, maxV});
            m_vertices.insert(m_vertices.end(), {fx,     fy,     fz,     -1, 0, 0, maxU, minV});
            break;
        case Face::Back: // +Z (CCW from back)
            m_vertices.insert(m_vertices.end(), {fx,     fy,     fz + 1, 0, 0, 1,  minU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy,     fz + 1, 0, 0, 1,  maxU, minV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy + 1, fz + 1, 0, 0, 1,  maxU, maxV});
            m_vertices.insert(m_vertices.end(), {fx,     fy + 1, fz + 1, 0, 0, 1,  minU, maxV});
            break;
        case Face::Front: // -Z (CCW from front)
            m_vertices.insert(m_vertices.end(), {fx + 1, fy,     fz,     0, 0,-1,  minU, minV});
            m_vertices.insert(m_vertices.end(), {fx,     fy,     fz,     0, 0,-1,  maxU, minV});
            m_vertices.insert(m_vertices.end(), {fx,     fy + 1, fz,     0, 0,-1,  maxU, maxV});
            m_vertices.insert(m_vertices.end(), {fx + 1, fy + 1, fz,     0, 0,-1,  minU, maxV});
            break;
    }

    // Add indices (using counter-clockwise winding order)
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);
    m_indices.push_back(baseIndex + 0);
    this->m_indexCount += 6; // Use this-> to be explicit about member access
}

// Updated generateTerrain to use the BiomeManager for biome-specific terrain
void VoxelChunk::generateTerrain() {
    if (!this->m_biomeManager) {
        std::cerr << "Warning: Generating terrain without BiomeManager" << std::endl;
        return;
    }

    FastNoiseLite baseNoise;
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    baseNoise.SetSeed(42);
    baseNoise.SetFrequency(0.02f);

    FastNoiseLite detailNoise;
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    detailNoise.SetSeed(43);
    detailNoise.SetFrequency(0.1f);

    int waterLevel = m_size / 3;
    float worldOffsetX = static_cast<float>(m_worldPosition.x * m_size);
    float worldOffsetZ = static_cast<float>(m_worldPosition.y * m_size);

    for (int x = 0; x < m_size; ++x) {
        for (int z = 0; z < m_size; ++z) {
            float worldX = worldOffsetX + static_cast<float>(x);
            float worldZ = worldOffsetZ + static_cast<float>(z);
            
            const Biome* biome = this->m_biomeManager->getBiomeAt(static_cast<int>(worldX), static_cast<int>(worldZ));
            if (!biome) continue; // Skip if biome is null
            
            float baseHeight = biome->getBaseHeight() * m_size;
            float heightVariation = biome->getHeightVariation() * m_size;
            
            float baseNoiseValue = baseNoise.GetNoise(worldX, worldZ);
            float detailNoiseValue = detailNoise.GetNoise(worldX, worldZ) * 0.2f;
            
            float normalizedNoise = (baseNoiseValue + detailNoiseValue + 1.0f) * 0.5f;
            int height = static_cast<int>(baseHeight + normalizedNoise * heightVariation);
            height = std::min(std::max(height, 1), m_size - 1);

            if (biome->getType() == Biome::Ocean) {
                height = std::min(height, waterLevel - 3);
            }
            
            for (int y = 0; y < m_size; ++y) {
                if (y < height) {
                    m_voxelData[x][y][z] = biome->getVoxelType(y, height, m_size);
                } else if (y <= waterLevel && biome->getType() != Biome::Desert) {
                    m_voxelData[x][y][z] = 3; // Water voxel type
                } else {
                    m_voxelData[x][y][z] = 0; // Air
                }
            }
        }
    }
    markForMeshUpdate();
}

// This implementation has been removed because it's a duplicate of the function
// defined earlier in the file (around line 565)