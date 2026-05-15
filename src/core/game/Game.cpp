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
        renderUI();

        glfwSwapBuffers(window);
        glfwPollEvents();
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

    crosshairTex = std::make_unique<Texture>(
        (getBasePath() + "/assets/textures/ui/crosshair.png").c_str(),
        GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        GL_NEAREST, GL_CLAMP_TO_BORDER);

        int width = 32, height = 32;

    std::vector<Vertex> crosshairVertices = {
        {{0.0f, 0.0f, 0.0f},        {0, 0, 1},    {0.0f, 1.0f}},
        {{1.0f, 0.0f, 0.0f},        {0, 0, 1},    {1.0f, 1.0f}},
        {{1.0f, 1.0f, 0.0f},        {0, 0, 1},    {1.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f},        {0, 0, 1},    {0.0f, 0.0f}},
    };

    std::vector<GLuint> crosshairIndices = {
        0, 2, 1,
        0, 3, 2
    };

    crosshairVAO = std::make_unique<VertexArrayObject>();
    crosshairVAO->init();
    crosshairVAO->bind();
    crosshairVAO->addVertexBuffer(crosshairVertices);
    crosshairVAO->addElementBuffer(crosshairIndices);
    crosshairVAO->addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    crosshairVAO->addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    crosshairVAO->addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    crosshairVAO->unbind();

    uiShaderProgram->use();
    crosshairTex->setUniform(*uiShaderProgram, "tex0", 0);

    std::vector<Vertex> cubeVerts = {
        {{0,0,0}, {}, {}}, {{1,0,0}, {}, {}}, {{1,1,0}, {}, {}}, {{0,1,0}, {}, {}},
        {{0,0,1}, {}, {}}, {{1,0,1}, {}, {}}, {{1,1,1}, {}, {}}, {{0,1,1}, {}, {}}
    };

    std::vector<GLuint> cubeLineIndices = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7 
    };
    
    wireFrameVAO = std::make_unique<VertexArrayObject>();
    wireFrameVAO->init();
    wireFrameVAO->bind();
    wireFrameVAO->addVertexBuffer(cubeVerts);
    wireFrameVAO->addElementBuffer(cubeLineIndices);
    wireFrameVAO->addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    wireFrameVAO->unbind();

    textRenderer = std::make_unique<TextRenderer>();
    textRenderer->init(getBasePath());

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

    uiShaderProgram = std::make_unique<Shader>(
        getBasePath() + "/shaders/ui.vert",
        getBasePath() + "/shaders/ui.frag"
    );

    wireFrameShaderProgram = std::make_unique<Shader>(
        getBasePath() + "/shaders/wireframe.vert",
        getBasePath() + "/shaders/wireframe.frag"
    );

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
    if (!NetworkManager::instance().isOnlineMode()) world->createSaveDirectory();

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
    updateSelectedBlockHUD(deltaTime);

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
    renderBlockOutline();
    renderHeldBlock();
}

void Game::shutdown() {
    atlas->deleteTexture();
    crosshairTex->deleteTexture();
    shaderProgram->deleteShader();
    uiShaderProgram->deleteShader();
    wireFrameShaderProgram->deleteShader();
    if (textRenderer) textRenderer->shutdown();
    if (heldBlockVAO) heldBlockVAO->deleteBuffers();
    AudioManager::shutdown();

    glfwTerminate();

    world->shutdown();
}

void Game::renderUI() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int winW, winH;
    glfwGetFramebufferSize(window, &winW, &winH);

    float scaleFactor = 0.015f;
    float size = winH * scaleFactor;
    float halfSize = size / 2.0f;

    glm::mat4 projection = glm::ortho(0.0f, float(winW), float(winH), 0.0f, -1.0f, 1.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(winW / 2.0f - halfSize, winH / 2.0f - halfSize, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(size, size, 1.0f));

    glm::mat4 final = projection * model * scale;

    uiShaderProgram->use();
    crosshairTex->bind();
    crosshairTex->setUniform(*uiShaderProgram, "tex0", 0);
    uiShaderProgram->setMat4("uProjection", final);


    crosshairVAO->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    crosshairVAO->unbind();

    // Selected-block name label (drawn while alpha blending is still enabled).
    renderSelectedBlockHUD(projection, winW, winH);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Resolves the selected block's human-readable name (lookup kept here, out of
// the renderer) and advances the fade timer. Called once per frame.
void Game::updateSelectedBlockHUD(float deltaTime) {
    int selected = Player::instance().selectedBlockID;

    if (selected != hudLastSelectedBlockID) {
        hudLastSelectedBlockID = selected;

        Block block = BlockRegister::instance().getBlockByIndex(selected);
        hudBlockName = block.name.empty()
            ? ("Block " + std::to_string(selected))
            : block.name;

        hudLabelTimer = 0.0f; // restart the show/fade cycle
    }

    if (hudLabelTimer < HUD_FADE_TIME) {
        hudLabelTimer += deltaTime;
    }
}

// Draws the resolved label, centered horizontally, with a fade-out alpha.
void Game::renderSelectedBlockHUD(const glm::mat4& projection, int winW, int winH) {
    if (!textRenderer || !textRenderer->isReady()) return;
    if (hudBlockName.empty() || hudLabelTimer >= HUD_FADE_TIME) return;

    float alpha;
    if (hudLabelTimer <= HUD_HOLD_TIME) {
        alpha = 1.0f;
    } else {
        alpha = 1.0f - (hudLabelTimer - HUD_HOLD_TIME) / (HUD_FADE_TIME - HUD_HOLD_TIME);
    }
    if (alpha <= 0.0f) return;

    float pixelHeight = winH * 0.035f;
    float textWidth = textRenderer->measureWidth(hudBlockName, pixelHeight);
    float x = (winW - textWidth) * 0.5f;
    float y = winH * 0.78f; // below the crosshair, above the bottom edge

    // Drop shadow first for legibility over the bright sky, then the label.
    float shadowOffset = pixelHeight * 0.08f;
    textRenderer->drawText(hudBlockName, x + shadowOffset, y + shadowOffset,
                           pixelHeight, glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.6f),
                           projection);
    textRenderer->drawText(hudBlockName, x, y, pixelHeight,
                           glm::vec4(1.0f, 1.0f, 1.0f, alpha), projection);
}

