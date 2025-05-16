#include "core/world/BiomeNoise.h"
#include "core/game/Game.h"
#include "core/world/World.h"

namespace BiomeNoise {
    
    FastNoiseLite noiseGenerator;
    
    void initializeNoiseGenerator() {
        noiseGenerator.SetSeed(Game::instance().getWorld().getSeed());
        noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noiseGenerator.SetFrequency(0.01f);
        noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
        noiseGenerator.SetFractalOctaves(3);
        noiseGenerator.SetFractalLacunarity(2.17f);
        noiseGenerator.SetFractalGain(0.62f);

        // noiseGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_BasicGrid);
        // noiseGenerator.SetDomainWarpAmp(2.5f);
    }

    float generateHills(int x, int z) {
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
        // Scale coordinates for domain warping
        float baseFreq = 0.03f;
        float warpFreq = 0.008f;
    
        float nx = (float)x * baseFreq;
        float nz = (float)z * baseFreq;
    
        // === DOMAIN WARPING ===
        float warp = noiseGenerator.GetNoise(x * warpFreq, z * warpFreq) * 5.0f;
        nx += warp;
        nz += warp;
    
        // === RIGID FRACTAL NOISE ===
        float noiseVal = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float totalAmplitude = 0.0f;
    
        for (int i = 0; i < 5; ++i) {
            float n = noiseGenerator.GetNoise(nx * frequency, nz * frequency);
            n = 1.0f - fabs(n); // Rigid shape
            n *= n;             // Sharpen peaks
    
            noiseVal += n * amplitude;
            totalAmplitude += amplitude;
    
            amplitude *= 0.6f;   // Gain
            frequency *= 2.0f;   // Lacunarity
        }
    
        noiseVal /= totalAmplitude;
    
        // === VALLEY MASK ===
        float valley = noiseGenerator.GetNoise(nx * 0.5f, nz * 0.5f);
        valley = pow(valley, 3.0f); // Strong flattening in valleys
        noiseVal *= (1.0f - valley); // Carve valleys into mountain range
    
        // === HEIGHT SHAPING ===
        const float baseHeight = 32.0f;
        const float heightScale = 160.0f;
        const float threshold = 0.5f;
    
        float rollingTerrain = std::pow(noiseVal, 1.05f) * 7.5f;
    
        if (noiseVal < threshold) {
            return baseHeight + rollingTerrain;
        }
    
        float remapped = (noiseVal - threshold) / (1.0f - threshold);
        float curved = std::pow(remapped, 3.4f); // Very steep mountains
    
        return baseHeight + rollingTerrain + curved * heightScale;
    }

    float generatePlains(int x, int z) {
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
       
}
