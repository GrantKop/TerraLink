#include "noise/Noise.h"

float Noise::getHeight(float x, float z, int seed, int octaves, float persistence, float lacunarity, float frequency, float amplitude)
{
    float noiseValue = 0.0f;
    float currentAmplitude = amplitude;
    float currentFrequency = frequency;
    
    // setSeed(seed); // Uncomment and adjust if needed
    
    for (int i = 0; i < octaves; ++i) {
        noiseValue += snoise2(x * currentFrequency, z * currentFrequency) * currentAmplitude;
        currentAmplitude *= persistence;
        currentFrequency *= lacunarity;
    }
    
    float baseHeight = 64.0f; 
    return noiseValue + baseHeight;
}
