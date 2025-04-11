#include "noise/Noise.h"

namespace Noise {
    uint32_t globalSeed;
    static int biomeMapSize;
    static float biomeNoiseScale = 0.0008f;  // lower = larger blobs
    static float warpStrength = 250.0f;      // tweak this for smoother vs. twistier regions

    static std::vector<std::vector<const Biome*>> biomeMap;
    static std::vector<std::vector<std::array<float, 6>>> climateMap;

    float warpedNoise(float x, float z, float scale, float warpStrength, float seedOffset) {
        float wx = snoise2((x + seedOffset) * scale * 0.5f, (z + seedOffset) * scale * 0.5f);
        float wz = snoise2((x - seedOffset) * scale * 0.5f, (z - seedOffset) * scale * 0.5f);
        x += wx * warpStrength;
        z += wz * warpStrength;
        return (snoise2(x * scale, z * scale) + 1.0f) * 0.5f;
    }

    void initialize(uint32_t seed) {
        globalSeed = seed;
    }

    void generateBiomeMap(int size) {
        biomeMapSize = size;
        biomeMap.resize(size, std::vector<const Biome*>(size));
        climateMap.resize(size, std::vector<std::array<float, 6>>(size));

        float weightTemp = 1.0f;
        float weightHumid = 1.0f;
        float weightCont = 1.0f;
        float weightEros = 0.8f;
        float weightDepth = 0.6f;
        float weightWeird = 0.5f;

        for (int z = 0; z < size; ++z) {
            for (int x = 0; x < size; ++x) {
                float fx = static_cast<float>(x);
                float fz = static_cast<float>(z);

                std::array<float, 6> climate = {
                    warpedNoise(fx, fz, biomeNoiseScale, warpStrength, globalSeed + 0),
                    warpedNoise(fx, fz, biomeNoiseScale, warpStrength, globalSeed + 100),
                    warpedNoise(fx, fz, biomeNoiseScale, warpStrength, globalSeed + 200),
                    warpedNoise(fx, fz, biomeNoiseScale, warpStrength, globalSeed + 300),
                    warpedNoise(fx, fz, biomeNoiseScale, warpStrength, globalSeed + 400),
                    warpedNoise(fx, fz, biomeNoiseScale, warpStrength, globalSeed + 500)
                };

                climateMap[z][x] = climate;

                float bestScore = std::numeric_limits<float>::max();
                const Biome* bestBiome = &BiomeRegistry::PLAINS;

                for (const auto& biome : BiomeRegistry::BIOME_LIST) {
                    float dist =
                        weightTemp  * std::abs(biome.temperature     - climate[0]) +
                        weightHumid * std::abs(biome.humidity        - climate[1]) +
                        weightCont  * std::abs(biome.continentalness - climate[2]) +
                        weightEros  * std::abs(biome.erosion         - climate[3]) +
                        weightDepth * std::abs(biome.depth           - climate[4]) +
                        weightWeird * std::abs(biome.weirdness       - climate[5]);

                    if (dist < bestScore) {
                        bestScore = dist;
                        bestBiome = &biome;
                    }
                }

                biomeMap[z][x] = bestBiome;
            }
        }
    }

    const Biome& getBiomeAtWorldPos(int worldX, int worldZ) {
        int x = std::clamp(worldX, 0, biomeMapSize - 1);
        int z = std::clamp(worldZ, 0, biomeMapSize - 1);
        return *biomeMap[z][x];
    }

    std::array<float, 6> getClimateAtWorldPos(int worldX, int worldZ) {
        int x = std::clamp(worldX, 0, biomeMapSize - 1);
        int z = std::clamp(worldZ, 0, biomeMapSize - 1);
        return climateMap[z][x];
    }

    void exportBiomeMapToPNG(const std::string& filename) {
        int width = biomeMapSize;
        int height = biomeMapSize;

        std::vector<unsigned char> image(width * height * 3);

        auto biomeColor = [](BiomeID id) -> std::array<unsigned char, 3> {
            switch (id) {
                case BiomeID::_PLAINS:   return { 100, 200, 100 };
                case BiomeID::_FOREST:   return { 34, 139, 34 };
                case BiomeID::_DESERT:   return { 237, 201, 175 };
                case BiomeID::_MOUNTAINS:return { 128, 128, 128 };
                case BiomeID::_RIVER:    return { 64, 164, 223 };
                case BiomeID::_TAIGA:    return { 0, 128, 128 };
                case BiomeID::_SAVANNA:  return { 189, 183, 107 };
                case BiomeID::_HILLS:    return { 50, 180, 90 };
                default:                 return { 0, 0, 0 };
            }
        };

        for (int z = 0; z < height; ++z) {
            for (int x = 0; x < width; ++x) {
                const Biome* biome = biomeMap[z][x];
                auto color = biomeColor(biome->id);
                int idx = (z * width + x) * 3;
                image[idx + 0] = color[0];
                image[idx + 1] = color[1];
                image[idx + 2] = color[2];
            }
        }

        stbi_write_png(filename.c_str(), width, height, 3, image.data(), width * 3);
    }

    int getBiomeMapSize() {
        return biomeMapSize;
    }

    const Biome* const* getBiomeMapRow(int z) {
        return biomeMap[z].data();
    }

    Biome Noise::getBlendedBiome(int worldX, int worldZ) {
        return getBiomeAtWorldPos(worldX, worldZ);
    }
}
