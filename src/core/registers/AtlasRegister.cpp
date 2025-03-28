#include "core/registers/AtlasRegister.h"

Atlas::Atlas(const char* path) {
    
    std::vector<TextureFile> textures = loadTextures(path);
    if (textures.size() == 0) {
        std::cerr << "No textures loaded for atlas creation" << std::endl;
        return;
    }

    numTextures = textures.size();

    this->size = findLargestAtlas(numTextures);

    largestTexture = findLargestTexture(textures);

    width = this->size * largestTexture;

    unsigned char* atlasData = new unsigned char[width * width * 4];

    // Initialize atlas with transparent black (0, 0, 0, 0)
    memset(atlasData, 0, width * width * 4);

    for (int i = 0; i < numTextures; i++) {
        int xOffset = (i % this->size) * largestTexture;
        int yOffset = (i / this->size) * largestTexture;

        for (int y = 0; y < largestTexture; y++) {
            for (int x = 0; x < largestTexture; x++) {
                int atlasIndex = ((yOffset + y) * width + (xOffset + x)) * 4;
                int textureIndex = (y * largestTexture + x) * 4;

                atlasData[atlasIndex] = textures[i].data[textureIndex];         // Red
                atlasData[atlasIndex + 1] = textures[i].data[textureIndex + 1]; // Green
                atlasData[atlasIndex + 2] = textures[i].data[textureIndex + 2]; // Blue
                atlasData[atlasIndex + 3] = textures[i].data[textureIndex + 3]; // Alpha

                textureMap[textures[i].name] = std::make_pair(xOffset, yOffset);
            }
        }

        stbi_image_free(textures[i].data);
    }

    stbi_write_png((std::string(path) + "block_atlas.png").c_str(), width, width, 4, atlasData, width * 4);
    delete[] atlasData;
}

Atlas::~Atlas() {}

// Returns the size of the atlas
int Atlas::getSize() {
    return this->size;
}

// Returns the atlas texture
Texture* Atlas::getAtlas() {
    return this->atlas;
}

// Finds the largest square atlas size for a given number of files, in powers of 2 since those are most efficient in OpenGL
int Atlas::findLargestAtlas(int numFiles) {
    int largest = 1;
    while (largest * largest < numFiles) {
        largest *= 2;
    }
    return largest;
}

// Finds the texture with the largest size in a vector of textures
int Atlas::findLargestTexture(std::vector<TextureFile> textures) {
    int largest = 1;
    for (TextureFile texture : textures) {
        if (texture.width > largest) {
            largest = texture.width;
        }
    }
    return largest;
}

std::vector<TextureFile> Atlas::loadTextures(const char* path) {
    std::vector<TextureFile> textures;

    #if defined(_WIN32)
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error locating folder: " << path << std::endl;
        return textures;
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            if (entry.path().filename() != "block_atlas.png") {
                TextureFile texture;
                texture.data = stbi_load(entry.path().string().c_str(), &texture.width, &texture.height, &texture.nrChannels, 0);
                texture.name = entry.path().filename().string().substr(0, entry.path().filename().string().find_last_of("."));
                if (texture.data == NULL) {
                    std::cerr << "Error loading texture: " << entry.path().string() << std::endl;
                    continue;
                }
                textures.push_back(texture);
            }
        }
    }
    #else
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                std::string filename = ent->d_name;
                if (filename.substr(filename.find_last_of(".") + 1) == "png") {
                    if (filename != "block_atlas.png") {
                        TextureFile texture;
                        std::string fullPath = std::string(path) + "/" + filename;
                        texture.data = stbi_load(fullPath.c_str(), &texture.width, &texture.height, &texture.nrChannels, 0);
                        texture.name = filename.substr(0, filename.find_last_of("."));
                        if (texture.data == NULL) {
                            std::cerr << "Error loading texture: " << fullPath << std::endl;
                            continue;
                        }
                        textures.push_back(texture);
                    }
                }
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Error locating folder: " << path << std::endl;
    }
    #endif
    return textures;
}

// Adds vertice texture coordinates to blocks based on the atlas
void Atlas::linkBlocksToAtlas(BlockRegister* blockRegister) {
    if (blocksLinked) {
        std::cout << "Blocks already linked to atlas\n";
        return;
    }

    blocksLinked = true;

    for (Block& block : blockRegister->blocks) {
        for (std::string texture : block.textures) {

            std::string textureKey = texture.substr(0, texture.find(":"));
            texture = texture.substr(texture.find(":") + 1);

            if (textureMap.find(texture) == textureMap.end()) {
                std::cerr << "Error linking block texture to atlas: " << texture << std::endl;
                continue;
            }

            float s = (float)textureMap[texture].first / (float)width;
            float t = ((float)width - (float)textureMap[texture].second - (float)largestTexture) / (float)width;

            if (block.model == "block_full") {
                block_full_linking(block, texture, textureKey, s, t);
            }
        }
    }
}

// Linking function for blocks that are the default cube model
void Atlas::block_full_linking(Block& block, std::string texture, std::string textureKey, float s, float t) {

    glm::vec2 texCoords;

    if (textureKey == "all") {
        for (int i = 0; i < 24; i++) {
            if (i % 4 == 0) {
                texCoords = glm::vec2(s, t);
            }
            if (i % 4 == 3) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i % 4 == 2) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i % 4 == 1) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[i].texCoords = texCoords;
        }
    }
    
    if (textureKey == "top") {
        for (int i = 0; i < 4; i++) {
            if (i == 1) {
                texCoords = glm::vec2(s, t);
            }
            if (i == 0) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i == 3) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i == 2) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < 19+i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[20+i].texCoords = texCoords;
        }
    } else if (textureKey == "bottom") {
        for (int i = 0; i < 4; i++) {
            if (i == 3) {
                texCoords = glm::vec2(s, t);
            }
            if (i == 2) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i == 1) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i == 0) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < 3+i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[4+i].texCoords = texCoords;
        }
    } else if (textureKey == "front") {
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                texCoords = glm::vec2(s, t);
            }
            if (i == 3) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i == 2) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i == 1) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < 7+i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[8+i].texCoords = texCoords;
        }
    } else if (textureKey == "back") {
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                texCoords = glm::vec2(s, t);
            }
            if (i == 3) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i == 2) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i == 1) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[i].texCoords = texCoords;
        }
    } else if (textureKey == "left") {
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                texCoords = glm::vec2(s, t);
            }
            if (i == 3) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i == 2) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i == 1) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < 11+i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[12+i].texCoords = texCoords;
        }
    } else if (textureKey == "right") {
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                texCoords = glm::vec2(s, t);
            }
            if (i == 3) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t);
            }
            if (i == 2) {
                texCoords = glm::vec2(s + (float)largestTexture / (float)width, t + (float)largestTexture / (float)width);
            }
            if (i == 1) {
                texCoords = glm::vec2(s, t + (float)largestTexture / (float)width);
            }
            if (block.vertices.size() < 15+i) {
                std::cerr << "Model block_full index [" << i << "] outside of range in texture: " << texture << " for face: " << textureKey << std::endl;
                continue;
            }
            block.vertices[16+i].texCoords = texCoords;
        }
    }
}
