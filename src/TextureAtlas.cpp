#include "include/TextureAtlas.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

TextureAtlas::TextureAtlas() 
    : m_textureID(0), m_tileSize(0), m_atlasWidth(0), m_atlasHeight(0) 
{
}

TextureAtlas::~TextureAtlas() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
    }
}

bool TextureAtlas::initialize(const std::string& atlasPath, int tileSize) {
    // Load the texture atlas image
    int width, height, channels;
    unsigned char* data = stbi_load(atlasPath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture atlas: " << atlasPath << std::endl;
        return false;
    }
    
    m_tileSize = tileSize;
    m_atlasWidth = width;
    m_atlasHeight = height;
    
    // Generate texture
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Upload data
    GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Free image data
    stbi_image_free(data);
    
    std::cout << "Loaded texture atlas: " << atlasPath << " (" << width << "x" << height 
              << ", " << (width / tileSize) << "x" << (height / tileSize) << " tiles)" << std::endl;
    
    return true;
}

void TextureAtlas::registerBlock(int blockType, const std::vector<int>& tileIndices) {
    // Store the tile indices for the block type
    if (tileIndices.empty()) {
        std::cerr << "Warning: Empty tile indices for block type " << blockType << std::endl;
        return;
    }
    
    m_blockTiles[blockType] = tileIndices;
}

unsigned int TextureAtlas::getTextureID() const {
    return m_textureID;
}

glm::vec4 TextureAtlas::getTexCoords(int blockType, int face) const {
    // Get the tile indices for this block type
    auto it = m_blockTiles.find(blockType);
    if (it == m_blockTiles.end()) {
        // Return default coordinates if block type not found
        return glm::vec4(0, 0, 1, 1);
    }
    
    const auto& indices = it->second;
    // If not enough face-specific indices, use the first one
    int index = (face < indices.size()) ? indices[face] : indices[0];
    
    // Calculate the tile coordinates
    int tilesPerRow = m_atlasWidth / m_tileSize;
    int tileX = index % tilesPerRow;
    int tileY = index / tilesPerRow;
    
    // Calculate UV coordinates
    float tileWidth = static_cast<float>(m_tileSize) / m_atlasWidth;
    float tileHeight = static_cast<float>(m_tileSize) / m_atlasHeight;
    float u1 = tileX * tileWidth;
    float v1 = tileY * tileHeight;
    float u2 = u1 + tileWidth;
    float v2 = v1 + tileHeight;
    
    return glm::vec4(u1, v1, u2, v2);
}

void TextureAtlas::bind(unsigned int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}