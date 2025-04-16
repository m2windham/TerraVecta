#pragma once

#include <vector>
#include <map>
#include <memory>
#include "Biome.h"
#include "FastNoiseLite.h"
#include <glm/glm.hpp>
#include "GlmOperators.h" // Include the GLM operators for comparison support

class BiomeManager {
public:
    BiomeManager();
    ~BiomeManager();

    // Get biome at world coordinates
    Biome* getBiomeAt(int x, int z) const;
    
    // Initialize different biome types
    void initializeBiomes();
    
    // Set the seed for noise generation
    void setSeed(int seed);

private:
    std::vector<Biome*> m_biomes;
    mutable std::map<glm::ivec2, int> m_biomeMap; // Cache biome lookups
    FastNoiseLite m_noise;
    int m_seed;
};