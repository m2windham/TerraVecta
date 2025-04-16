#pragma once

#include <vector>
#include <string> // Include string for texture path
#include <map>    // Include map for block texture map
#include <glad/glad.h> // Include glad for GLuint
#include <glm/glm.hpp>

class TextureAtlas {
public:
    TextureAtlas(); // Default constructor
    ~TextureAtlas();

    bool initialize(const std::string& atlasImagePath, int tileSize);

    // Method used in VoxelChunk.cpp - should return vec4(minU, minV, maxU, maxV)
    glm::vec4 getTexCoords(int blockType, int faceIndex) const;
    
    void registerBlock(int blockType, const std::vector<int>& tileIndices); // Declaration needed

    void bind(unsigned int textureUnit = 0) const;
    unsigned int getTextureID() const;

private:
    unsigned int m_textureID = 0;
    int m_tileSize = 16;
    int m_atlasWidth = 0;
    int m_atlasHeight = 0;
    std::map<int, std::vector<int>> m_blockTiles; // Store tile indices for each block type
};