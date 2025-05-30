#ifndef GAME_INIT_H
#define GAME_INIT_H

#include <iostream>
#include <fstream>
#include <string>

namespace GameInit {
    void parseGameSettings(const std::string& filePath);

    std::string getPlayerName(const std::string& filePath);

    bool parseNetworkSettings(const std::string& filePath);
}

#endif
