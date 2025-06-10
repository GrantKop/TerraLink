#ifndef BIOME_NOISE_H
#define BIOME_NOISE_H

#include <vector>

#include "noise/FastNoiseLite.h"
#include "core/world/BiomeRegistry.h"

struct ChunkPosition;

namespace BiomeNoise {


    void initializeNoiseGenerator();

    float generateHills(int x, int z);

    float generateMountains(int x, int z);

    float generatePlains(int x, int z);

    float generateHeight(int biomeID, int x, int z);

    std::vector<std::pair<int, float>> getBiomeBlend(int x, int z, int maxBiomes = 2);

    float generateBlendedHeight(int x, int z);

    bool isChunkLikelyEmpty(const ChunkPosition& pos);
}

#endif