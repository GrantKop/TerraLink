#include "core/registers/AtlasRegister.h"

// Finds the largest square atlas size for a given number of files, in powers of 2
int findLargestAtlas(int numFiles) {
    int size = 1;
    while (size * size < numFiles) {
        size *= 2;
    }
    return size;
}

int findLargestTexture(std::vector<TextureFile> textures) {
    int size = 1;
    for (TextureFile texture : textures) {
        if (texture.width > size) {
            size = texture.width;
        }
    }
    return size;
}

std::vector<TextureFile> loadTextures(const char* path) {
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

void createAtlas(const char* path) {
    
    std::vector<TextureFile> textures = loadTextures(path);
    if (textures.size() == 0) {
        std::cerr << "No textures loaded for atlas creation" << std::endl;
        return;
    }

    int count = textures.size();

    // Size in textures squared that the atlas will hold
    int size = findLargestAtlas(count);
    // Size of each texture in the atlas
    int offset = findLargestTexture(textures);

    int totalSize = size * offset;

    unsigned char* atlasData = new unsigned char[totalSize * totalSize * 4];

    // Initialize atlas with transparent black (0, 0, 0, 0)
    memset(atlasData, 0, totalSize * totalSize * 4);

    for (int i = 0; i < count; i++) {
        int xOffset = (i % size) * offset;
        int yOffset = (i / size) * offset;

        for (int y = 0; y < offset; y++) {
            for (int x = 0; x < offset; x++) {
                int atlasIndex = ((yOffset + y) * totalSize + (xOffset + x)) * 4;
                int textureIndex = (y * offset + x) * 4;

                atlasData[atlasIndex] = textures[i].data[textureIndex];         // Red
                atlasData[atlasIndex + 1] = textures[i].data[textureIndex + 1]; // Green
                atlasData[atlasIndex + 2] = textures[i].data[textureIndex + 2]; // Blue
                atlasData[atlasIndex + 3] = textures[i].data[textureIndex + 3]; // Alpha
            }
        }

        stbi_image_free(textures[i].data);
    }

    stbi_write_png((std::string(path) + "block_atlas.png").c_str(), totalSize, totalSize, 4, atlasData, totalSize * 4);
    delete[] atlasData;
}

// Registers an atlas with the given path
void registerAtlas() {

    createAtlas("../../assets/textures/blocks/");

}

void linkBlocksToAtlas(BlockRegister* blockRegister) {}

