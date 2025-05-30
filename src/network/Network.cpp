#include <iostream>
#include <cstdio> 
#include <cstdlib>
#include <sstream>
#include <array>

#include "network/Network.h"

NetworkRole NetworkManager::currentRole = NetworkRole::CLIENT;

NetworkManager* NetworkManager::s_instance = nullptr;

NetworkManager::NetworkManager() {
    udpSocket = nullptr;
}

NetworkManager::~NetworkManager() {}

void NetworkManager::setInstance(NetworkManager* instance) {
    s_instance = instance;
}

NetworkManager& NetworkManager::instance() {
    return *s_instance;
}

void NetworkManager::setRole(NetworkRole role) {
    currentRole = role;
}

NetworkRole NetworkManager::getRole() {
    return currentRole;
}

void NetworkManager::setIP(const std::string& ip) {
    if (ip == "default") {
        std::string publicIP = getPublicIP();
        if (publicIP.empty()) {
            std::cerr << "Failed to get public IP. Defaulting to localhost." << std::endl;
            publicIP = "127.0.0.1";
        }
        instance().serverAddress.ip = publicIP;
    } else if (ip == "localhost") {
        instance().serverAddress.ip = "127.0.0.1";
    } else {
        instance().serverAddress.ip = ip;
    }
}

std::string NetworkManager::getIP() {
    return instance().serverAddress.ip;
}

void NetworkManager::setPort(uint16_t port) {
    instance().serverAddress.port = port;
}

uint16_t NetworkManager::getPort() {
    return instance().serverAddress.port;
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

bool NetworkManager::connectTCP() {
    tcpSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpSocket == INVALID_SOCKET) {
        std::cerr << "[Client] Failed to create TCP socket\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverAddress.port);
    serverAddr.sin_addr.s_addr = inet_addr(serverAddress.ip.c_str());

    if (::connect(tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[Client] Failed to connect to server over TCP\n";
#ifdef _WIN32
        closesocket(tcpSocket);
#else
        close(tcpSocket);
#endif
        tcpSocket = INVALID_SOCKET;
        return false;
    }
;
    return true;
}

SOCKET NetworkManager::getTCPSocket() const {
    return tcpSocket;
}

std::string NetworkManager::getPublicIP() {
    std::array<char, 128> buffer{};
    std::string result;

#ifdef _WIN32
    FILE* pipe = _popen("curl -s ifconfig.me", "r");
#else
    FILE* pipe = popen("curl -s ifconfig.me", "r");
#endif

    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    if (result.empty() || result.find('.') == std::string::npos) return "";
    return result;
}
