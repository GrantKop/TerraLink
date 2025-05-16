#include "network/Network.h"

NetworkRole NetworkManager::currentRole = NetworkRole::CLIENT;

void NetworkManager::setRole(NetworkRole role) {
    currentRole = role;
}

NetworkRole NetworkManager::getRole() {
    return currentRole;
}

bool NetworkManager::isServer() {
    return currentRole == NetworkRole::SERVER;
}

bool NetworkManager::isClient() {
    return currentRole == NetworkRole::CLIENT;
}

bool NetworkManager::isHost() {
    return currentRole == NetworkRole::HOST;
}
