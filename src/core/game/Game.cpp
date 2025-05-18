#include "core/game/Game.h"
#include "core/world/World.h"


Game::Game() {

}

Game::~Game() {
    shutdown();
}

Game* Game::s_instance = nullptr;

void Game::setInstance(Game* instance) {
    s_instance = instance;
}

Game& Game::instance() {
    if (!s_instance) {
        s_instance = new Game();
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
    world = std::make_unique<World>();
    World::setInstance(world.get());
}

void Game::tick() {

}

void Game::shutdown() {
    if (world) {
        world.reset();
    }
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