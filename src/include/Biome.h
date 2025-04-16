#pragma once

#include <string>
#include <glm/glm.hpp>

// Biome class defines the characteristics of a specific terrain type
class Biome {
public:
    // Define the biome types as a traditional C-style enum
    enum Type {
        Plains,
        Forest,
        Desert,
        Mountains,
        Tundra,
        Ocean
    };

    // Constructor with base height and height variation
    Biome(Type type, float baseHeight, float heightVariation);
    virtual ~Biome();

    // Initialize biome-specific parameters
    void initializeBiomeParameters();

    // Getters
    Type getType() const;
    float getBaseHeight() const;
    float getHeightVariation() const;
    int getVoxelType(int x, int y, int height) const;
    std::string getTypeName() const;

private:
    Type m_type;
    float m_baseHeight;
    float m_heightVariation;
    
    // Block types for terrain generation
    int m_surfaceBlock;      // Top block (grass, sand, etc)
    int m_subsurfaceBlock;   // Below surface (dirt, etc)
    int m_bedrockBlock;      // Deep blocks (stone, etc)
    float m_decorationDensity; // Density of decorations (trees, etc)
};