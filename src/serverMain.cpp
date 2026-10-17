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
    // second line of route is the default network interface
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
    // piggybacking off of linux drivers to get bytes upload
    ifstream tx;
    string interface = getIface();
    string txDir = "/sys/class/net/" + interface + "/statistics/tx_bytes";
    // read total tx bytes sent twice to get bytes sent per second
    tx.open(txDir);
    string temp;
    getline(tx, temp);
    // using long longs because these numbers can get too long for integers and these should be large enough
    unsigned long long read1 = stoll(temp);
    cout << "read1" << read1 << endl;
    this_thread::sleep_for(chrono::milliseconds(1000));
    // close the file to read the updated value
    tx.close();
    tx.open(txDir);
    getline(tx, temp);
    unsigned long long read2 = stoull(temp);
    cout << "read2" << read2 << endl;
    // send diff between 2 reads to get bps
    return read2 - read1;
}
int getRx()
{
    // same as tx but with download
    ifstream rx;
    string interface = getIface();
    string rxDir = "/sys/class/net/" + interface + "/statistics/rx_bytes";
    rx.open(rxDir);
    string temp;
    getline(rx, temp);
    cout << temp << endl;
    unsigned long long read1 = stoull(temp);
    this_thread::sleep_for(std::chrono::milliseconds(1000));
    rx.close();
    rx.open(rxDir);
    getline(rx, temp);
    unsigned long long read2 = stoll(temp);
    // send diff between 2 reads to get bps
    cout << read2 << "read2" << endl;
    cout << read1 << "read1" << endl;
    return read2 - read1;
}
double getSwap()
{
    // use library to get swap from linux drivers
    auto memory_data = get_system_usage_linux::read_memory_data();
    cout << "answering swap usage call" << endl;
    return (100.0 * memory_data.get_swap_usage());
}

double getStor()
{
    // same as above
    cout << "answering storage usage call" << endl;
    return (100.0 * get_system_usage_linux::get_disk_usage("/"));
}
double getCpu()
{
    // you can't get the cpu usage as a instantaneous percentage, it needs to be estimated since the value
    // is how long the cpu is taking
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