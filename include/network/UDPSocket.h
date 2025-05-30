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

#include "network/Address.h"

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    static void setInstance(UDPSocket* instance);
    static UDPSocket& instance();

    void setIP(const std::string& ip);
    void setPort(uint16_t port);
    std::string getIP() const;
    uint16_t getPort() const;

    Address getAddress() const {
        return { ip, port };
    }

    bool bind(uint16_t port);
    bool sendTo(const std::vector<uint8_t>& data, const Address& addr);
    bool receiveFrom(std::vector<uint8_t>& data, Address& fromAddr);

    void close();

private:
    SOCKET socketHandle;

    Address address;

    static UDPSocket* s_instance;

    std::string ip;
    uint16_t port;
};

bool initSockets();
void shutdownSockets();

#endif
