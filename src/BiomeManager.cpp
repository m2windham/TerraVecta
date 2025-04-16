#include "include/BiomeManager.h"
#include <random>
#include <cmath>
#include <iostream>

BiomeManager::BiomeManager()
    : m_seed(12345) // Default seed
{
    // Initialize noise generator
    m_noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noise.SetSeed(m_seed);
    m_noise.SetFrequency(0.01f);
    
    // Initialize biome types
    initializeBiomes();
}

BiomeManager::~BiomeManager() {
    // Clean up biomes
    for (auto& biome : m_biomes) {
        delete biome;
    }
    m_biomes.clear();
    m_biomeMap.clear();
}

void BiomeManager::initializeBiomes() {
    // Create different biome types
    m_biomes.push_back(new Biome(Biome::Type::Plains, 10.0f, 5.0f));
    m_biomes.push_back(new Biome(Biome::Type::Desert, 8.0f, 3.0f));
    m_biomes.push_back(new Biome(Biome::Type::Mountains, 20.0f, 15.0f));
    m_biomes.push_back(new Biome(Biome::Type::Forest, 12.0f, 7.0f));
    m_biomes.push_back(new Biome(Biome::Type::Tundra, 9.0f, 4.0f));
    
    std::cout << "Initialized " << m_biomes.size() << " biome types" << std::endl;
}

void BiomeManager::setSeed(int seed) {
    m_seed = seed;
    m_noise.SetSeed(seed);
}

Biome* BiomeManager::getBiomeAt(int x, int z) const {
    // Check if the biome is already cached
    glm::ivec2 pos(x, z);
    auto it = m_biomeMap.find(pos);
    if (it != m_biomeMap.end()) {
        return m_biomes[it->second];
    }
    
    // Determine biome type based on noise
    float moistureNoise = m_noise.GetNoise(x * 0.5f, z * 0.5f);
    float temperatureNoise = m_noise.GetNoise(z * 0.5f, x * 0.5f);
    float elevationNoise = m_noise.GetNoise(x * 0.25f, z * 0.25f);
    
    // Map noise values to biome types
    int biomeIndex = 0; // Default to plains
    
    if (elevationNoise > 0.5f) {
        biomeIndex = 2; // Mountains
    } else if (temperatureNoise > 0.3f && moistureNoise < -0.3f) {
        biomeIndex = 1; // Desert
    } else if (temperatureNoise < -0.3f) {
        biomeIndex = 4; // Tundra
    } else if (moistureNoise > 0.2f) {
        biomeIndex = 3; // Forest
    } else {
        biomeIndex = 0; // Plains
    }
    
    // Cache the result
    m_biomeMap[pos] = biomeIndex;
    
    return m_biomes[biomeIndex];
}