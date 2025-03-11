#include "core/registers/BlockRegister.h"

Block BlockRegister::getBlockByName(std::string name) {
    for (Block block : blocks) {
        if (block.name == name) {
            return block;
        }
    }
    std::cerr << "Block not found: " << name << std::endl;
    return Block();
}

Block BlockRegister::getBlockByIndex(int index) {
    if (index < blocks.size()) {
        return blocks[index];
    }
    std::cerr << "Block index out of range: " << index << std::endl;
    return Block();
}

int BlockRegister::getBlockIndex(std::string name) {
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].name == name) {
            return i;
        }
    }
    std::cerr << "Block not found: " << name << std::endl;
    return -1;
}

void BlockRegister::registerBlock(std::string name, std::vector<std::string> states, std::vector<std::string> textures, std::string model) {
    Block block;
    block.name = name;
    // block.vertices = vertices;
    // block.indices = indices;
    block.states = states;
    block.textures = textures;
    block.model = model;
    block.ID = blocks.size();
    blocks.push_back(block);
}

void BlockRegister::parseJson(std::string contents) {

    nlohmann::json blockJson = nlohmann::json::parse(contents);

    std::string name = blockJson["name"];

    // Get all listed texture values 
    std::vector<std::string> textures;
    for (auto& [key, value] : blockJson["textures"].items()) {
        std::string texture = key + ":" + value.get<std::string>();
        textures.push_back(texture);
    }

    // Get all states listed
    std::vector<std::string> states;
    for (auto& [key, value] : blockJson["states"].items()) {
        std::string state = key + ":" + value.get<std::string>();
        states.push_back(state);
    }
    

    registerBlock(name, states, textures, blockJson["mesh"].get<std::string>());
}

void BlockRegister::loadBlocks() {

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
            parseJson(contents);
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
                    parseJson(contents);
                }
            }
        }
    }
    #endif
}