void Game::renderBlockOutline() {
    glLineWidth(2.0f);

    auto hit = Player::instance().getHighlightedBlock();
    glm::vec3 pos;

    if (hit.has_value()) {
        glm::ivec3 blockPos = hit.value();
        int id = getWorld().getBlockIDAtWorldPosition(blockPos.x, blockPos.y, blockPos.z);
        if (id == 0) return;

        pos = glm::vec3(blockPos);
    } else {
        return;
    }

    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos - glm::vec3(0.001f));
    model = glm::scale(model, glm::vec3(1.002f));
    glm::mat4 mvp = Player::instance().getCamera().cameraMatrix * model;

    wireFrameShaderProgram->use();
    wireFrameShaderProgram->setMat4("uMVP", mvp);

    wireFrameVAO->bind();
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    wireFrameVAO->unbind();
}

// Builds (or rebuilds) the held-block mesh from the block's render-ready
// vertices. block.vertices already carries atlas UVs (set by Atlas linking);
// we only need to add the engine's standard quad -> triangle indices.
void Game::buildHeldBlockMesh(int blockID) {
    heldBlockID = blockID;
    heldBlockIndexCount = 0;

    Block block = BlockRegister::instance().getBlockByIndex(blockID);
    if (block.vertices.empty() || block.vertices.size() % 4 != 0) {
        return; // air / non-quad model: nothing sensible to hold
    }

    std::vector<Vertex> verts = block.vertices;
    std::vector<GLuint> indices;
    indices.reserve(verts.size() / 4 * 6);
    for (GLuint i = 0; i < verts.size(); i += 4) {
        // Same quad winding the chunk mesher uses, so global GL_CULL_FACE
        // shows the outward faces just like world blocks.
        indices.insert(indices.end(), {
            i, i + 2, i + 1,
            i, i + 3, i + 2
        });
    }

    if (!heldBlockVAO) {
        heldBlockVAO = std::make_unique<VertexArrayObject>();
        heldBlockVAO->init();
    }

    heldBlockVAO->bind();
    heldBlockVAO->addVertexBuffer(verts, GL_DYNAMIC_DRAW);
    heldBlockVAO->addElementBuffer(indices, GL_DYNAMIC_DRAW);
    heldBlockVAO->addAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    heldBlockVAO->addAttribute(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    heldBlockVAO->addAttribute(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    heldBlockVAO->unbind();

    heldBlockIndexCount = static_cast<int>(indices.size());
}

// Draws the selected (to-be-placed) block as a small 3D cube in the
// bottom-right corner. Reuses the world block shader + atlas; the only
// difference is a screen-anchored projection/model instead of the world
// camera, so the block stays put regardless of where the player looks.
void Game::renderHeldBlock() {
    int id = Player::instance().selectedBlockID;
    if (id != heldBlockID) {
        buildHeldBlockMesh(id);
    }
    if (heldBlockIndexCount == 0 || !heldBlockVAO) return;

    int winW, winH;
    glfwGetFramebufferSize(window, &winW, &winH);
    if (winW <= 0 || winH <= 0) return;
    float aspect = static_cast<float>(winW) / static_cast<float>(winH);

    // Its own little perspective view; the cube sits ~1 unit in front.
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 10.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.62f, -0.40f, -1.15f)); // bottom-right
    model = glm::rotate(model, glm::radians(45.0f),  glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.36f));
    model = glm::translate(model, glm::vec3(-0.5f)); // rotate about cube center

    // Draw over the world without touching its colors: only depth is cleared.
    glClear(GL_DEPTH_BUFFER_BIT);

    shaderProgram->use();
    shaderProgram->setUniform4("cameraMatrix", proj);
    shaderProgram->setMat4("model", model);
    shaderProgram->setUniform3("camPos", Player::instance().getCamera().position);
    shaderProgram->setInt("useFog", 0); // never fog the held item
    atlas->bind();

    heldBlockVAO->bind();
    glDrawElements(GL_TRIANGLES, heldBlockIndexCount, GL_UNSIGNED_INT, 0);
    heldBlockVAO->unbind();

    shaderProgram->setInt("useFog", isFogEnabled() ? 1 : 0); // restore world state
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