// BiomeRegistry.h
#ifndef BIOME_REGISTRY_H
#define BIOME_REGISTRY_H

#include "core/world/Biome.h"

namespace BiomeRegistry {
    constexpr int NUM_BIOMES = 5;

    const Biome& getBiome(int biomeID);
    const std::array<Biome, NUM_BIOMES>& getAllBiomes();
}

#endif
