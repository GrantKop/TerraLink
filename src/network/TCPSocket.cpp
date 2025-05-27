#include "network/TCPSocket.h"
#include <cstring>
#include <iostream>

TCPSocket::TCPSocket() {
    socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle == INVALID_SOCKET) {
        std::cerr << "Failed to create TCP socket\n";
    }
}

TCPSocket::~TCPSocket() {
#ifdef _WIN32
    closesocket(socketHandle);
#else
    close(socketHandle);
#endif
}

bool TCPSocket::bind(uint16_t port) {
    socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return ::bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

bool TCPSocket::listen() {
    return ::listen(socketHandle, SOMAXCONN) != SOCKET_ERROR;
}

SOCKET TCPSocket::acceptClient() {
    return ::accept(socketHandle, nullptr, nullptr);
}

bool TCPSocket::connectTo(const std::string& ip, uint16_t port) {
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());

    return ::connect(socketHandle, (sockaddr*)&server, sizeof(server)) != SOCKET_ERROR;
}

bool TCPSocket::sendAll(SOCKET socket, const std::vector<uint8_t>& data) {
    size_t total = 0;
    while (total < data.size()) {
        int sent = send(socket, reinterpret_cast<const char*>(data.data() + total), data.size() - total, 0);
        if (sent <= 0) return false;
        total += sent;
    }
    return true;
}

bool TCPSocket::recvAll(SOCKET socket, std::vector<uint8_t>& out, size_t size) {
    out.resize(size);
    size_t total = 0;
    while (total < size) {
        int received = recv(socket, reinterpret_cast<char*>(out.data() + total), size - total, 0);
        if (received <= 0) return false;
        total += received;
    }
    return true;
}

SOCKET TCPSocket::getHandle() const {
    return socketHandle;
}
