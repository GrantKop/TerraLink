#ifndef BIOME_NOISE_H
#define BIOME_NOISE_H

#include "noise/FastNoiseLite.h"
#include "../registry/BiomeRegistry.h"

namespace BiomeNoise {
    extern FastNoiseLite noiseGenerator;

    inline void initializeNoiseGenerator() {
        noiseGenerator.SetSeed(13375656);
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

}

#endif