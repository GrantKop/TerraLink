#include "network/Network.h"

#include "network/UDPSocket.h"

NetworkRole NetworkManager::currentRole = NetworkRole::CLIENT;

void NetworkManager::setRole(NetworkRole role) {
    currentRole = role;
}

NetworkRole NetworkManager::getRole() {
    return currentRole;
}

void NetworkManager::setIP(const std::string& ip) {
    udpSocket.setIP(ip);
}

std::string NetworkManager::getIP() {
    return udpSocket.getIP();
}

void NetworkManager::setPort(uint16_t port) {
    udpSocket.setPort(port);
}

uint16_t NetworkManager::getPort() {
    return udpSocket.getPort();
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
