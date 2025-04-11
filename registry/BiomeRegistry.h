// BiomeRegistry.h
#ifndef BIOME_REGISTRY_H
#define BIOME_REGISTRY_H

#include "core/world/Biome.h"

namespace BiomeRegistry {

    constexpr int NUM_BIOMES = 7;

    constexpr Biome PLAINS = {
        _PLAINS, "Plains",
        {64.0f, 2, 0.5f, 2.0f, 0.01f, 4.0f, 5.0f, -5.0f},
        0.5f, 0.5f,
        0.5f, 0.5f,
        0.5f, 0.5f
    };
    
    constexpr Biome FOREST = {
        _FOREST, "Forest",
        {64.0f, 1, 0.5f, 2.0f, 0.01f, 6.0f, -56.0f, 57.0f},
        0.6f, 0.4f, // <- shifted slightly to distinguish from plains
        0.5f, 0.4f,
        0.3f, 0.4f
    };
    
    constexpr Biome DESERT = {
        _DESERT, "Desert",
        {64.0f, 2, 0.5f, 2.0f, 0.01f, 4.0f, 220.0f, 100.0f},
        0.9f, 0.1f,
        0.6f, 0.2f,
        0.3f, 0.3f
    };
    
    constexpr Biome MOUNTAINS = {
        _MOUNTAINS, "Mountains",
        {64.0f, 2, 0.5f, 3.0f, 0.01f, 48.0f, 697.0f, -147.0f},
        0.2f, 0.3f,  // <- bigger variance makes it harder to match
        0.95f, 0.3f,
        0.9f, 0.4f
    };
    
    constexpr Biome RIVER = {
        _RIVER, "River",
        {62.0f, 2, 0.5f, 2.0f, 0.01f, 3.0f, 0.5f, 0.5f},
        0.6f, 0.8f,
        0.3f, 0.1f,
        0.1f, 0.2f
    };
    
    constexpr Biome HILLS = {
        _HILLS, "Hills",
        {64.0f, 2, 0.5f, 2.0f, 0.01f, 14.0f, -31.0f, 221.0f},
        0.6f, 0.3f,
        0.6f, 0.3f,
        0.6f, 0.3f
    };
    
    constexpr Biome SAVANNA = {
        _SAVANNA, "Savanna",
        {64.0f, 2, 0.5f, 2.0f, 0.01f, 4.0f, 5.0f, -5.0f},
        0.8f, 0.2f,
        0.4f, 0.2f,
        0.3f, 0.2f
    };
    
    constexpr Biome TAIGA = {
        _TAIGA, "Taiga",
        {64.0f, 1, 0.5f, 2.0f, 0.01f, 6.0f, -56.0f, 57.0f},
        0.3f, 0.2f, // <- narrowed range so it's harder to match
        0.5f, 0.2f,
        0.3f, 0.2f
    };
           

    constexpr Biome BIOME_LIST[NUM_BIOMES] = {
        PLAINS,
        FOREST,
        DESERT,
        MOUNTAINS,
        HILLS,
        SAVANNA,
        TAIGA
    };
}

#endif
