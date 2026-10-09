#include "cppsys.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace std;
// general idea for networking is to make a request then reply, assume reply is for latest request on client

// to start the server
class server {
private:
    int srvSocket;
    int cliSocket;
    string interface;

public:
    server()
    {
        // in order to get the rx and tx traffic we need the primary interface name
        ifstream route;
        route.open("/proc/net/route", std::ifstream::in);
        // only need the second line so assign both to same variable to overwrite
        getline(route, interface);
        getline(route, interface);
        // parse interface name from line
        interface = interface.substr(0, interface.find(" "));

        // create socket
        srvSocket = socket(AF_INET, SOCK_STREAM, 0);

        // specify socket address and port
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(7777);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        // bind socket
        bind(srvSocket, (struct sockaddr*)&serverAddress,
            sizeof(serverAddress));

        // listen for connection on socket
        listen(srvSocket, 5);

        // accept connection
        cliSocket = accept(srvSocket, nullptr, nullptr);
        std::cout << "connection recieved" << endl;
    };
    ~server()
    {
        // close socket
        close(srvSocket);
        std::cout << "closing socket" << endl;
    }
    int recieve()
    {
        uint8_t msg;
        recv(cliSocket, &msg, sizeof(msg), 0);
        std::cout << "recieving " << msg << endl;
        return msg;
    }
    void sendReply(uint32_t reply)
    {
        std::cout << "sending " << reply << endl;
        send(cliSocket, &reply, 32, 0);
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
        case 1: {
            // reply here with cpu usage
            // send as percentage by using an integer out of 100
            get_system_usage_linux::CPU_stats t1 = get_system_usage_linux::read_cpu_data();

            this_thread::sleep_for(std::chrono::milliseconds(1000));

            get_system_usage_linux::CPU_stats t2 = get_system_usage_linux::read_cpu_data();
            sendReply(get_system_usage_linux::get_cpu_usage(t1, t2));
            break;
        }

        case 2: {
            // reply here with memory usage
            // send as percentage by using an integer out of 100
            auto memory_data = get_system_usage_linux::read_memory_data();
            sendReply(memory_data.get_memory_usage());
            break;
        }

        case 3: {
            // reply here with disk usage
            // send as percentage by using an integer out of 100
            sendReply(get_system_usage_linux::get_disk_usage("/"));
            break;
        }

        case 4: {
            // reply here with swap usage
            // send as percentage by using integer out of 100
            auto memory_data = get_system_usage_linux::read_memory_data();
            sendReply(memory_data.get_swap_usage());

            break;
        }
        case 5: {
            // reply here with rx network traffic
            // send network traffic as bps
            // since this is for unix only read the route file to find default network interface

            ifstream rx;
            // read total rx bytes sent twice to get bytes sent per second
            rx.open("/sys/class/net/" + interface + "/statistics/rx_bytes");
            string temp;
            getline(rx, temp);
            int read1 = stoi(temp);
            this_thread::sleep_for(chrono::milliseconds(1000));
            rx.open("/sys/class/net/" + interface + "/statistics/rx_bytes");
            getline(rx, temp);
            int read2 = stoi(temp);
            // send diff between 2 reads to get bps
            sendReply(read2 - read1);
            break;
        }
        case 6: {
            ifstream tx;
            // read total tx bytes sent twice to get bytes sent per second
            tx.open("/sys/class/net/" + interface + "/statistics/tx_bytes");
            string temp;
            getline(tx, temp);
            int read1 = stoi(temp);
            this_thread::sleep_for(chrono::milliseconds(1000));
            tx.open("/sys/class/net/" + interface + "/statistics/tx_bytes");
            getline(tx, temp);
            int read2 = stoi(temp);
            // send diff between 2 reads to get bps
            sendReply(read2 - read1);
            break;
        }
        }
    }
};

int main()
{
    cout << "started" << endl;
    server srv;
    while (1 == 1) {
    }
}