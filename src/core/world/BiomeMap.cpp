#include "core/world/BiomeMap.h"
#include <algorithm>
#include <cmath>

namespace BiomeMap {

    static FastNoiseLite cellWarp;
    static FastNoiseLite biomeSeedNoise;
    static FastNoiseLite biomeRegionNoise;
    static const int CELL_SIZE = 128;

    void initialize(int seed) {
        cellWarp.SetSeed(seed);
        cellWarp.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        cellWarp.SetFrequency(0.0003f);

        biomeSeedNoise.SetSeed(seed * 3);
        biomeSeedNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        biomeSeedNoise.SetFrequency(0.05f);

        biomeRegionNoise.SetSeed(seed * 42);
        biomeRegionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        biomeRegionNoise.SetFrequency(0.1f);
    }

    std::vector<BiomeSample> getBlendedBiomesWithCells(int x, int z, int max) {
        glm::vec2 pos(x, z);
        float dx = cellWarp.GetNoise(x + 453.2f, z + 981.4f);
        float dz = cellWarp.GetNoise(x - 122.7f, z - 775.9f);
        glm::vec2 warp(dx, dz);
        warp *= glm::clamp(CELL_SIZE / 8.0f, 16.0f, 32.0f);
        pos += warp;

        glm::ivec2 baseCell = glm::floor(pos / float(CELL_SIZE));
        std::vector<BiomeSample> nearby;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                glm::ivec2 neighbor = baseCell + glm::ivec2(dx, dz);
                glm::vec2 neighborCenter = (glm::vec2(neighbor) + 0.5f) * float(CELL_SIZE);
                float dist = glm::distance(pos, neighborCenter);
                float sigma = CELL_SIZE * 0.6f;
                float score = std::exp(-dist * dist / (2.0f * sigma * sigma));

                float regionVal = biomeRegionNoise.GetNoise(float(neighbor.x) * 0.4f, float(neighbor.y) * 0.4f);
                regionVal = (regionVal + 1.0f) * 0.5f;

                float jitter = biomeSeedNoise.GetNoise(float(neighbor.x), float(neighbor.y)) * 0.45f;
                regionVal = glm::clamp(regionVal + jitter, 0.0f, 1.0f);

                int id = int(regionVal * BiomeRegistry::NUM_BIOMES) % BiomeRegistry::NUM_BIOMES;

                nearby.push_back({ id, score, neighbor, dist });
            }
        }

        std::sort(nearby.begin(), nearby.end(), [](auto& a, auto& b) {
            return a.weight > b.weight;
        });

        float total = 0.0f;
        int count = std::min(max, (int)nearby.size());
        for (int i = 0; i < count; ++i) total += nearby[i].weight;
        for (int i = 0; i < count; ++i) nearby[i].weight /= total;

        return std::vector<BiomeSample>(nearby.begin(), nearby.begin() + count);
    }

    std::vector<std::pair<int, float>> getBlendedBiomes(int x, int z, int max) {
        auto full = getBlendedBiomesWithCells(x, z, max);
        std::vector<std::pair<int, float>> result;
        for (auto& s : full) result.emplace_back(s.id, s.weight);
        return result;
    }

    bool BiomeMap::isMountainNeighbor(const glm::ivec2& cell) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dz == 0) continue;
                glm::ivec2 neighbor = cell + glm::ivec2(dx, dz);
            
                float regionVal = biomeRegionNoise.GetNoise(float(neighbor.x) * 0.4f, float(neighbor.y) * 0.4f);
                regionVal = (regionVal + 1.0f) * 0.5f;
                float jitter = biomeSeedNoise.GetNoise(float(neighbor.x), float(neighbor.y)) * 0.45f;
                regionVal = glm::clamp(regionVal + jitter, 0.0f, 1.0f);
            
                int neighborID = int(regionVal * BiomeRegistry::NUM_BIOMES) % BiomeRegistry::NUM_BIOMES;
                if (neighborID == _MOUNTAINS) return true;
            }
        }
        return false;
    }

}
