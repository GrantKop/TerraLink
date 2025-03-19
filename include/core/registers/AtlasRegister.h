#ifndef ATLAS_REGISTER_H
#define ATLAS_REGISTER_H

#include <iostream>
#include <string>
#include <vector>
#include <stb_image.h>

#include "stb_image_write.h"
#include "core/registers/BlockRegister.h"

#if defined(_WIN32)
#include <filesystem>
#else
#include <dirent.h>
#endif

struct TextureFile {
    unsigned char* data;
    int width, height, nrChannels;
    std::string name;
};

// Finds the largest square atlas size for a given number of files, in powers of 2
int findLargestAtlas(int numFiles);

int findLargestTexture(std::vector<TextureFile> textures);

std::vector<TextureFile> loadTextures(const char* path);

void createAtlas(const char* path);

// Registers an atlas with the given path
void registerAtlas();

void linkBlocksToAtlas(BlockRegister* blockRegister);


#endif
