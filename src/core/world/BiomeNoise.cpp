#include "core/world/BiomeNoise.h"
#include "core/game/Game.h"
#include "core/world/World.h"
#include "core/world/BiomeMap.h"

namespace BiomeNoise {

    thread_local FastNoiseLite noiseGenerator;
    thread_local FastNoiseLite tempNoise;
    thread_local FastNoiseLite humidNoise;

    void ensureInitialized() {
        static thread_local bool initialized = false;
        if (!initialized) {
            initializeNoiseGenerator();
            initialized = true;
        }
    }
    
    void initializeNoiseGenerator() {
        int seed = Game::instance().getWorld().getSeed();

        noiseGenerator.SetSeed(seed);
        noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noiseGenerator.SetFrequency(0.01f);
        noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
        noiseGenerator.SetFractalOctaves(3);
        noiseGenerator.SetFractalLacunarity(2.17f);
        noiseGenerator.SetFractalGain(0.62f);

        tempNoise.SetSeed(seed * 3 + 123);
        tempNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        tempNoise.SetFrequency(0.45f);
        tempNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        tempNoise.SetFractalOctaves(4);
        tempNoise.SetFractalLacunarity(2.0f);
        tempNoise.SetFractalGain(0.55f);

        humidNoise.SetSeed(seed * 7 + 987);
        humidNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        humidNoise.SetFrequency(0.60f);
        humidNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        humidNoise.SetFractalOctaves(4);
        humidNoise.SetFractalLacunarity(2.0f);
        humidNoise.SetFractalGain(0.55f);
    }

    float generateHills(int x, int z) {
        ensureInitialized(); 
        float noiseVal = noiseGenerator.GetNoise((float)x, (float)z);
        noiseVal = (noiseVal + 1.0f) * 0.5f;
    
        const float baseHeight = 32.0f;
        const float heightScale = 55.0f;
        const float threshold = 0.05f;
    
        float rollingTerrain = std::pow(noiseVal, 1.3f) * 10.0f;
    
        if (noiseVal < threshold) {
            return baseHeight + rollingTerrain;
        }
    
        float remapped = (noiseVal - threshold) / (1.0f - threshold);
        float curved = std::pow(remapped, 2.5f);
    
        return baseHeight + rollingTerrain + curved * heightScale;
    }

    float generateMountains(int x, int z) {
        ensureInitialized(); 
        float baseFreq = 0.03f;
        float warpFreq = 0.008f;
    
        float nx = (float)x * baseFreq;
        float nz = (float)z * baseFreq;
    
        float warp = noiseGenerator.GetNoise(x * warpFreq, z * warpFreq) * 5.0f;
        nx += warp;
        nz += warp;
    
        float noiseVal = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float totalAmplitude = 0.0f;
    
        for (int i = 0; i < 5; ++i) {
            float n = noiseGenerator.GetNoise(nx * frequency, nz * frequency);
            n = 1.0f - fabs(n);
            n *= n;
    
            noiseVal += n * amplitude;
            totalAmplitude += amplitude;
    
            amplitude *= 0.6f;
            frequency *= 2.0f;
        }
    
        noiseVal /= totalAmplitude;
    
        float valley = noiseGenerator.GetNoise(nx * 0.5f, nz * 0.5f);
        valley = pow(valley, 3.0f);
        noiseVal *= (1.0f - valley);
    
        const float baseHeight = 32.0f;
        const float heightScale = 180.0f;
        const float threshold = 0.5f;
    
        float rollingTerrain = std::pow(noiseVal, 1.05f) * 7.5f;
    
        if (noiseVal < threshold) {
            return baseHeight + rollingTerrain;
        }
    
        float remapped = (noiseVal - threshold) / (1.0f - threshold);
        float curved = std::pow(remapped, 3.4f);
    
        return baseHeight + rollingTerrain + curved * heightScale;
    }

    float generatePlains(int x, int z) {
        ensureInitialized(); 
        float noiseVal = noiseGenerator.GetNoise((float)x, (float)z);
        noiseVal = (noiseVal + 1.0f) * 0.5f;
    
        const float baseHeight = 32.0f;
        const float heightScale = 10.0f;
        const float threshold = 0.00002f;
    
        float rollingTerrain = std::pow(noiseVal, 1.9f) * 10.0f;
    
        if (noiseVal < threshold) {
            return baseHeight + rollingTerrain;
        }
    
        float remapped = (noiseVal - threshold) / (1.0f - threshold);
        float curved = std::pow(remapped, 2.25f);
    
        return baseHeight + rollingTerrain + curved * heightScale;
    }

    float generateHeight(int biomeID, int x, int z) {
        ensureInitialized();
        switch (biomeID) {
            case _PLAINS:
                return generatePlains(x, z);
            case _HILLS:
                return generateHills(x, z);
            case _MOUNTAINS:
                return generateMountains(x, z);
            case _FOREST:
                return generateHills(x, z);
            case _DESERT: {
                float base = generatePlains(x, z);
                float wobble = std::abs(std::sin(x * 0.01f + z * 0.01f)) * 2.0f;
                return base * 0.6f + wobble;
            }
            default:
                return generatePlains(x, z);
        }
    }

    float BiomeNoise::generateBlendedHeight(int x, int z) {
        ensureInitialized();
        auto fullBlends = BiomeMap::getBlendedBiomesWithCells(x, z, 4);
        float blended = 0.0f;
        float weightSum = 0.0f;

        for (auto& sample : fullBlends) {
            float h = generateHeight(sample.id, x, z);
            float trust = glm::smoothstep(0.1f, 0.7f, sample.weight);
        
            if (sample.id == _MOUNTAINS) {
                bool hasMountainNeighbor = BiomeMap::isMountainNeighbor(sample.cell);
                if (!hasMountainNeighbor) {
                    float maxDist = 75.0f;
                    float edgeFactor = glm::clamp(sample.distanceToCenter / maxDist, 0.0f, 1.0f);
                    float shapeFactor = glm::mix(1.0f, 0.15f, edgeFactor);
                    h = glm::mix(32.0f, h, shapeFactor);
                }
            }
        
            blended += h * trust;
            weightSum += trust;
        }
        float result = (weightSum > 0.001f) ? blended / weightSum : 32.0f;
        return 32.0f + (result - 32.0f) * 1.2f;
    }

    std::vector<std::pair<int, float>> BiomeNoise::getBiomeBlend(int x, int z, int maxBiomes) {
        return BiomeMap::getBlendedBiomes(x, z, maxBiomes);
    }

    bool BiomeNoise::isChunkLikelyEmpty(const ChunkPosition& pos) {
        ensureInitialized();

        int x0 = pos.x * CHUNK_SIZE;
        int z0 = pos.z * CHUNK_SIZE;
        int yMin = pos.y * CHUNK_SIZE;
        int yMax = yMin + CHUNK_SIZE;

        for (int x = 0; x < CHUNK_SIZE; ++x) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                int wx = x0 + x;
                int wz = z0 + z;

                float height = generateBlendedHeight(wx, wz);
                int surfaceY = static_cast<int>(height);

                if (surfaceY >= yMin - 1 && surfaceY < yMax + 1) {
                    return false;
                }
            }
        }

        return true;
    }
}
