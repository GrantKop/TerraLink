#include "core/game/GameInit.h"

#include "core/game/Game.h"
#include "core/player/Player.h"
#include "network/Network.h"

namespace GameInit {
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

            if (key == "renderDistance") {
                int renderDistance = std::stoi(value);
                if (renderDistance < 1) renderDistance = 1;
                if (renderDistance > 64) renderDistance = 64;
                Player::instance().syncViewDistance(renderDistance);
            } else if (key == "fov") {
                float fov = std::stof(value);
                if (fov < 40.0f) fov = 40.0f;
                if (fov > 120.0f) fov = 120.0f;
                Player::instance().getCamera().setBaseFOV(fov);
            } else if (key == "sensitivity") {
                Player::instance().getCamera().setSensitivity(std::stof(value));
            } else if (key == "playerName") {
                Player::instance().setPlayerName(value);
            } else if (key == "saveName") {
                World::instance().setSaveDirectory(value);
                Game::instance().setWorldSave(value);
            } else if (key == "seed") {
                World::instance().setSeed(std::stoi(value));
            } else if (key == "distanceFog") {
                Game::instance().setEnableFog(value == "true" || value == "1" || value == "True" || value == "TRUE");
            }
        }
    }

    bool parseNetworkSettings(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open network settings file: " << filePath << std::endl;
            return false;
        }

        bool onlineMode = false;

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

            if (key == "mode") {
                if (value == "server" || value == "Server" || value == "SERVER") {
                    NetworkManager::setRole(NetworkRole::SERVER);
                } else if (value == "client" || value == "Client" || value == "CLIENT") {
                    NetworkManager::setRole(NetworkRole::CLIENT);
                } else if (value == "host" || value == "Host" || value == "HOST") {
                    NetworkManager::setRole(NetworkRole::HOST);
                } else {
                    std::cerr << "Invalid network role: " << value << std::endl;
                    std::cerr << "Defaulting to CLIENT." << std::endl;
                    NetworkManager::setRole(NetworkRole::CLIENT);
                }
            } else if (key == "ip") {
                NetworkManager::setIP(value);
            } else if (key == "port") {
                NetworkManager::setPort(std::stoi(value));
            } else if (key == "onlineMode") {
                onlineMode = (value == "true" || value == "1" || value == "True" || value == "TRUE");
            }
        }

        return onlineMode;
    }
}