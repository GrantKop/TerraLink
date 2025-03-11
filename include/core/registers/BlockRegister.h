#ifndef BLOCK_REGISTER_H
#define BLOCK_REGISTER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "graphics/Vertex.h"

#if defined(_WIN32)
#include <filesystem>
#else
#include <dirent.h>
#endif

struct Block {
    std::string name;
    int ID;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    std::vector<std::string> textures;
    std::string mesh;

    std::vector<std::string> states;
};

class BlockRegister {
public:
    std::vector<Block> blocks;

    BlockRegister() {}

    Block getBlockByName(std::string name);
    Block getBlockByIndex(int index);
    int getBlockIndex(std::string name);

private:
    void registerBlock(std::string name, std::vector<std::string> states, std::vector<std::string> textures, std::string model);
    void parseJson(std::string contents);
    void loadBlocks();

};

#endif
