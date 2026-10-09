#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cppsys.hpp"
#include "httplib.h"
#include <chrono>
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
    iface = iface.substr(0, iface.find(" "));
    return iface;
}

int getTx()
{
    ifstream tx;
    string interface = getIface();
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
    return read2 - read1;
}
int getRx()
{
    ifstream rx;
    string interface = getIface();
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
    return read2 - read1;
}
float getSwap()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    return (100.0f * memory_data.get_swap_usage());
}

float getStor()
{
    return (100.0f * get_system_usage_linux::get_disk_usage("/"));
}
float getMem()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    return 100.0f * memory_data.get_memory_usage();
}
float getCpu()
{
    get_system_usage_linux::CPU_stats t1 = get_system_usage_linux::read_cpu_data();

    this_thread::sleep_for(std::chrono::milliseconds(1000));

    get_system_usage_linux::CPU_stats t2 = get_system_usage_linux::read_cpu_data();
    return get_system_usage_linux::get_cpu_usage(t1, t2);
}
int getTotalMem()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    return memory_data.total_memory;
}
int getUsedMem()
{
    auto memory_data = get_system_usage_linux::read_memory_data();
    return memory_data.total_memory - memory_data.available_memory;
}

int main()
{
    httplib::Server srv;
    while (true) {
        // add mem max and mem curr
        // get from total and available-total
        srv.Get("/cpu", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getCpu()), "text/plain"); });
        srv.Get("/memory", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getMem()), "text/plain"); });
        srv.Get("/swap", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getSwap()), "text/plain"); });
        srv.Get("/storage", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getMem()), "text/plain"); });
        srv.Get("/rx", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getRx()), "text/plain"); });
        srv.Get("/tx", [](const httplib::Request&, httplib::Response& res) { res.set_content(to_string(getTx()), "text/plain"); });
        this_thread::sleep_for(chrono::seconds(5));
    }
}