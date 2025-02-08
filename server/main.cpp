#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <map>
#include <chrono>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 1024

std::map<SOCKET, std::string> clientSockets;
std::mutex clientSocketsMutex;
bool running = true;

void list_clients() {
    std::lock_guard<std::mutex> lock(clientSocketsMutex);
    std::cout << "Connected clients:" << std::endl;
    int index = 1;
    for (const auto& client : clientSockets) {
        std::cout << index++ << ": " << client.second << std::endl;
    }
}

void remove_disconnected_clients() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        for (auto it = clientSockets.begin(); it != clientSockets.end(); ) {
            char buffer;
            int result = recv(it->first, &buffer, 1, MSG_PEEK);
            if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)) {
                std::cout << "Client disconnected: " << it->second << std::endl;
                closesocket(it->first);
                it = clientSockets.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void handle_client(SOCKET ClientSocket) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    std::string command;

    while (true) {
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        if (clientSockets.find(ClientSocket) == clientSockets.end()) {
            std::cout << "Client disconnected, waiting for new connection..." << std::endl;
            break;
        }

        std::cout << "Enter command: ";
        std::getline(std::cin, command);
        if (command == "exit") {
            send(ClientSocket, command.c_str(), command.length(), 0);
            break;
        }
        else if (command == "list") {
            list_clients();
            continue;
        }
        else if (command.rfind("switch ", 0) == 0) {
            int clientIndex = std::stoi(command.substr(7)) - 1;
            if (clientIndex >= 0 && clientIndex < clientSockets.size()) {
                auto it = std::next(clientSockets.begin(), clientIndex);
                ClientSocket = it->first;
                std::cout << "Switched to client " << clientIndex + 1 << ": " << it->second << std::endl;
            }
            else {
                std::cout << "Invalid client index" << std::endl;
            }
            continue;
        }

        send(ClientSocket, command.c_str(), command.length(), 0);

        int iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            std::string output(recvbuf, iResult);
            std::cout << output << std::endl;
        }
    }
}

void add_client(SOCKET ClientSocket, sockaddr_in clientAddr) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    std::string clientInfo = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));
    {
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        clientSockets[ClientSocket] = clientInfo;
    }
    std::cout << "Client connected: " << clientInfo << std::endl;
}

int main() {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    struct addrinfo* result = nullptr, hints;
    const char* server_port = "55375";

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(nullptr, server_port, &hints, &result);

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    listen(ListenSocket, SOMAXCONN);

    std::thread clientHandler;
    std::thread disconnectionChecker(remove_disconnected_clients);
    disconnectionChecker.detach();

    while (running) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (ClientSocket != INVALID_SOCKET) {
            add_client(ClientSocket, clientAddr);
            if (!clientHandler.joinable()) {
                clientHandler = std::thread(handle_client, ClientSocket);
                clientHandler.detach();
            }
        }
    }

    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}