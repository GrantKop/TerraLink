#ifndef BIOME_NOISE_H
#define BIOME_NOISE_H

#include "noise/FastNoiseLite.h"
#include "../registry/BiomeRegistry.h"

namespace BiomeNoise {
    extern FastNoiseLite noiseGenerator;

    void initializeNoiseGenerator();

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