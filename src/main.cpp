#include <iostream>
#include <atomic>

#include "graphics/VertexArrayObject.h"
#include "core/registers/AtlasRegister.h"
#include "core/game/Game.h"
#include "network/Network.h"
#include "core/game/GameInit.h"
#include "network/Server.h"
#include "network/Serializer.h"

bool DEV_MODE = true;
float gameVersionMajor = 0.f;
float gameVersionMinor = 5.f;
float gameVersionPatch = 8.f;

bool waitForServerConnection(UDPSocket& socket, const Address& serverAddr, float timeoutSeconds = 5.0f) {
    Message connect;
    connect.type = MessageType::ClientConnect;

    socket.sendTo(connect.serialize(), serverAddr);

    double start = glfwGetTime();
    std::vector<uint8_t> buffer;
    Address from;

    while (glfwGetTime() - start < timeoutSeconds) {
        if (socket.receiveFrom(buffer, from)) {
            try {
                Message msg = Message::deserialize(buffer);
                if (msg.type == MessageType::ClientConnectAck) {
                    return true;
                }
            } catch (...) {
                std::cerr << "[Client] [UDP] Failed to parse server response\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cerr << "[Client] [UDP] Timed out attempting UDP handshake.\n";
    return false;
}

int main() {

    std::filesystem::path basePath = !DEV_MODE
        ? std::filesystem::current_path().parent_path()
        : std::filesystem::current_path().parent_path().parent_path();

    initGLFW(3, 3);

    if (!initSockets()) {
        std::cerr << "Failed to initialize sockets!" << std::endl;
        return -1;
    }

    NetworkManager networkManager;
    NetworkManager::setInstance(&networkManager);

    bool onlineMode = GameInit::parseNetworkSettings((basePath / "network.settings").string());

    if (NetworkManager::getRole() == NetworkRole::SERVER) {
        if (!NetworkManager::instance().isOnlineMode()) {
            std::cerr << "Server mode requires online mode to be enabled in network.settings.\n";
            shutdownSockets();
            return -1;
        }
        Server server(NetworkManager::getPort());
        server.run();
        shutdownSockets();
        return 0;
    }

    GLFWwindow* window = nullptr;
    if (!createWindow(window, "TerraLink", 800, 800, !DEV_MODE)) {
        glfwTerminate();
        shutdownSockets();
        return -1;
    }

    Game game(window, DEV_MODE);
    Game::setInstance(&game);
    game.setGameVersion(gameVersionMajor, gameVersionMinor, gameVersionPatch);

    if (!onlineMode) {
        shutdownSockets();
        game.init();
        game.gameLoop();
        return 0;
    }

    switch (NetworkManager::getRole()) {
        case NetworkRole::CLIENT: {
            NetworkManager::instance().setUDPSocket();

            if (!NetworkManager::instance().connectTCP()) {
                shutdownSockets();
                return -1;
            }

            Message playerNameMsg;
            playerNameMsg.type = MessageType::ClientInfo;
            Serializer::writeString(playerNameMsg.data, GameInit::getPlayerName((basePath / "game.settings").string()));

            std::vector<uint8_t> serialized = playerNameMsg.serialize();
            uint32_t len = static_cast<uint32_t>(serialized.size());
            std::vector<uint8_t> lengthPrefix(4);
            std::memcpy(lengthPrefix.data(), &len, 4);


            TCPSocket::sendAll(NetworkManager::instance().getTCPSocket(), lengthPrefix);
            TCPSocket::sendAll(NetworkManager::instance().getTCPSocket(), serialized);

            std::vector<uint8_t> lengthBuf;
            while (!TCPSocket::recvAll(NetworkManager::instance().getTCPSocket(), lengthBuf, 4)) {
                uint32_t msgLength;
                std::memcpy(&msgLength, lengthBuf.data(), 4);
                std::vector<uint8_t> data;
                TCPSocket::recvAll(NetworkManager::instance().getTCPSocket(), data, len);
            }

            if (!waitForServerConnection(UDPSocket::instance(), NetworkManager::instance().getAddress())) {
                shutdownSockets();
                return -1;
            }

            std::cout << "[Client] Connected to server at " << NetworkManager::getIP() << ":" << NetworkManager::getPort() << "\n";
        
            game.init();
            game.gameLoop();
            break;
        }
        case NetworkRole::HOST: {
            std::cout << "[Host] Running local server + client\n";

            Server* server = new Server(NetworkManager::getPort());
            std::thread serverThread([&]() {
                server->run();
            });

            game.init();
            game.gameLoop();

            serverThread.detach();
            delete server;
            break;
        }
        default: {
            std::cerr << "Invalid network role specified in network.settings. Defaulting to CLIENT.\n";
            game.init();
            game.gameLoop();
            break;
        }
    }

    shutdownSockets();
    return 0;
}
