#include <array>
#include "core/world/BiomeRegistry.h"

namespace BiomeRegistry {

  static std::array<Biome, NUM_BIOMES> biomes = {{
    { _PLAINS, "Plains", { 31.0f, 4, 0.5f, 2.0f, 0.01f, 10.0f, 0.0f, 0.0f },
      0.4f, 0.9f, 0.3f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f },
    
    { _HILLS, "Hills", { 31.0f, 4, 0.5f, 2.1f, 0.02f, 18.0f, 133.7f, 42.0f },
      0.4f, 0.8f, 0.5f, 0.9f, 0.5f, 0.4f, 0.6f, 0.4f },
    
    { _MOUNTAINS, "Mountains", { 32.0f, 6, 0.6f, 2.3f, 0.02f, 40.0f, 82.1f, -73.2f },
      0.2f, 0.6f, 0.2f, 0.6f, 0.0f, 0.3f, 0.0f, 0.5f },
    
    { _FOREST, "Forest", { 31.0f, 5, 0.5f, 2.0f, 0.015f, 12.0f, -12.3f, 27.4f },
      0.4f, 0.8f, 0.6f, 1.0f, 0.4f, 0.7f, 0.7f, 1.0f },
    
    { _DESERT, "Desert", { 31.0f, 2, 0.4f, 2.0f, 0.01f, 8.0f, 99.0f, -15.0f },
      0.9f, 1.0f, 0.0f, 0.3f, 0.8f, 1.0f, 0.0f, 0.4f },
  }};

  const Biome& getBiome(int biomeID) {
    return biomes[biomeID];
  }

  const std::array<Biome, NUM_BIOMES>& getAllBiomes() {
    return biomes;
  }

}
