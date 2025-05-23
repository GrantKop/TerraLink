#ifndef NETWORK_H
#define NETWORK_H

#include <cstdint>
#include <string>

#include "network/Message.h"
#include "network/NetworkRole.h"

class UDPSocket;

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

private:
    static NetworkRole currentRole;

    static UDPSocket udpSocket;

    static NetworkManager* s_instance;
};


#endif
