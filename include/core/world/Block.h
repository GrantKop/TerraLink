#ifndef BLOCK_H
#define BLOCK_H

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "graphics/Vertex.h"

// Enum for block types
enum BLOCKTYPE {
    AIR,
    DIRT,
    GRASS,
    STONE,
    WOOD,
    LEAVES,
    SAND
};

inline const char* blockTypeToString(BLOCKTYPE type) {
    switch (type) {
        case AIR:    return "air";
        case DIRT:   return "dirt";
        case GRASS:  return "grass";
        case STONE:  return "stone";
        case WOOD:   return "wood";
        case LEAVES: return "leaves";
        case SAND:   return "sand";
        default:     return "stone";
    }
}

// Enum for block faces
enum Face {
    BACK = 0,
    BOTTOM = 1,
    FRONT = 2,
    LEFT = 3,
    RIGHT = 4,
    TOP = 5
};

// Struct for holding block data in a chunk
struct BlockData {
    uint16_t id;
    uint8_t state;
};

// Offsets for each face in 3D space
const glm::ivec3 FACE_OFFSETS[6] = {
    {0, 0, -1}, // BACK
    {0, -1, 0}, // BOTTOM
    {0, 0, 1},  // FRONT
    {-1, 0, 0}, // LEFT
    {1, 0, 0},  // RIGHT
    {0, 1, 0}   // TOP
};

// Block class representing a block in the world
struct Block {
    std::string name;
    int ID;

    BLOCKTYPE type;

    bool isSolid;
    bool isTransparent;
    bool isAir;

    std::vector<Vertex> vertices;

    std::vector<std::string> textures;
    std::string model;

    std::vector<std::string> states;

    Block(std::string name, int ID, bool solid = false, bool transparent = true, bool air = true, BLOCKTYPE type = AIR)
        : name(name), ID(ID), isSolid(solid), isTransparent(transparent), isAir(air), type(type) {}
    Block() {}
};

#endif
