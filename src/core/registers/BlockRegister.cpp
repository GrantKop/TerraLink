#include "core/registers/BlockRegister.h"
#include "core/game/Game.h"

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

// Static instance of BlockRegister
BlockRegister* BlockRegister::s_instance = nullptr;

// Sets the static instance of BlockRegister
void BlockRegister::setInstance(BlockRegister* instance) {
    s_instance = instance;
}

// Returns the static instance of BlockRegister
BlockRegister& BlockRegister::instance() {
    if (!s_instance) {
        s_instance = new BlockRegister();
    }
    return *s_instance;
}

// Default constructor
BlockRegister::BlockRegister() {
    parseBlockRegistryJson();
    loadBlocks();
    saveBlockRegistryJson();
}

BlockRegister::~BlockRegister() {}

// Retrieves a block by its name from the registered blocks
const Block BlockRegister::getBlockByName(std::string name) {
    for (Block block : blocks) {
        if (block.name == name) {
            return block;
        }
    }
    std::cerr << "Block not found: " << name << std::endl;
    return Block();
}

// Retrieves a block by its index from the registered blocks
const Block BlockRegister::getBlockByIndex(int index) {
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
    Block block;

    block.isSolid = solid;
    block.isTransparent = transparent;
    block.isAir = air;
    block.type = type;

    block.states = states;
    block.textures = textures;
    block.model = model;

    linkModelToBlock(block);

    // Use blocks.size() as ID if block name not found in the map
    if (nameToIndexMap.find(name) == nameToIndexMap.end()) {
        block.name = name;
        block.ID = blocks.size();
        blocks.push_back(block);
        nameToIndexMap[name] = block.ID;
    } else {
        // set block to point to the existing block in the vector
        block.ID = nameToIndexMap[name];
        block.name = name;
        blocks[nameToIndexMap[name]] = block;
    }
}

// Parses a JSON file to create and register a block
void BlockRegister::parseBlockMapJson(std::string contents, std::string fileName) {

    nlohmann::json blockJson = nlohmann::json::parse(contents);

    std::string name = fileName.substr(0, fileName.find_last_of("."));
    std::replace(name.begin(), name.end(), '_', ' ');
    name[0] = std::toupper(name[0]);
    for (int i = 1; i < name.size(); i++) {
        if (name[i - 1] == ' ') {
            name[i] = std::toupper(name[i]);
        }
    }

    bool isAir = false;
    if (name == "Air") {
        isAir = true;
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

    registerBlock(name, states, textures, blockJson["model"].get<std::string>(), solid, transparent, isAir, type);
}

// Loads blocks from JSON files in a specified directory
void BlockRegister::loadBlocks() {

    // // Registers a default air block as block 0 in the vector
    // if (blocks.size() == 0) {
    //     blocks.push_back(Block("Air", 0, false, true, true, AIR));
    //     nameToIndexMap["Air"] = 0;
    // }

    #if defined(_WIN32)
    if (!std::filesystem::exists(Game::instance().getBasePath() + "/assets/maps/blocks/")) {
        std::cerr << "Error locating folder: ./assets/maps/blocks/" << std::endl;
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(Game::instance().getBasePath() + "/assets/maps/blocks/")) {
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
            parseBlockMapJson(contents, entry.path().filename().string());
        }
    }
    #else
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(Game::instance().getBasePath() + "/assets/maps/blocks/")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                std::string filename = ent->d_name;
                if (filename.substr(filename.find_last_of(".") + 1) == "json") {
                    std::string fullPath = Game::instance().getBasePath() + "/assets/maps/blocks/" + filename;
                    std::ifstream blockFile(fullPath);
                    if (!blockFile.is_open()) {
                        std::cerr << "Failed to open block file: " << fullPath << std::endl;
                        continue;
                    }
                    std::stringstream buffer;
                    buffer << blockFile.rdbuf();
                    std::string contents = buffer.str();
                    blockFile.close();
                    parseBlockMapJson(contents, filename);
                }
            }
        }
    }
    #endif
}

// Saves the block registry to a JSON file as names and IDs, for persistent ID storage
void BlockRegister::saveBlockRegistryJson() {
    nlohmann::json blockRegistryJson;

    for (const auto& [name, id] : nameToIndexMap) {
        blockRegistryJson[name] = id;
    }

    std::ofstream file(Game::instance().getBasePath() + "/registry/block_registry.json");
    if (!file) {
        std::cerr << "Error opening file for writing: ./registry/block_registry.json" << std::endl;
        return;
    }

    file << blockRegistryJson.dump(4); // Pretty print with 4 spaces
    file.close();
}

// Parses the block registry JSON file to create a mapping of block names to IDs
void BlockRegister::parseBlockRegistryJson() {
    std::ifstream file(Game::instance().getBasePath() + "/registry/block_registry.json");
    if (!file) {
        std::cerr << "Error opening file for reading: ./registry/block_registry.json" << std::endl;
        return;
    }

    nlohmann::json blockRegistryJson;
    file >> blockRegistryJson;

    Block block;

    for (auto& [name, id] : blockRegistryJson.items()) {
        nameToIndexMap[name] = id.get<int>();
        block.ID = id.get<int>();
        block.name = name;
        blocks.push_back(block);
    }

    file.close();
}

