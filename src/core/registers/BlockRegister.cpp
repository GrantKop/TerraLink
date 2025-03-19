#include "core/registers/BlockRegister.h"

// Function to create a mapping from string names to block type enums
std::unordered_map<std::string, BLOCKTYPE> createBlockTypeMap()  {
    return {
        ENUM_ENTRY(AIR),
        ENUM_ENTRY(DIRT),
        ENUM_ENTRY(GRASS),
        ENUM_ENTRY(STONE),
        ENUM_ENTRY(WOOD),
        ENUM_ENTRY(LEAVES),
        ENUM_ENTRY(SAND)
    };
};

// Retrieves a block by its name from the registered blocks
Block BlockRegister::getBlockByName(std::string name) {
    for (Block block : blocks) {
        if (block.name == name) {
            return block;
        }
    }
    std::cerr << "Block not found: " << name << std::endl;
    return Block();
}

// Retrieves a block by its index from the registered blocks
Block BlockRegister::getBlockByIndex(int index) {
    if (index < blocks.size()) {
        return blocks[index];
    }
    std::cerr << "Block index out of range: " << index << std::endl;
    return Block();
}

// Returns the index of a block by its name, or -1 if not found
int BlockRegister::getBlockIndex(std::string name) {
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].name == name) {
            return i;
        }
    }
    std::cerr << "Block not found: " << name << std::endl;
    return -1;
}

// Registers a new block with the specified properties
void BlockRegister::registerBlock(
        std::string name, std::vector<std::string> states, std::vector<std::string> textures, std::string model,
        bool solid, bool transparent, bool air, BLOCKTYPE type) {

    Block block(name, blocks.size(), solid, transparent, air, type);

    // Assign additional properties to the block
    block.states = states;
    block.textures = textures;
    block.model = model;

    // Set the block ID to the current size of the blocks vector
    block.ID = blocks.size();
    // Add the new block to the vector of registered blocks
    blocks.push_back(block);
}

// Parses a JSON file to create and register a block
void BlockRegister::parseJson(std::string contents, std::string fileName) {

    nlohmann::json blockJson = nlohmann::json::parse(contents);

    // Extract the block name from the file name
    std::string name = fileName.substr(0, fileName.find_last_of("."));

    // Determine the block type from the JSON data
    BLOCKTYPE type = blockTypeMap[blockJson["block_type"].get<std::string>()];

    // Extract physical properties from the JSON data
    bool solid = blockJson["solid"];
    bool transparent = blockJson["transparent"];

    // Extract texture information from the JSON data
    std::vector<std::string> textures;
    for (auto& [key, value] : blockJson["textures"].items()) {
        std::string texture = key + ":" + value.get<std::string>();
        textures.push_back(texture);
    }

    // Extract state information from the JSON data
    std::vector<std::string> states;
    for (auto& [key, value] : blockJson["states"].items()) {
        std::string state = key + ":" + value.get<std::string>();
        states.push_back(state);
    }

    // Register the block with all extracted properties
    registerBlock(name, states, textures, blockJson["mesh"].get<std::string>(), solid, transparent, false, type);
}

// Loads blocks from JSON files in a specified directory
void BlockRegister::loadBlocks() {

    // Register a default air block
    registerBlock("air", {}, {}, "air", false, false, true, AIR);

    // Platform-specific directory handling
    #if defined(_WIN32)
    if (!std::filesystem::exists("../../assets/models/blocks/")) {
        std::cerr << "Error locating folder: ../../assets/models/blocks/" << std::endl;
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator("../../assets/models/blocks/")) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream blockFile(entry.path().string());
            if (!blockFile.is_open()) {
                std::cerr << "Failed to open block file: " << entry.path().string() << std::endl;
                continue;
            }
            std::stringstream buffer;
            buffer << blockFile.rdbuf();
            std::string contents = buffer.str();
            blockFile.close();
            parseJson(contents, entry.path().filename().string());
        }
    }
    #else
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir("gameFiles/register/blocks")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                std::string filename = ent->d_name;
                if (filename.substr(filename.find_last_of(".") + 1) == "json") {
                    std::string fullPath = "gameFiles/register/blocks/" + filename;
                    std::ifstream blockFile(fullPath);
                    if (!blockFile.is_open()) {
                        std::cerr << "Failed to open block file: " << fullPath << std::endl;
                        continue;
                    }
                    std::stringstream buffer;
                    buffer << blockFile.rdbuf();
                    std::string contents = buffer.str();
                    blockFile.close();
                    parseJson(contents, filename);
                }
            }
        }
    }
    #endif
}
