#include "cpptui.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
using namespace std;

class client {
private:
    int srvSocket;
    int cliSocket;

public:
    client(string ip)
    {
        // create socket
        int cliSocket = socket(AF_INET, SOCK_STREAM, 0);

        // specify address
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(7777);
        // convert ip string into machine readable address and store in the server's s_addr
        inet_pton(AF_INET, ip.data(), &serverAddress.sin_addr.s_addr);

        // connect to the server
        srvSocket = connect(cliSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }
    ~client()
    {
        // close socket
        close(cliSocket);
    }
    int recieve()
    {
        // make var to store the response
        uint32_t msg;
        // recieve the response and store it in the var
        recv(srvSocket, &msg, sizeof(msg), 0);
        return msg;
    }
    void sendMsg(uint8_t out)
    {
        send(cliSocket, &out, sizeof(out), 0);
    }
    int request(uint8_t msg)
    {
        sendMsg(msg);
        int reply = recieve();
        if (msg <= 4) {
            return reply / 100;
        } else {
            return reply;
        }
    }
};
