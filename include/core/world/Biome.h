// Biome.h
#ifndef BIOME_H
#define BIOME_H

#include <iostream>
#include <glm/glm.hpp>

enum BiomeID {
    _PLAINS = 0,
    _FOREST = 1,
    _DESERT = 2,
    _MOUNTAINS = 3,
    _TAIGA = 4,
    _HILLS = 5,
    _SAVANNA = 6,
    _RIVER = 7
};

struct BiomeTerrain {
    float baseHeight;

    int octaves;
    float persistence;
    float lacunarity;
    float frequency;
    float amplitude;

    float noiseOffsetX;
    float noiseOffsetZ;

    constexpr BiomeTerrain(
        float baseHeight, int octaves, float persistence, float lacunarity,
        float frequency, float amplitude, float noiseOffsetX, float noiseOffsetZ
    ) : baseHeight(baseHeight), octaves(octaves), persistence(persistence), lacunarity(lacunarity), 
        frequency(frequency), amplitude(amplitude), noiseOffsetX(noiseOffsetX), noiseOffsetZ(noiseOffsetZ) {}
};

struct Biome {
    BiomeID id;
    const char* name;
    BiomeTerrain terrain;

    float temperature;
    float humidity;

    float continentalness;
    float erosion;
    float depth;
    float weirdness;
};

#endif
