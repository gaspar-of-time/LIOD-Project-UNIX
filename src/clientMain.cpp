#include "cpptui.hpp"
#include "httplib.h"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;
struct stats {
    float cpuPer;
    unsigned long memoryUsed;
    unsigned long memoryTotal;
    float memoryPer;
    float swapPer;
    float storagePer;
    unsigned long rxBps;
    unsigned long txBps;
};
stats getStats(string address)
{
    httplib::Client cli(address);
    float cpuPer = 100.0f * stof(cli.Get("/cpu")->body);
    unsigned long memoryUsed = stoll(cli.Get("/memoryUsed")->body);
    unsigned long memoryTotal = stoll(cli.Get("/memoryTotal")->body);
    float memoryPer = (100.0f * memoryUsed) / memoryTotal;
    float swapPer = stof(cli.Get("/swap")->body);
    float storagePer = stof(cli.Get("/storage")->body);
    cout << cli.Get("/rx")->body << "temp" << endl;
    // currently tx and rx don't work, set to 0 for testing purposes
    // unsigned long rxBps = stoll(cli.Get("/rx")->body);
    // unsigned long txBps = stoll(cli.Get("/tx")->body);
    int txBps = 0;
    int rxBps = 0;
    return { cpuPer, memoryUsed, memoryTotal, memoryPer, swapPer, storagePer, rxBps, txBps };
}

int main()
{
    // this is all test code
    string ip = "localhost:7777";
    // cin >> ip;
    stats stats = getStats(ip);
    cout << "CPU%: " << stats.cpuPer << endl;
    cout << "Memory Used (Kb): " << stats.memoryUsed << endl;
    cout << "Memory Total (Kb): " << stats.memoryTotal << endl;
    cout << "Memory%: " << stats.memoryPer << endl;
    cout << "Swap%: " << stats.swapPer << endl;
    cout << "Storage%: " << stats.storagePer << endl;
    cout << "RX (bps): " << stats.rxBps << endl;
    cout << "TX (bps): " << stats.txBps << endl;
}