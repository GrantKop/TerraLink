#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include <vector>
#include <cstdint>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
#endif

#include "network/Network.h"
#include "network/Address.h"

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    bool bind(uint16_t port);
    bool sendTo(const std::vector<uint8_t>& data, const Address& addr);
    bool receiveFrom(std::vector<uint8_t>& data, Address& fromAddr);

private:
    SOCKET socketHandle;
};

bool initSockets();
void shutdownSockets();

#endif
