#ifndef BIOME_MAP_H
#define BIOME_MAP_H

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <utility>
#include "noise/FastNoiseLite.h"
#include "core/world/BiomeRegistry.h"

namespace BiomeMap {

    struct BiomeSample {
        int id;
        float weight;
        glm::ivec2 cell;
        float distanceToCenter;
    };

    void initialize(int seed);
    std::vector<std::pair<int, float>> getBlendedBiomes(int x, int z, int max = 2);
    std::vector<BiomeSample> getBlendedBiomesWithCells(int x, int z, int max = 2);
    bool isMountainNeighbor(const glm::ivec2& cell);
} 

#endif