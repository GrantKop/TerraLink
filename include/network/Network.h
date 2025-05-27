#ifndef NETWORK_H
#define NETWORK_H

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <cstdint>
#include <string>

#include "network/Message.h"
#include "network/NetworkRole.h"
#include "network/Address.h" 

// class UDPSocket;

class NetworkManager {
public:

    NetworkManager();
    ~NetworkManager();

    static void setInstance(NetworkManager* instance);
    static NetworkManager& instance();

    static void setRole(NetworkRole role);
    static NetworkRole getRole();

    static void setIP(const std::string& ip);
    static std::string getIP();

    static void setPort(uint16_t port);
    static uint16_t getPort();

    static bool isServer();
    static bool isClient();
    static bool isHost();

    bool connectTCP();
    SOCKET getTCPSocket() const;

    static std::string getPublicIP();

    static bool isOnlineMode() {
        return instance().onlineMode;
    }
    static void setOnlineMode(bool mode) {
        instance().onlineMode = mode;
    }

private:
    static NetworkRole currentRole;

    // UDPSocket* udpSocket;
    Address serverAddress;
    bool onlineMode = false;

    SOCKET tcpSocket = INVALID_SOCKET;

    static NetworkManager* s_instance;
};

#endif
