#include "cppsys.hpp"
#include "httplib.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

using namespace std;
string getIface()
{
    ifstream route;
    string iface;
    route.open("/proc/net/route", std::ifstream::in);
    // only need the second line so assign both to same variable to overwrite
    getline(route, iface);
    getline(route, iface);
    // parse interface name from line
    iface = iface.substr(0, iface.find("\t"));
    return iface;
}

int getTx()
{
    ifstream tx;
    string interface = getIface();
    string txDir = "/sys/class/net/" + interface + "/statistics/tx_bytes";
    // read total tx bytes sent twice to get bytes sent per second
    tx.open(txDir);
    string temp;
    getline(tx, temp);
    int read1 = stoi(temp);
    this_thread::sleep_for(chrono::milliseconds(1000));
    tx.open(txDir);
    getline(tx, temp);
    int read2 = stoi(temp);
    // send diff between 2 reads to get bps
    return read2 - read1;
}
int getRx()
{
    ifstream rx;
    string interface = getIface();
    string rxDir = "/sys/class/net/" + interface + "/statistics/rx_bytes";
    cout << rxDir << endl;
    // read total rx bytes sent twice to get bytes sent per second
    rx.open(rxDir);
    string temp;
    getline(rx, temp);
    int read1 = stoi(temp);
    cout << read1 << "read1" << endl;
    rx.close();
    temp = "";
    // this_thread::sleep_for(std::chrono::milliseconds(1000));
    sleep(1);
    rx.open(rxDir);
    getline(rx, temp);
    int read2 = stoi(temp);
    // send diff between 2 reads to get bps
    cout << read2 << "read2" << endl;
    cout << read1 << "read1" << endl;
    return read2 - read1;
}
float getSwap()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    cout << "answering swap usage call" << endl;
    return (100.0f * memory_data.get_swap_usage());
}

float getStor()
{
    cout << "answering storage usage call" << endl;
    return (100.0f * get_system_usage_linux::get_disk_usage("/"));
}
float getCpu()
{
    get_system_usage_linux::CPU_stats t1 = get_system_usage_linux::read_cpu_data();

    this_thread::sleep_for(std::chrono::milliseconds(1000));

    get_system_usage_linux::CPU_stats t2 = get_system_usage_linux::read_cpu_data();
    cout << "answering cpu usage call" << endl;
    return get_system_usage_linux::get_cpu_usage(t1, t2);
}
int getTotalMem()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    cout << "answering total memory call" << endl;
    return memory_data.total_memory;
}
int getUsedMem()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    cout << "answering used memory call" << endl;
    return memory_data.total_memory - memory_data.available_memory;
}

int main()
{
    httplib::Server srv;

    srv.Get("/cpu", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getCpu()), "text/plain"); });
    srv.Get("/memoryTotal", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getTotalMem()), "text/plain"); });
    srv.Get("/memoryUsed", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getUsedMem()), "text/plain"); });
    srv.Get("/swap", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getSwap()), "text/plain"); });
    srv.Get("/storage", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getStor()), "text/plain"); });
    srv.Get("/rx", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getRx()), "text/plain"); });
    srv.Get("/tx", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getTx()), "text/plain"); });

    cout << "listening for api calls" << endl;
    srv.listen("0.0.0.0", 7777);
}