#ifndef GAME_H
#define GAME_H

#include <memory>
#include <string>
#include <filesystem>

class World;

class Game {
public:
    Game();
    ~Game();

    static void setInstance(Game* instance);
    static Game& instance();

    std::string getBasePath() const;
    std::string getSavePath() const;

    void init();
    void tick();
    void shutdown();

    void setWorldSave(const std::string& saveName);
    std::string getWorldSave() const;

    World& getWorld();

    void setEnableFog(bool enable) { enableFog = enable; }
    bool isFogEnabled() const { return enableFog; }

    std::string getGameVersion() const;

    void setReleaseMode(bool releaseMode) { DEV_MODE = !releaseMode; }
    bool isReleaseMode() const { return !DEV_MODE; }

private:
    std::string curWorldSave;
    std::unique_ptr<World> world;

    bool enableFog = false;

    bool DEV_MODE = true;
    float gameVersionMajor = 0.f;
    float gameVersionMinor = 4.f;
    float gameVersionPatch = 2.f;

    std::filesystem::path basePath = DEV_MODE
        ? std::filesystem::current_path().parent_path().parent_path()
        : std::filesystem::current_path().parent_path();
    std::filesystem::path savePath = basePath / "saves/";
    static Game* s_instance;
};

#endif
