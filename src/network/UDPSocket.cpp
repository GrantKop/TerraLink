#include "network/UDPSocket.h"
#include <iostream>
#include <cstring>

UDPSocket::UDPSocket() {
    socketHandle = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketHandle == INVALID_SOCKET) {
        std::cerr << "Failed to create UDP socket\n";
    }
}

UDPSocket::~UDPSocket() {
#ifdef _WIN32
    closesocket(socketHandle);
#else
    close(socketHandle);
#endif
}

bool UDPSocket::bind(uint16_t port) {
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    return ::bind(socketHandle, (sockaddr*)&localAddr, sizeof(localAddr)) != SOCKET_ERROR;
}

bool UDPSocket::sendTo(const std::vector<uint8_t>& data, const Address& addr) {
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(addr.port);
    dest.sin_addr.s_addr = inet_addr(addr.ip.c_str());

    int result = ::sendto(socketHandle, reinterpret_cast<const char*>(data.data()), data.size(), 0,
                          (sockaddr*)&dest, sizeof(dest));
    return result != SOCKET_ERROR;
}

bool UDPSocket::receiveFrom(std::vector<uint8_t>& data, Address& fromAddr) {
    data.resize(65536);

    sockaddr_in sender{};
    socklen_t senderLen = sizeof(sender);
    int bytesRead = ::recvfrom(socketHandle, reinterpret_cast<char*>(data.data()), data.size(), 0,
                               (sockaddr*)&sender, &senderLen);

    if (bytesRead == SOCKET_ERROR) return false;

    data.resize(bytesRead);
    fromAddr.ip = inet_ntoa(sender.sin_addr);
    fromAddr.port = ntohs(sender.sin_port);
    return true;
}

bool initSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void shutdownSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}
