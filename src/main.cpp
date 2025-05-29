#include <iostream>
#include <atomic>

#include "graphics/VertexArrayObject.h"
#include "core/registers/AtlasRegister.h"
#include "core/game/Game.h"
#include "network/Network.h"
#include "core/game/GameInit.h"
#include "network/Server.h"

bool DEV_MODE = true;
float gameVersionMajor = 0.f;
float gameVersionMinor = 5.f;
float gameVersionPatch = 3.f;

bool waitForServerConnection(UDPSocket& socket, const Address& serverAddr, float timeoutSeconds = 5.0f) {
    Message ping;
    ping.type = MessageType::PingPong;

    socket.sendTo(ping.serialize(), serverAddr);

    double start = glfwGetTime();
    std::vector<uint8_t> buffer;
    Address from;

    while (glfwGetTime() - start < timeoutSeconds) {
        if (socket.receiveFrom(buffer, from)) {
            try {
                Message msg = Message::deserialize(buffer);
                if (msg.type == MessageType::PingPong) {
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
        Server server(NetworkManager::getPort());
        server.run();
        shutdownSockets();
        return 0;
    }

    GLFWwindow* window = nullptr;
    if (!createWindow(window, "TerraLink", 800, 600, !DEV_MODE)) {
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
            TCPSocket tcpSocket;

            if (!tcpSocket.connectTo(NetworkManager::getIP(), NetworkManager::getPort())) {
                std::cerr << "[Client] [TCP] Failed to connect to server via TCP\n";
                shutdownSockets();
                return -1;
            }

            if (!waitForServerConnection(UDPSocket::instance(), NetworkManager::instance().getAddress())) {
                shutdownSockets();
                return -1;
            }
        
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
