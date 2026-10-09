#include "cpptui.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
using namespace std;

class Client {
private:
    int srvSocket;
    int cliSocket;
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
        // sending message using out
        send(cliSocket, &out, sizeof(out), 0);
    }

public:
    Client(string ip)
    {
        // create socket
        int cliSocket = socket(AF_INET, SOCK_STREAM, 0);

        // specify address
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(7778);
        // convert localhost into ip version
        if (ip == "localhost") {
            ip = "127.0.0.1";
        }
        // convert ip string into machine readable address and store in the server's s_addr
        inet_pton(AF_INET, ip.data(), &(serverAddress.sin_addr.s_addr));

        // connect to the server
        srvSocket = connect(cliSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }
    ~Client()
    {
        // close socket
        close(cliSocket);
    }

    int request(uint8_t msg)
    {
        // the reply after the request is assumed to be the response, this needs to be tested.
        //
        sendMsg(msg);
        return recieve();
    }
};
int main()
{
    // this is all test code
    string ip;
    std::cout << "Insert Server IP Address:" << std::endl;
    std::cin >> ip;
    Client client(ip);
    // use message 1 as request for cpu%
    // message 2 as request for memory%
    // message 3 disk usage%
    // message 4 swap usage%
    // message 5 network traffic rx bits/s
    // message 6 network traffic tx bits/s
    cout << "sending \%cpu request" << endl;
    cout << client.request(1) << endl;
    cout << "sending \%mem request" << endl;
    cout << client.request(2) << endl;
    cout << "sending \%disk request" << endl;
    cout << client.request(3) << endl;
    cout << "sending \%swap request" << endl;
    cout << client.request(4) << endl;
    cout << "sending rx request" << endl;
    cout << client.request(5) << endl;
    cout << "sending tx request" << endl;
    cout << client.request(6) << endl;
}