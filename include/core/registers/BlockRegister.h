#ifndef BLOCK_REGISTER_H
#define BLOCK_REGISTER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "graphics/Vertex.h"

#if defined(_WIN32)
#include <filesystem>
#else
#include <dirent.h>
#endif

#define ENUM_ENTRY(name) {#name, BLOCKTYPE::name}

enum BLOCKTYPE {
    AIR,
    DIRT,
    GRASS,
    STONE,
    WOOD,
    LEAVES,
    SAND
};

std::unordered_map<std::string, BLOCKTYPE> createBlockTypeMap();

struct Block {
    std::string name;
    int ID;

    BLOCKTYPE type;

    bool isSolid;
    bool isTransparent;
    bool isAir;

    int atlasX;
    int atlasY;

    std::vector<std::string> textures;
    std::string model;

    std::vector<std::string> states;

    Block(std::string name, int ID, bool solid = true, bool transparent = false, bool air = false, BLOCKTYPE type = AIR)
        : name(name), ID(ID), isSolid(solid), isTransparent(transparent), isAir(air), type(type) {}
    Block() {}
};

class BlockRegister {
public:
    std::vector<Block> blocks;

    BlockRegister();
    ~BlockRegister();

    Block getBlockByName(std::string name);
    Block getBlockByIndex(int index);
    int getBlockIndex(std::string name);

private:
    std::unordered_map<std::string, BLOCKTYPE> blockTypeMap = createBlockTypeMap();

    void registerBlock(std::string name, std::vector<std::string> states, std::vector<std::string> textures, std::string model,
                       bool solid = true, bool transparent = false, bool air = false, BLOCKTYPE type = AIR);
    void parseJson(std::string contents, std::string fileName);
    void loadBlocks();

};

#endif
