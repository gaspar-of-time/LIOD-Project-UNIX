#include "cpptui.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

class client {
private:
    int srvSocket;
    int cliSocket;

public:
    client()
    {
        int cliSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(8080);
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }
}