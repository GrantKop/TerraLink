#ifndef BIOME_NOISE_H
#define BIOME_NOISE_H

#include "noise/FastNoiseLite.h"
#include "../registry/BiomeRegistry.h"

namespace BiomeNoise {
    extern FastNoiseLite noiseGenerator;

    inline void initializeNoiseGenerator() {
        noiseGenerator.SetSeed(13372323);
        noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noiseGenerator.SetFrequency(0.01f);
        noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
        noiseGenerator.SetFractalOctaves(3);
        noiseGenerator.SetFractalLacunarity(2.17f);
        noiseGenerator.SetFractalGain(0.62f);

        // noiseGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_BasicGrid);
        // noiseGenerator.SetDomainWarpAmp(2.5f);
    }

    float generateHills(int x, int z);

    float generateMountains(int x, int z);

    float generatePlains(int x, int z);

    inline int getBiomeID(int x, int z) {
        float biomeNoise = noiseGenerator.GetNoise(x * 0.002f, z * 0.002f);
        biomeNoise = (biomeNoise + 1.0f) * 0.5f;

        // std::cout << "Biome Noise: " << biomeNoise << std::endl;
    
        if (biomeNoise < 0.15f) return _OCEAN;
        else if (biomeNoise < 0.3f) return _RIVER;
        else if (biomeNoise < 0.45f) return _DESERT;
        else if (biomeNoise < 0.6f) return _FOREST;
        else if (biomeNoise < 0.75f) return _HILLS;
        else return _MOUNTAINS;
    }
    
    inline float getRiverMask(int x, int z) {
        float riverNoise = noiseGenerator.GetNoise(x * 0.004f, z * 0.004f);
        return 1.0f - std::abs(riverNoise);
    }

}

#endif