// Sets vertices and normals for a block based on its model
void BlockRegister::linkModelToBlock(Block& block) {
    if (block.model == "block_full" || block.model == "block_slim") {
        link_block_full(block);
    }
    if (block.model == "covered_cross") {
        link_covered_cross(block);
    }
    if (block.model == "air") {
        block.vertices.clear();
    }
}

// Model specific linking for the default cube model
// Json reading library reads textures on map in alphabetical order, default blocks must be created with BaBoFLRT order in mind
void BlockRegister::link_block_full(Block& block) {
    std::string modelPath;
    if (block.model == "block_full") {
        modelPath = Game::instance().getBasePath() + "/assets/models/block_full.obj";
    } else if (block.model == "block_slim") {
        modelPath = Game::instance().getBasePath() + "/assets/models/block_slim.obj";
    } else {
        std::cerr << "Invalid block model: " << block.model << std::endl;
        return;
    }
    std::ifstream file(modelPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open block model file: " << modelPath << std::endl;
        return;
    }

    std::vector<Vertex> face;
    std::vector<Vertex> tempFaces[6];
    int currentFaceIdx = 0;

    std::string line;
    std::vector<glm::vec3> OBJvertices;
    std::vector<glm::vec3> OBJnormals;
    while (std::getline(file, line)) {
        // Parse vertices
        if (line.substr(0, 2) == "v ") {
            std::istringstream s(line.substr(2));

            glm::vec3 vertex;

            s >> vertex.x >> vertex.y >> vertex.z;

            if (vertex.x == -1.0f) vertex.x = 1.0f;

            OBJvertices.push_back(vertex);

        // Parse normals
        } else if (line.substr(0, 3) == "vn ") {
            std::istringstream s(line.substr(3));

            glm::vec3 normal;

            s >> normal.x >> normal.y >> normal.z;

            OBJnormals.push_back(normal);

        // Parse faces
        } else if (line.substr(0, 2) == "f ") {
            std::istringstream s(line.substr(2));

            std::string faceData;

            while (s >> faceData) {
                size_t vPos = faceData.find("//");
                int vIndex = std::stoi(faceData.substr(0, vPos)) - 1;
                int nIndex = std::stoi(faceData.substr(vPos + 2)) - 1;

                Vertex vertex;
                vertex.position = OBJvertices[vIndex];
                vertex.normal = OBJnormals[nIndex];

                face.push_back(vertex);
            }

            if (currentFaceIdx < 6) {
                tempFaces[currentFaceIdx] = face;
                currentFaceIdx++;
                face.clear();
            }
        }
    }

    int reorder[6] = {1, 3, 5, 2, 4, 0};

    // Reorder faces to match BaBoFLRT
    for (int i = 0; i < 6; ++i) {
        const std::vector<Vertex>& face = tempFaces[reorder[i]];
        for (const auto& vertex : face) {
            block.vertices.push_back(vertex);
        }
    }
}

// Model specific linking for the covered cross model
void BlockRegister::link_covered_cross(Block& block) {

    std::string modelPath = Game::instance().getBasePath() + "/assets/models/covered_cross.obj";
    std::ifstream file(modelPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open block model file: " << modelPath << std::endl;
        return;
    }

    std::vector<glm::vec3> OBJvertices;
    std::vector<glm::vec3> OBJnormals;

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            std::istringstream s(line.substr(2));
            glm::vec3 vertex;
            s >> vertex.x >> vertex.y >> vertex.z;
            if (vertex.z == -0.5f) vertex.z = 0.5f;
            if (vertex.z == -1.0f) vertex.z = 1.0f;
            if (vertex.y == -0.5210f) vertex.y = 0.5f;
            if (vertex.y == 0.5210f) vertex.y = 0.5f;
            if (vertex.z == -0.8536f) vertex.z = 0.8536f;

            OBJvertices.push_back(vertex);
        } else if (line.substr(0, 3) == "vn ") {
            std::istringstream s(line.substr(3));
            glm::vec3 normal;
            s >> normal.x >> normal.y >> normal.z;
            OBJnormals.push_back(normal);
        } else if (line.substr(0, 2) == "f ") {
            std::istringstream s(line.substr(2));
            std::string faceData;
            while (s >> faceData) {
                size_t vPos = faceData.find("//");
                int vIndex = std::stoi(faceData.substr(0, vPos)) - 1;
                int nIndex = std::stoi(faceData.substr(vPos + 2)) - 1;

                Vertex vertex;
                vertex.position = OBJvertices[vIndex];
                vertex.normal = OBJnormals[nIndex];
                block.vertices.push_back(vertex);
            }
        }
    }
}
