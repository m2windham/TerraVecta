#include "include/Biome.h"
#include <iostream>
#include <glm/glm.hpp>
#include <stdexcept> // For std::runtime_error
#include <algorithm>
#include <cmath>

Biome::Biome(Type type, float baseHeight, float heightVariation)
    : m_type(type), m_baseHeight(baseHeight), m_heightVariation(heightVariation)
{
    initializeBiomeParameters();
}

Biome::~Biome() {}

void Biome::initializeBiomeParameters() {
    // Set biome-specific parameters based on type
    switch (Biome::m_type) {
        case Type::Plains:
            m_surfaceBlock = 1; // Grass
            m_subsurfaceBlock = 2; // Dirt
            m_bedrockBlock = 5; // Stone
            m_decorationDensity = 0.02f;
            break;
            
        case Type::Desert:
            m_surfaceBlock = 4; // Sand
            m_subsurfaceBlock = 4; // Sand
            m_bedrockBlock = 5; // Stone
            m_decorationDensity = 0.005f;
            break;
            
        case Type::Mountains:
            m_surfaceBlock = 5; // Stone
            m_subsurfaceBlock = 5; // Stone
            m_bedrockBlock = 5; // Stone
            m_decorationDensity = 0.01f;
            break;
            
        case Type::Forest:
            m_surfaceBlock = 1; // Grass
            m_subsurfaceBlock = 2; // Dirt
            m_bedrockBlock = 5; // Stone
            m_decorationDensity = 0.08f; // Higher for more trees
            break;
            
        case Type::Tundra:
            m_surfaceBlock = 6; // Snow
            m_subsurfaceBlock = 2; // Dirt
            m_bedrockBlock = 5; // Stone
            m_decorationDensity = 0.005f;
            break;
            
        default:
            m_surfaceBlock = 1; // Default to grass
            m_subsurfaceBlock = 2; // Default to dirt
            m_bedrockBlock = 5; // Default to stone
            m_decorationDensity = 0.01f;
            break;
    }
}

Biome::Type Biome::getType() const {
    return m_type;
}

float Biome::getBaseHeight() const {
    return m_baseHeight;
}

float Biome::getHeightVariation() const {
    return m_heightVariation;
}

int Biome::getVoxelType(int x, int y, int height) const {
    if (y < 1) {
        return m_bedrockBlock; // Bedrock at the very bottom
    }
    else if (y < height - 3) {
        return m_bedrockBlock; // Stone below surface
    }
    else if (y < height - 1) {
        return m_subsurfaceBlock; // Dirt/sand layer
    }
    else if (y == height - 1) {
        return m_surfaceBlock; // Surface block (grass, sand, etc)
    }
    else if (y < 10 && m_type != Type::Desert && m_type != Type::Tundra) {
        return 3; // Water in low areas (except desert/tundra)
    }
    
    return 0; // Air above surface
}

std::string Biome::getTypeName() const {
    switch (m_type) {
        case Type::Plains:   return "Plains";
        case Type::Desert:   return "Desert";
        case Type::Mountains: return "Mountains";
        case Type::Forest:   return "Forest";
        case Type::Tundra:   return "Tundra";
        default:             return "Unknown";
    }
}