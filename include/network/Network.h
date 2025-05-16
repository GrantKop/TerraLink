#ifndef NETWORK_H
#define NETWORK_H

#include <cstdint>
#include <string>

#include "network/UDPSocket.h"
#include "network/Message.h"
#include "network/NetworkRole.h"

class NetworkManager {
public:
    static void setRole(NetworkRole role);
    static NetworkRole getRole();

    static bool isServer();
    static bool isClient();
    static bool isHost();

private:
    static NetworkRole currentRole;
};


#endif
