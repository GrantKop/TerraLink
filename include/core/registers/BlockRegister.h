#ifndef BLOCK_REGISTER_H
#define BLOCK_REGISTER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <nlohmann/json.hpp>

#include "graphics/Vertex.h"
#include "core/world/Block.h"

#if defined(_WIN32)
#include <filesystem>
#else
#include <dirent.h>
#endif

#define ENUM_ENTRY(name) {#name, BLOCKTYPE::name}

std::unordered_map<std::string, BLOCKTYPE> createBlockTypeMap();

class BlockRegister {
public:
    std::vector<Block> blocks;

    static void setInstance(BlockRegister* instance);
    static BlockRegister& instance();

    BlockRegister();
    ~BlockRegister();

    const Block getBlockByName(std::string name);
    const Block getBlockByIndex(int index);
    int getBlockIndex(std::string name);

private:
    std::unordered_map<std::string, BLOCKTYPE> blockTypeMap = createBlockTypeMap();
    std::unordered_map<std::string, int> nameToIndexMap;

    void registerBlock(std::string name, std::vector<std::string> states, std::vector<std::string> textures, std::string model,
                       bool solid = true, bool transparent = true, bool air = false, BLOCKTYPE type = AIR);
    void parseBlockMapJson(std::string contents, std::string fileName);
    void loadBlocks();

    void saveBlockRegistryJson();
    void parseBlockRegistryJson();

    void linkModelToBlock(Block& block);
    void link_block_full(Block& block);
    void link_covered_cross(Block& block);
    void link_cross(Block& block);
    void link_ore_block(Block& block);

    static BlockRegister* s_instance;
};

#endif
