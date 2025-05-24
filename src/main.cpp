#include <iostream>

#include "graphics/VertexArrayObject.h"
#include "core/registers/AtlasRegister.h"
#include "core/game/Game.h"
#include "network/Network.h"
#include "core/game/GameInit.h"

int main() {

    initGLFW(3, 3);

    GLFWwindow* window = nullptr;
    if (!createWindow(window, "TerraLink", 800, 600)) {
        glfwTerminate();
        return -1;
    }

    NetworkManager networkManager;
    NetworkManager::setInstance(&networkManager);

    Game game(window);
    Game::setInstance(&game);

    bool onlineMode = GameInit::parseNetworkSettings((game.getBasePath() + "/network.settings").c_str());

    if (onlineMode && NetworkManager::instance().isServer()) {

        return 0;
    }

    game.init();
    game.gameLoop();

    return 0;
}
