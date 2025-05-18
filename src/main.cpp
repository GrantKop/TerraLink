#include <iostream>

#include "graphics/VertexArrayObject.h"
#include "graphics/Texture.h"
#include "core/registers/AtlasRegister.h"
#include "core/world/World.h"
#include "core/player/Player.h"
#include "core/game/Game.h"
#include "network/Network.h"

bool enableFog;

void parseGameSettings(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open game settings file: " << filePath << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t commentLine = line.find("#");
        if (commentLine != std::string::npos) {
            line = line.substr(0, commentLine);
        }

        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (line.empty()) continue;

        size_t equalPos = line.find("=");
        if (equalPos == std::string::npos) {
            std::cerr << "Invalid line in game settings file: " << line << std::endl;
            continue;
        }

        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        if (key == "renderDistance") 
            Player::instance().VIEW_DISTANCE = std::stoi(value);
        else if (key == "fov")
            Player::instance().getCamera().setFOV(std::stof(value));
        else if (key == "sensitivity")
            Player::instance().getCamera().setSensitivity(std::stof(value));
        else if (key == "playerName")
            Player::instance().setPlayerName(value);
        else if (key == "saveName") {
            World::instance().setSaveDirectory(value);
            Game::instance().setWorldSave(value);
        }
        else if (key == "seed")
            World::instance().setSeed(std::stoi(value));
        else if (key == "distanceFog")
            enableFog = (value == "true" || value == "1" || value == "True" || value == "TRUE");
    }
}

int _fpsCount = 0, fps = 0;
float prevTime = 0.0f;

std::string fpsCount() {

    float curTime = glfwGetTime();
    ++_fpsCount;
    if (curTime - prevTime > 1.0f) {
        prevTime = curTime;
        fps = _fpsCount;
        _fpsCount = 0;
    }

    return std::string(("TerraLink " + Game::instance().getGameVersion()).c_str()) + "  //  " + std::to_string(fps) + " fps";
}

int main(int argc, char** argv) {

    if (argc >= 2) {
            std::string mode = argv[1];
        if (mode == "server") {
            NetworkManager::setRole(NetworkRole::SERVER);
        } else if (mode == "client") {
            NetworkManager::setRole(NetworkRole::CLIENT);
        } else if (mode == "host") {
            NetworkManager::setRole(NetworkRole::HOST);
        } else {
            std::cerr << "Invalid network role: " << mode << std::endl;
            return 1;
        }

        std::cout << "\nStarting TerraLink in " << mode << " mode...\n" << std::endl;
    } else {
        std::cout << "No network role specified. Defaulting to CLIENT." << std::endl;
        std::cout << "Usage: " << argv[0] << " [server|client|host]" << std::endl;
        NetworkManager::setRole(NetworkRole::CLIENT);
    }

    Game game;
    Game::setInstance(&game);

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, "TerraLink", 800, 600)) {
        glfwTerminate();
        return -1;
    }

    Player player(window);
    Player::setInstance(&player);

    BlockRegister blockRegister;
    Atlas blockAtlas((Game::instance().getBasePath() + "/assets/textures/blocks/").c_str());
    blockAtlas.linkBlocksToAtlas(&blockRegister);
    BlockRegister::setInstance(&blockRegister);

    game.init();

    parseGameSettings((Game::instance().getBasePath() + "/game.settings").c_str());

    game.getWorld().createSaveDirectory();

    World::instance().loadPlayerData(Player::instance(), Player::instance().getPlayerName());

    Shader shaderProgram((Game::instance().getBasePath() + "/shaders/block.vert").c_str(), (Game::instance().getBasePath() + "/shaders/block.frag").c_str());
    Shader cloudShader((Game::instance().getBasePath() + "/shaders/cloud.vert").c_str(), (Game::instance().getBasePath() + "/shaders/cloud.frag").c_str());

    shaderProgram.use();

    Texture atlas((Game::instance().getBasePath() + "/assets/textures/blocks/block_atlas.png").c_str(), GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    atlas.setUniform(shaderProgram, "tex0", 0);

    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.3f));
    glUniform3fv(glGetUniformLocation(shaderProgram.ID, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), 1.0f, 1.0f, 1.0f, 1.0f);

    glUniform3f(glGetUniformLocation(shaderProgram.ID, "fogColor"), 0.38f, 0.66f, 0.77f);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "fogDensity"), 0.015f);

    glUniform3f(glGetUniformLocation(shaderProgram.ID, "foliageColor"), 0.3f, 0.7f, 0.2f);
    GLint location = glGetUniformLocation(shaderProgram.ID, "useFog");
    glUniform1i(location, enableFog ? 1 : 0);
    

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    while (!glfwWindowShouldClose(window)) {
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame += deltaTime;

        glClearColor(0.38f, 0.66f, 0.77f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        game.getWorld().uploadChunkMeshes(15);
        game.getWorld().unloadDistantChunks();
        game.getWorld().uploadChunksToMap();

        player.update(deltaTime);

        shaderProgram.setUniform4("cameraMatrix", player.getCamera().cameraMatrix);
        shaderProgram.setUniform3("camPos", player.getCamera().position);
        atlas.bind();

        for (auto& [pos, chunk] : game.getWorld().chunks) {
            if (!chunk->mesh.isUploaded || chunk->mesh.vertices.empty() || chunk->mesh.indices.empty()) continue;

            chunk->mesh.VAO.bind();
            glm::mat4 model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, chunk->mesh.indices.size(), GL_UNSIGNED_INT, 0);
        }

        // game.getWorld().chunkReset();
        glfwSwapBuffers(window);
        glfwPollEvents();

        glfwSetWindowTitle(window, fpsCount().c_str());

        // CHECK_GL_ERROR();
    }

    atlas.deleteTexture();
    shaderProgram.deleteShader();
    glfwTerminate();

    game.getWorld().shutdown();

    if (Game::instance().isReleaseMode()) {
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.ignore();
    }

    return 0;
}
