#include "cpptui.hpp"
#include "httplib.h"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace cpptui;

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

std::mutex state_mutex;
stats current_stats = {0, 0, 0, 0, 0, 0, 0, 0};
std::atomic<bool> is_fetching(false);

std::string format_network_bytes(unsigned long bytes) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (bytes > 1024 * 1024 * 1024) ss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    else if (bytes > 1024 * 1024) ss << (bytes / (1024.0 * 1024.0)) << " MB";
    else if (bytes > 1024) ss << (bytes / 1024.0) << " KB";
    else ss << bytes << " B";
    return ss.str();
}

std::string format_memory_kb(unsigned long kb) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (kb > 1024 * 1024) ss << (kb / (1024.0 * 1024.0)) << " GB";
    else if (kb > 1024) ss << (kb / 1024.0) << " MB";
    else ss << kb << " KB";
    return ss.str();
}

std::shared_ptr<Label> make_label(std::string text, int w) {
    auto l = std::make_shared<Label>(text);
    l->width = w;
    l->fixed_width = w;
    return l;
}

stats getStats(string address) {
    httplib::Client cli(address);
    cli.set_read_timeout(5, 0); 
    
    float cpuPer = 100.0f * stof(cli.Get("/cpu")->body);
    unsigned long memoryUsed = stoll(cli.Get("/memoryUsed")->body);
    unsigned long memoryTotal = stoll(cli.Get("/memoryTotal")->body);
    float memoryPer = (100.0f * memoryUsed) / memoryTotal;
    float swapPer = stof(cli.Get("/swap")->body);
    float storagePer = stof(cli.Get("/storage")->body);
    unsigned long rxBps = stoll(cli.Get("/rx")->body);
    unsigned long txBps = stoll(cli.Get("/tx")->body);
    
    return { cpuPer, memoryUsed, memoryTotal, memoryPer, swapPer, storagePer, rxBps, txBps };
}

void fetch_worker(string ip) {
    while (true) {
        try {
            is_fetching = true;
            stats latest = getStats(ip);
            
            std::lock_guard<std::mutex> lock(state_mutex);
            current_stats = latest;
        } catch (...) {
            // Silently handle server disconnects during the background poll
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

//DANGER! THIS MAY NOT MATCH OUR VARIABLES! Code/variables  adapted from htop snippet -Adrian

class DashboardRoot : public Vertical {
public:
    bool on_event(const Event &event) override {
        if (Vertical::on_event(event)) return true;
        if (event.type == EventType::Key && (event.key == 'q' || event.key == 'Q')) {
            App::quit();
            return true;
        }
        return false;
    }
};

int main() {
    string ip = "localhost:7777";
    
    std::thread worker(fetch_worker, ip);
    worker.detach();

    App app;
    auto root = std::make_shared<DashboardRoot>();

    auto header = std::make_shared<Label>(" SYSTEM DASHBOARD | Press 'q' to quit ", Color::Cyan());
    header->fixed_height = 2;
    root->add(header);

    auto mem_panel = std::make_shared<Vertical>();
    mem_panel->fixed_height = 4;
    
    auto mem_row = std::make_shared<Horizontal>();
    mem_row->fixed_height = 1;
    auto mem_bar = std::make_shared<ProgressBar>();
    auto mem_text = make_label(" 0.0%", 15);
    mem_row->add(make_label(" RAM  ", 6));
    mem_row->add(mem_bar);
    mem_row->add(mem_text);

    auto swap_row = std::make_shared<Horizontal>();
    swap_row->fixed_height = 1;
    auto swap_bar = std::make_shared<ProgressBar>();
    auto swap_text = make_label(" 0.0%", 15);
    swap_row->add(make_label(" SWAP ", 6));
    swap_row->add(swap_bar);
    swap_row->add(swap_text);

    mem_panel->add(mem_row);
    mem_panel->add(swap_row);
    root->add(mem_panel);
    root->add(std::make_shared<VerticalSpacer>(1));
    
//Adapted from chart snippet

    
    auto top_row = std::make_shared<Horizontal>();
    top_row->fixed_height = 6;

    auto cpu_panel = std::make_shared<Vertical>();
    auto cpu_label = std::make_shared<Label>(" CPU Usage", Color::White());
    cpu_label->fixed_height = 1;
    auto cpu_gauge = std::make_shared<Gauge>(0.0);
    cpu_gauge->fixed_height = 4;
    cpu_panel->add(cpu_label);
    cpu_panel->add(cpu_gauge);

    auto storage_panel = std::make_shared<Vertical>();
    auto storage_label = std::make_shared<Label>(" Storage Usage", Color::White());
    storage_label->fixed_height = 1;
    auto storage_gauge = std::make_shared<Gauge>(0.0);
    storage_gauge->fixed_height = 4;
    storage_panel->add(storage_label);
    storage_panel->add(storage_gauge);

    top_row->add(cpu_panel);
    top_row->add(storage_panel);
    root->add(top_row);
    root->add(std::make_shared<VerticalSpacer>(1));

    auto net_panel = std::make_shared<Vertical>();
    
    auto rx_label = std::make_shared<Label>(" RX (Download) ", Color::Green());
    rx_label->fixed_height = 1;
    auto rx_spark = std::make_shared<Sparkline>();
    rx_spark->fixed_height = 3;
    rx_spark->auto_scale = true;

    auto tx_label = std::make_shared<Label>(" TX (Upload) ", Color::Yellow());
    tx_label->fixed_height = 1;
    auto tx_spark = std::make_shared<Sparkline>();
    tx_spark->fixed_height = 3;
    tx_spark->auto_scale = true;

    net_panel->add(rx_label);
    net_panel->add(rx_spark);
    net_panel->add(std::make_shared<VerticalSpacer>(1));
    net_panel->add(tx_label);
    net_panel->add(tx_spark);
    
    root->add(net_panel);

    app.add_timer(100, [&]() {
        stats s;
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            s = current_stats;
        }

        cpu_gauge->value = s.cpuPer / 100.0f;
        std::stringstream cpu_ss;
        cpu_ss << std::fixed << std::setprecision(1) << s.cpuPer << "%";
        cpu_gauge->max_label = cpu_ss.str();
        
        storage_gauge->value = s.storagePer / 100.0f;
        std::stringstream stor_ss;
        stor_ss << std::fixed << std::setprecision(1) << s.storagePer << "%";
        storage_gauge->max_label = stor_ss.str();

        mem_bar->value = s.memoryPer / 100.0f;
        std::stringstream m_ss;
        m_ss << " " << format_memory_kb(s.memoryUsed) << " / " << format_memory_kb(s.memoryTotal);
        mem_text->set_text(m_ss.str());

        swap_bar->value = s.swapPer / 100.0f;
        std::stringstream sw_ss;
        sw_ss << std::fixed << std::setprecision(1) << " " << s.swapPer << "%";
        swap_text->set_text(sw_ss.str());

        static std::vector<float> rx_history, tx_history;
        rx_history.push_back((float)s.rxBps);
        tx_history.push_back((float)s.txBps);
        
        if (rx_history.size() > 100) rx_history.erase(rx_history.begin());
        if (tx_history.size() > 100) tx_history.erase(tx_history.begin());

        rx_spark->data = rx_history;
        tx_spark->data = tx_history;

        rx_label->set_text(" RX (Download): " + format_network_bytes(s.rxBps) + "/s");
        tx_label->set_text(" TX (Upload): " + format_network_bytes(s.txBps) + "/s");
    });

    app.run(root);
    return 0;
}
