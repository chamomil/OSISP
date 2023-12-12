#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#include <ws2tcpip.h>

class ChatRoom {
public:
    void join(SOCKET clientSocket, const std::string& username)
    {
        clients.push_back({ clientSocket, username });
        broadcast(username + " has joined the room.\n");
    }

    void leave(SOCKET clientSocket, const std::string& username)
    {
        auto it = std::find_if(clients.begin(), clients.end(), [clientSocket](const ClientInfo& info) {
            return info.socket == clientSocket;
        });

        if (it != clients.end()) {
            broadcast(username + " has left the room.\n");
            clients.erase(it);
        }
    }

    void broadcast(const std::string& message, SOCKET excludeClient = INVALID_SOCKET)
    {
        for (const auto& client : clients) {
            if (client.socket != excludeClient) {
                send(client.socket, message.c_str(), message.size(), 0); // sends data on a connected socket socket descriptor, buffer, length of the buffer, flags
            }
        }
    }

private:
    struct ClientInfo {
        SOCKET socket;
        std::string username;
    };

    std::vector<ClientInfo> clients;
};

ChatRoom globalRoom;

DWORD WINAPI ClientThread(LPVOID lpParam)
{
    SOCKET clientSocket = reinterpret_cast<SOCKET>(lpParam);
    char buffer[1024];
    int recvResult;

    recvResult = recv(clientSocket, buffer, sizeof(buffer), 0); // returns the number of bytes received
    //socket descriptor, pointer to buffer, len of buffer, flags
    std::string username;
    if (recvResult > 0) {
        buffer[recvResult] = '\0';
        username = std::string(buffer);

        globalRoom.join(clientSocket, username);
    }

    while (true) {
        recvResult = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvResult > 0) {
            buffer[recvResult] = '\0';
            std::string message(buffer);

            globalRoom.broadcast(username + ": " + message);
        } else if (recvResult == 0) {
            globalRoom.leave(clientSocket, username);
            closesocket(clientSocket);
            std::cout << "Client disconnected\n";
            break;
        } else {
            std::cerr << "Recv failed\n";
            globalRoom.leave(clientSocket, username);
            closesocket(clientSocket);
            break;
        }
    }

    return 0;
}

int main()
{
    WSADATA wsaData; // information about the Windows Sockets implementation
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { // initiates use of the Winsock DLL by a process like a load library
        //version and wsadata
        std::cerr << "WSAStartup failed\n";
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // af_inet - ipv4 (adress family specification), sock_stream - for stream socket
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr; //specifies a transport address and port for the AF_INET address family
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888); // converts port to big-endian
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) { // bind local address with a socket
        // socket descriptor, pointer to sockaddr, length of sockaddr
        std::cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) { //somaxconn - max
        std::cerr << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server is listening on port 8888...\n";


    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            break;
        }

        std::cout << "Client connected\n";

        HANDLE threadHandle = CreateThread(nullptr, 0, ClientThread, reinterpret_cast<LPVOID>(clientSocket), 0, nullptr);
        //pointer to a SECURITY_ATTRIBUTES (null - cannot be inherited), size (0 - default value is used), function, pointer to params to pass, flags (0 - runs immidiately)
        //pointer to a variable that recieves thread identifier
        if (threadHandle == nullptr) {
            std::cerr << "Failed to create thread\n";
            closesocket(clientSocket);
            break;
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
