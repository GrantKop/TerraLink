#ifndef NOISE_H
#define NOISE_H

#include <cmath>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <array>
#include <stb_image.h>

#include "stb_image_write.h"
#include "noise/simplexnoise1234.h"
#include "../registry/BiomeRegistry.h"

namespace Noise {
    void initialize(uint32_t seed);
    void generateBiomeMap(int size);
    
    const Biome& getBiomeAtWorldPos(int worldX, int worldZ);
    Biome getBlendedBiome(int worldX, int worldZ);
    std::array<float, 6> getClimateAtWorldPos(int worldX, int worldZ);

    void exportBiomeMapToPNG(const std::string& filename);

    extern uint32_t globalSeed;
}

#endif