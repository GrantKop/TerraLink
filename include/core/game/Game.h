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
#include "graphics/TextRenderer.h"

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

    void renderUI();
    void renderBlockOutline();

    // First-person "held block": the selected (to-be-placed) block drawn as a
    // small 3D cube in the bottom-right corner. See docs/selected-block-hud.md.
    void renderHeldBlock();

    // Selected-block name HUD (see docs/selected-block-hud.md).
    // updateSelectedBlockHUD does the block-name *lookup* and runs the fade
    // timer; renderSelectedBlockHUD only draws the resolved string.
    void updateSelectedBlockHUD(float deltaTime);
    void renderSelectedBlockHUD(const glm::mat4& projection, int winW, int winH);

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
    std::unique_ptr<Shader> uiShaderProgram;
    std::unique_ptr<Shader> wireFrameShaderProgram;
    std::unique_ptr<Texture> atlas;
    std::unique_ptr<Texture> crosshairTex;
    std::unique_ptr<VertexArrayObject> crosshairVAO;
    std::unique_ptr<VertexArrayObject> wireFrameVAO;

    std::unique_ptr<TextRenderer> textRenderer;

    // Held-block view model. Rebuilt only when the selection changes.
    std::unique_ptr<VertexArrayObject> heldBlockVAO;
    int heldBlockID = -1;
    int heldBlockIndexCount = 0;
    void buildHeldBlockMesh(int blockID);

    // Selected-block HUD state. The label is shown for HUD_HOLD_TIME seconds
    // at full opacity, then fades to nothing over the next
    // (HUD_FADE_TIME - HUD_HOLD_TIME) seconds, and stays hidden until the
    // player selects a different block.
    static constexpr float HUD_HOLD_TIME = 2.0f;
    static constexpr float HUD_FADE_TIME = 3.0f;
    std::string hudBlockName;
    float hudLabelTimer = HUD_FADE_TIME; // start fully expired (nothing shown)
    int hudLastSelectedBlockID = -1;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};

#endif
