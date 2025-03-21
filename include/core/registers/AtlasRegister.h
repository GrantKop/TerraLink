#ifndef ATLAS_REGISTER_H
#define ATLAS_REGISTER_H

#include <iostream>
#include <string>
#include <vector>
#include <stb_image.h>

#include "stb_image_write.h"
#include "core/registers/BlockRegister.h"
#include "graphics/Texture.h"

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

// Create a map of texture names to their corresponding X and Y positions in the atlas
// std::unordered_map<std::string, std::pair<int, int>> createTextureMap(std::vector<TextureFile> textures, int atlasSize);

class Atlas {
public:
    Atlas(const char* path);
    ~Atlas();

    int getSize();
    Texture* getAtlas();

    void linkBlocksToAtlas(BlockRegister* blockRegister);

private:

    int size;
    int numTextures;
    int largestTexture;
    int width;
    bool blocksLinked = false;
    Texture* atlas;

    std::unordered_map<std::string, std::pair<int, int>> textureMap;

    std::vector<TextureFile> loadTextures(const char* path);

    // Finds the largest square atlas size for a given number of files, in powers of 2
    int findLargestAtlas(int numFiles);

    int findLargestTexture(std::vector<TextureFile> textures);

    // block model linking
    void block_full_linking(Block& block, std::string texture, std::string textureKey, float s, float t);
};

#endif
