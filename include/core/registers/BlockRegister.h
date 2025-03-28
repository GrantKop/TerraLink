#ifndef BLOCK_REGISTER_H
#define BLOCK_REGISTER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
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

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    std::vector<std::string> textures;
    std::string model;

    std::vector<std::string> states;

    Block(std::string name, int ID, bool solid = false, bool transparent = true, bool air = true, BLOCKTYPE type = AIR)
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
    std::unordered_map<std::string, int> nameToIndexMap;

    void registerBlock(std::string name, std::vector<std::string> states, std::vector<std::string> textures, std::string model,
                       bool solid = true, bool transparent = false, bool air = false, BLOCKTYPE type = AIR);
    void parseBlockMapJson(std::string contents, std::string fileName);
    void loadBlocks();

    void saveBlockRegistryJson();
    void parseBlockRegistryJson();

    void linkModelToBlock(Block& block);
    void link_block_full(Block& block);
};

#endif
