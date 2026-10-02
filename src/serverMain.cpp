#include "cppsys.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

// to start the server
class server {
private:
    int srvSocket;
    int cliSocket;
    string interface;

public:
    server()
    {
        // create socket
        srvSocket = socket(AF_INET, SOCK_STREAM, 0);

        // specify socket address and port
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(7777);
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        // listen for connection on socket
        listen(srvSocket, 5);

        // accept connection
        cliSocket = accept(srvSocket, nullptr, nullptr);
    };
    ~server()
    {
        // close socket
        close(srvSocket);
    }
    int recieve()
    {
        uint8_t msg;
        recv(cliSocket, msg, sizeof(msg), 0);
        return msg;
    }
    void send(uint32_t reply)
    {
        // put sending business here
    }

    // use message 1 as request for cpu
    // message 2 as request for memory
    // message 3 disk usage
    // message 4 swap usage
    // message 5 network traffic rx
    // message 6 network traffic tx
    void reply(int msg)
    {
        switch (msg) {
        case 1:
            // reply here with cpu usage
            // send as percentage by using an integer out of 100
            get_system_usage_linux::CPU_stats t1 = read_cpu_data();

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            get_system_usage_linux::CPU_stats t2 = read_cpu_data();
            send(get_system_usage_linux::get_cpu_usage(t1, t2));
            break;

        case 2:
            // reply here with memory usage
            // send as percentage by using an integer out of 100
            auto memory_data = read_memory_data();
            send(memory_data.get_memory_usage());
            break;

        case 3:
            // reply here with disk usage
            // send as percentage by using an integer out of 100
            send(get_system_usage_linux::get_disk_usage("/")) break;

        case 4:
            // reply here with swap usage
            auto memory_data = read_memory_data();
            send(memory_data.get_swap_usage());

            break;

        case 5:
            // reply here with rx network traffic
            // send network traffic as bps
            // since this is for unix only read the route file to find default network interface
            ifstream route
                route.open("/proc/net/route", std::ifstream::in);
            getline(route, NULL);
            string iface;
            getline(route, iface);
            // parse interface name from line
            iface = iface.substr(0, iface.find(" "));
            ifstream tx;

            send();

            break;
        case 6:
        }
    }
};

int main()
{
    server srv;
}