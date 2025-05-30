#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#endif

class TCPSocket {
public:
    TCPSocket();
    ~TCPSocket();

    bool bind(uint16_t port);
    bool listen();
    SOCKET acceptClient(sockaddr_in* outAddr = nullptr);
    bool connectTo(const std::string& ip, uint16_t port);

    static bool sendAll(SOCKET socket, const std::vector<uint8_t>& data);
    static bool recvAll(SOCKET socket, std::vector<uint8_t>& data, size_t length);

    SOCKET getHandle() const;

private:
    SOCKET socketHandle;
};

#endif
