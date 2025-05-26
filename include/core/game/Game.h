#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <filesystem>

#include "core/player/Player.h"
#include "core/registers/BlockRegister.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"

class World;

class Game {
public:
    Game(GLFWwindow* windowptr, bool devMode = true);
    ~Game();

    void gameLoop();

    void loadAssets();
    void setupShadersAndUniforms();

    static void setInstance(Game* instance);
    static Game& instance();

    std::string getBasePath() const;
    std::string getSavePath() const;

    void init();
    void tick();
    void render();
    void shutdown();

    void setWorldSave(const std::string& saveName);
    std::string getWorldSave() const;

    World& getWorld();

    void setEnableFog(bool enable) { enableFog = enable; }
    bool isFogEnabled() const { return enableFog; }

    std::string getGameVersion() const;
    void setGameVersion(float major, float minor, float patch) {
        gameVersionMajor = major;
        gameVersionMinor = minor;
        gameVersionPatch = patch;
    }

    void setReleaseMode(bool releaseMode) { DEV_MODE = !releaseMode; }
    bool isReleaseMode() const { return !DEV_MODE; }

    void setMusicVolume(float volume) { musicVolume = volume; }
    float getMusicVolume() const { return musicVolume; }
    void setSoundVolume(float volume) { soundVolume = volume; }
    float getSoundVolume() const { return soundVolume; }

    std::string fpsCount();

private:
    std::string curWorldSave;
    std::unique_ptr<World> world;

    std::unique_ptr<Player> player;

    std::unique_ptr<BlockRegister> blockRegister;

    GLFWwindow* window = nullptr;

    bool enableFog = false;

    float musicVolume = 0.5f;
    float soundVolume = 0.5f;

    bool DEV_MODE;
    float gameVersionMajor;
    float gameVersionMinor;
    float gameVersionPatch;

    std::filesystem::path basePath;
    std::filesystem::path savePath;
    static Game* s_instance;
       
    std::unique_ptr<Shader> shaderProgram;
    std::unique_ptr<Texture> atlas;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};

#endif
