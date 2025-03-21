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

BlockRegister::BlockRegister() {
    loadBlocks();
}

BlockRegister::~BlockRegister() {}

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
void BlockRegister::registerBlock(std::string name, std::vector<std::string> states, std::vector<std::string> textures,
                                  std::string model, bool solid, bool transparent, bool air, BLOCKTYPE type) {

    Block block(name, blocks.size(), solid, transparent, air, type);

    block.states = states;
    block.textures = textures;
    block.model = model;

    // TODO: Potentially find a better ID system, since block IDs can change if blocks are removed or added
    block.ID = blocks.size();
    blocks.push_back(block);
}

// Parses a JSON file to create and register a block
void BlockRegister::parseJson(std::string contents, std::string fileName) {

    nlohmann::json blockJson = nlohmann::json::parse(contents);

    std::string name = fileName.substr(0, fileName.find_last_of("."));
    std::replace(name.begin(), name.end(), '_', ' ');
    name[0] = std::toupper(name[0]);
    for (int i = 1; i < name.size(); i++) {
        if (name[i - 1] == ' ') {
            name[i] = std::toupper(name[i]);
        }
    }

    std::string blockType = blockJson["block_type"].get<std::string>();
    std::transform(blockType.begin(), blockType.end(), blockType.begin(), ::toupper);
    if (blockTypeMap.find(blockType) == blockTypeMap.end()) {
        std::cerr << "Invalid block type: " << blockType << std::endl;
        return;
    }
    BLOCKTYPE type = blockTypeMap[blockJson["block_type"].get<std::string>()];

    bool solid = blockJson["solid"];
    bool transparent = blockJson["transparent"];

    // Extract texture information from the JSON data
    // This happens in alphabetical order regardless of the order in the JSON file
    std::vector<std::string> textures;
    for (auto& [key, value] : blockJson["textures"].items()) {
        std::string texture = key + ":" + value.get<std::string>();
        textures.push_back(texture);
    }

    // TODO: Use states for flipping textures, or modifying textures in shaders, etc
    std::vector<std::string> states;
    for (auto& [key, value] : blockJson["states"].items()) {
        std::string state = key + ":" + value.get<std::string>();
        states.push_back(state);
    }

    registerBlock(name, states, textures, blockJson["model"].get<std::string>(), solid, transparent, false, type);
}

// Loads blocks from JSON files in a specified directory
void BlockRegister::loadBlocks() {

    // Registers a default air block as block 0 in the vector
    registerBlock("air", {}, {}, "air", false, false, true, AIR);

    #if defined(_WIN32)
    if (!std::filesystem::exists("../../assets/maps/blocks/")) {
        std::cerr << "Error locating folder: ../../assets/maps/blocks/" << std::endl;
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator("../../assets/maps/blocks/")) {
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
    if ((dir = opendir("../../assets/maps/blocks/")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                std::string filename = ent->d_name;
                if (filename.substr(filename.find_last_of(".") + 1) == "json") {
                    std::string fullPath = "../../assets/maps/blocks/" + filename;
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
