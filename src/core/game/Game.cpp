#include "core/game/Game.h"
#include "core/world/World.h"
#include "audio/AudioManager.h"
#include "core/player/Player.h"
#include "core/registers/AtlasRegister.h"
#include "core/game/GameInit.h"
#include "graphics/Shader.h"
#include "network/Network.h"
#include "network/Serializer.h"

int _fpsCount = 0, fps = 0;
float prevTime = 0.0f;

std::string Game::fpsCount() {

    float curTime = glfwGetTime();
    ++_fpsCount;
    if (curTime - prevTime > 1.0f) {
        prevTime = curTime;
        fps = _fpsCount;
        _fpsCount = 0;
    }

    return std::string(("TerraLink " + getGameVersion()).c_str()) + "  //  " + std::to_string(fps) + " fps";
}

Game::Game(GLFWwindow* windowptr, bool devMode) {
    window = windowptr;
    if (window == nullptr) {
        std::cerr << "Error: Window is null!" << std::endl;
        exit(EXIT_FAILURE);
    }

    player = std::make_unique<Player>(window);
    Player::setInstance(player.get());

    DEV_MODE = devMode;
    basePath = DEV_MODE
        ? std::filesystem::current_path().parent_path().parent_path()
        : std::filesystem::current_path().parent_path();

    savePath = basePath / "saves";
}

Game::~Game() {}

void Game::gameLoop() {
    while (!glfwWindowShouldClose(window)) {
        deltaTime = static_cast<float>(glfwGetTime() - lastFrame);
        lastFrame += deltaTime;

        glClearColor(0.38f, 0.66f, 0.77f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        tick();
        render();

        glfwSetWindowTitle(window, fpsCount().c_str());
    }

    shutdown();

    if (!DEV_MODE) {
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.ignore();  
    }

    exit(0);
    return;
}

void Game::loadAssets() {
    
    Atlas blockAtlas((Game::instance().getBasePath() + "/assets/textures/blocks/").c_str());
    blockRegister = std::make_unique<BlockRegister>();
    blockAtlas.linkBlocksToAtlas(blockRegister.get());
    BlockRegister::setInstance(blockRegister.get());

    atlas = std::make_unique<Texture>(
        (getBasePath() + "/assets/textures/blocks/block_atlas.png").c_str(),
        GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        GL_NEAREST, GL_CLAMP_TO_BORDER
    );
    shaderProgram->use();
    atlas->setUniform(*shaderProgram, "tex0", 0);

    AudioManager::setMusicVolume(musicVolume);
    AudioManager::setSoundVolume(soundVolume);
    AudioManager::init();
    AudioManager::loadMusicTracks(getBasePath() + "/assets/sounds/music/");
}

void Game::setupShadersAndUniforms() {
    shaderProgram = std::make_unique<Shader>(
        getBasePath() + "/shaders/block.vert",
        getBasePath() + "/shaders/block.frag"
    );

    shaderProgram->use();
    shaderProgram->setUniform3("lightDir", glm::normalize(glm::vec3(-1.0f, -1.0f, -0.3f)));
    shaderProgram->setUniform4("lightColor", glm::vec4(1.0f));
    shaderProgram->setUniform3("fogColor", glm::vec3(0.38f, 0.66f, 0.77f));
    shaderProgram->setFloat("fogDensity", 0.015f);
    shaderProgram->setUniform3("foliageColor", glm::vec3(0.3f, 0.7f, 0.2f));
    shaderProgram->setInt("useFog", isFogEnabled() ? 1 : 0);
    shaderProgram->setFloat("fogStart", Player::instance().getNearFogDistance());
    shaderProgram->setFloat("fogEnd", Player::instance().getFarFogDistance());
    shaderProgram->setFloat("fogBottom", Player::instance().getBottomFogDistance());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

Game* Game::s_instance = nullptr;

void Game::setInstance(Game* instance) {
    s_instance = instance;
}

Game& Game::instance() {
    if (!s_instance) {
        std::cerr << "Game::instance() called before Game::setInstance()!\n";
        std::exit(1);
    }
    return *s_instance;
}

std::string Game::getBasePath() const {
    return basePath.string();
}

std::string Game::getSavePath() const {
    return savePath.string();
}

void Game::init() {

    GameInit::parseGameSettings((basePath.string() + "/game.settings").c_str());

    setupShadersAndUniforms();
    loadAssets();

    world = std::make_unique<World>();
    World::setInstance(world.get());

    world->init();
    world->setSaveDirectory(getWorldSave());
    world->createSaveDirectory();

    std::cout << "Waiting for spawn chunks...\n";

    // while (world->chunks.size() < 1) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }

    world->loadPlayerData(*player, player->getPlayerName());
}

void Game::tick() {
    getWorld().uploadChunkMeshes(15);
    getWorld().unloadDistantChunks();
    getWorld().uploadChunksToMap();
}

void Game::render() {
    Player::instance().update(deltaTime);

    shaderProgram->use();

    shaderProgram->setUniform4("cameraMatrix", Player::instance().getCamera().cameraMatrix);
    shaderProgram->setUniform3("camPos", Player::instance().getCamera().position);
    atlas->bind();

    for (auto& [pos, chunk] : world->chunks) {
        if (!chunk->mesh.isUploaded || chunk->mesh.vertices.empty() || chunk->mesh.indices.empty()) continue;
        chunk->mesh.VAO.bind();
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawElements(GL_TRIANGLES, chunk->mesh.indices.size(), GL_UNSIGNED_INT, 0);
    }

    AudioManager::update(deltaTime);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Game::shutdown() {
    atlas->deleteTexture();
    shaderProgram->deleteShader();
    AudioManager::shutdown();

    glfwTerminate();

    world->shutdown();
}

std::string Game::getWorldSave() const {
    return curWorldSave;
}

void Game::setWorldSave(const std::string& saveName) {
    curWorldSave = saveName;
}

World& Game::getWorld() {
    return *world;
}

std::string Game::getGameVersion() const {
    return "v" + std::to_string(static_cast<int>(gameVersionMajor)) + "." +
                 std::to_string(static_cast<int>(gameVersionMinor)) + "." +
                 std::to_string(static_cast<int>(gameVersionPatch));
}