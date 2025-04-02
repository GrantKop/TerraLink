#ifndef NOISE_H
#define NOISE_H

#include <cmath>
#include <iostream>
#include "noise/simplexnoise1234.h"

class Noise {
public:
    static float getHeight(float x, float z, int seed, int octaves, float persistence, float lacunarity, float frequency, float amplitude);

};

#endif
