#include "cpptui.hpp"
#include "httplib.h"
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

using namespace std;
// these below are stored as variables for ease of testing for nice values.
// update interval in milliseconds for testing
int updateInterval = 100;
// length of history in seconds
int dataHistory = 1;
// vectors should be initialized to store the length of time for the interval specified
int vectorLength = dataHistory * 1000 / updateInterval;

// warning percentages for cpu, memory, swap, and storage usages
float cpuWarn = 100.0;
float memWarn = 100.0;
float swapWarn = 100.0;
float storageWarn = 100.0;

// to support unlimited ips need to use vector vector
// these store each recorded value of usage for the graphs
vector<vector<double>> cpuUsage;
vector<vector<double>> memoryUsage;
vector<vector<double>> swapUsage;
// unable to do rx and tx as a graph since cpptui expects doubles instead of integers.

// these store non-graph values for each server
vector<unsigned long long> memoryMax;
vector<unsigned long long> memoryUsed;
vector<double> storageUsage;
vector<unsigned long long> rxBps;
vector<unsigned long long> txBps;
// to prevent colors from changing each update store it in a vector
vector<cpptui::Color> serverColors;

struct stats {
    double cpuPer;
    unsigned long memoryUsed;
    unsigned long memoryTotal;
    double memoryPer;
    double swapPer;
    double storagePer;
    unsigned long rxBps;
    unsigned long txBps;
};
stats getStats(string address)
{
    httplib::Client cli(address);
    double cpuPer = 100.0f * stod(cli.Get("/cpu")->body);
    unsigned long long memoryUsed = stoull(cli.Get("/memoryUsed")->body);
    unsigned long long memoryTotal = stoull(cli.Get("/memoryTotal")->body);
    double memoryPer = (100.0f * memoryUsed) / memoryTotal;
    double swapPer = stod(cli.Get("/swap")->body);
    double storagePer = stod(cli.Get("/storage")->body);
    unsigned long long rxBps = stoull(cli.Get("/rx")->body);
    unsigned long long txBps = stoull(cli.Get("/tx")->body);
    return { cpuPer, memoryUsed, memoryTotal, memoryPer, swapPer, storagePer, rxBps, txBps };
}

// function from Adrian's version of ui, modified to support infinitely many ips
void fetch_worker(vector<string> _ips)
{
    while (true) {
        try {
            // for each ip get the stats
            for (int i = 0; i < _ips.size(); i++) {
                string x = _ips[i];
                stats latest = getStats(x);
                // record the latest to the end and remove the beginning to keep a consistent length
                cpuUsage[i].push_back(latest.cpuPer);
                cpuUsage[i].erase(cpuUsage[i].begin());
                memoryUsage[i].push_back(latest.memoryPer);
                memoryUsage[i].erase(memoryUsage[i].begin());
                swapUsage[i].push_back(latest.swapPer);
                swapUsage[i].erase(swapUsage[i].begin());

                // these are just latest values since they don't go to a graph

                memoryUsed[i] = latest.memoryUsed;
                memoryMax[i] = latest.memoryTotal;
                storageUsage[i] = latest.storagePer;
                txBps[i] = latest.txBps;
                rxBps[i] = latest.rxBps;
            }
        } catch (...) {
            // Silently handle server disconnects during the background poll
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(updateInterval));
    }
}
// Adrian's byte formatting function
string format_kilobytes(unsigned long long bytes)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (bytes >= 1024 * 1024)
        ss << (bytes / (1024.0 * 1024.0)) << " GB";
    else if (bytes >= 1024)
        ss << (bytes / (1024.0)) << " MB";
    else
        ss << (bytes) << " KB";
    return ss.str();
}
float randfloat()
{
    return (float)rand() / RAND_MAX;
}

int main()
{
    cout << "How many servers will you be monitoring?" << endl;
    int numberOfAddr;
    cin >> numberOfAddr;
    vector<string> ips;
    for (int i = 0; i < numberOfAddr; i++) {
        cout << "Enter the ip and port of server number " << i + 1 << " in the format (255.255.255.255:8080):" << endl;
        string temp = "";
        cin >> temp;
        ips.push_back(temp);
        // create the vectors to store in the vectors for each server
        vector<double> cpuData(vectorLength, 0);
        vector<double> memoryData(vectorLength, 0);
        vector<double> swapData(vectorLength, 0);
        vector<double> storageData(vectorLength, 0);
        cpuUsage.push_back(cpuData);
        memoryUsage.push_back(memoryData);
        swapUsage.push_back(swapData);
        memoryUsed.push_back(0.0);
        memoryMax.push_back(0.0);
        storageUsage.push_back(0.0);
        txBps.push_back(0);
        rxBps.push_back(0);

        //  calculate colors for servers on graphs once
        //  colors need to be random-ish, mainly computed and non-repeating to support unlimited servers
        serverColors.push_back(cpptui::Color::hsv_to_rgb(randfloat(), randfloat(), 1));
    }
    // make worker thread for getting data
    // maybe do separate threads for each server
    std::thread worker(fetch_worker, ips);
    worker.detach();
    // create ui
    cpptui::App app;
    // make basis
    auto root = make_shared<cpptui::Vertical>();
    // make header
    auto header = make_shared<cpptui::Label>("Linux Infrastructure Observability Dashboard | Press q to exit.");
    header->fixed_height = 2;
    header->underline = true;
    root->add(header);
    // make grid for all graphs to go into
    auto statsboard = make_shared<cpptui::Grid>();
    root->add(statsboard);

    // make housing for cpu chart and label
    auto cpuBlock = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    // add label
    cpuBlock->set_title("CPU Usage (%)", cpptui::Alignment::Left);
    // add chart
    auto cpuChart = make_shared<cpptui::LineChart>();
    // add chart to housing
    cpuBlock->add(cpuChart);
    // define the specifics of the chart
    cpuChart->min_val = 0.0;
    cpuChart->max_val = 100.0;
    cpuChart->show_legend = true;
    cpuChart->show_x_axis = true;
    cpuChart->show_y_axis = true;
    cpuChart->show_y_tick_labels = true;
    cpuChart->show_x_tick_labels = false;
    cpuChart->show_y_ticks = true;
    cpuChart->show_x_ticks = false;
    cpuChart->label_all_x_ticks = true;
    cpuChart->label_all_y_ticks = true;
    cpuChart->y_tick_count = 5;

    // make housing for memory chart and label
    auto memBlock = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    // add label
    memBlock->set_title("Memory Usage (%)", cpptui::Alignment::Left);
    // add chart
    auto memChart = make_shared<cpptui::LineChart>();
    // add chart to housing
    memBlock->add(memChart);
    // define the specifics of the chart
    memChart->min_val = 0.0;
    memChart->max_val = 100.0;
    memChart->show_legend = true;
    memChart->show_x_axis = true;
    memChart->show_y_axis = true;
    memChart->show_y_tick_labels = true;
    memChart->show_x_tick_labels = false;
    memChart->show_y_ticks = true;
    memChart->show_x_ticks = false;
    memChart->label_all_x_ticks = true;
    memChart->label_all_y_ticks = true;
    memChart->y_tick_count = 5;

    // make housing for swap chart and label
    auto swapBlock = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    // add label
    swapBlock->set_title("Swap Usage (%)", cpptui::Alignment::Left);
    // add chart
    auto swapChart = make_shared<cpptui::LineChart>();
    // add chart to housing
    swapBlock->add(swapChart);
    // define the specifics of the chart
    swapChart->min_val = 0.0;
    swapChart->max_val = 100.0;
    swapChart->show_legend = true;
    swapChart->show_x_axis = true;
    swapChart->show_y_axis = true;
    swapChart->show_y_tick_labels = true;
    swapChart->show_x_tick_labels = false;
    swapChart->show_y_ticks = true;
    swapChart->show_x_ticks = false;
    swapChart->label_all_x_ticks = true;
    swapChart->label_all_y_ticks = true;
    swapChart->y_tick_count = 5;

    auto storageBarBlock = make_shared<cpptui::Vertical>();
    vector<shared_ptr<cpptui::ProgressBar>> storageBars(numberOfAddr);
    vector<shared_ptr<cpptui::Label>> storageBarsLabels(numberOfAddr);

    vector<shared_ptr<cpptui::Label>> rxLabels(numberOfAddr);
    vector<shared_ptr<cpptui::Label>> txLabels(numberOfAddr);

    auto rxBorder = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    auto txBorder = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    auto rxBlock = make_shared<cpptui::Vertical>();
    auto txBlock = make_shared<cpptui::Vertical>();
    rxBorder->add(rxBlock);
    txBorder->add(txBlock);

    auto storageBarBorder = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    storageBarBorder->set_title("Storage Usage (%)", cpptui::Alignment::Left);
    storageBarBorder->add(storageBarBlock);
    for (int i = 0; i < numberOfAddr; i++) {
        storageBars[i] = make_shared<cpptui::ProgressBar>();
        storageBars[i]->color = serverColors[i];
        storageBarsLabels[i] = make_shared<cpptui::Label>("Server " + to_string(i + 1) + " (" + ips[i] + ") | Total Usage: " + to_string(storageUsage[i]) + "%");
        storageBarsLabels[i]->fg_color = serverColors[i];
        storageBarBlock->add(storageBarsLabels[i]);
        storageBarBlock->add(storageBars[i]);

        // these are done in here to avoid calling another for loop
        rxLabels[i] = make_shared<cpptui::Label>("Server " + to_string(i + 1) + " (" + ips[i] + ") | Recieved Bytes: " + format_kilobytes(rxBps[i] / 1024ll));
        txLabels[i] = make_shared<cpptui::Label>("Server " + to_string(i + 1) + " (" + ips[i] + ") | Transmitted Bytes: " + format_kilobytes(txBps[i] / 1024));
        txBlock->add(txLabels[i]);
        rxBlock->add(rxLabels[i]);
    }

    // add charts and headers to stats board via housing
    statsboard->add_item(cpuBlock, 0, 0);
    statsboard->add_item(memBlock, 0, 1);
    statsboard->add_item(swapBlock, 1, 1);
    statsboard->add_item(storageBarBorder, 1, 0);
    statsboard->add_item(rxBlock, 2, 0);
    statsboard->add_item(txBlock, 2, 1);
    app.add_timer(updateInterval, [&]() {
        // clear charts
        cpuChart->series.clear();
        memChart->series.clear();
        swapChart->series.clear();
        // update data
        for (int i = 0; i < numberOfAddr; i++) {
            cpuChart->add_series(cpuUsage[i], "Server " + to_string(i + 1) + " (" + ips[i] + ") | Usage:" + to_string(cpuUsage[i][vectorLength - 1]) + "%", serverColors[i], cpptui::LineChart::LineStyle::Braille);
            memChart->add_series(memoryUsage[i], "Server " + to_string(i + 1) + " (" + ips[i] + ") | Used:" + format_kilobytes(memoryUsed[i]) + " | Total: " + format_kilobytes(memoryMax[i]), serverColors[i], cpptui::LineChart::LineStyle::Braille);
            swapChart->add_series(swapUsage[i], "Server " + to_string(i + 1) + " (" + ips[i] + ") | Usage:" + to_string(swapUsage[i][vectorLength - 1]) + "%", serverColors[i], cpptui::LineChart::LineStyle::Braille);
            storageBars[i]->value = storageUsage[i] / 100.0;
            storageBarsLabels[i]->set_text("Server " + to_string(i + 1) + " (" + ips[i] + ") | Usage: " + to_string(storageUsage[i]) + "%");
            rxLabels[i]->set_text("Server " + to_string(i + 1) + " (" + ips[i] + ") | Recieved Bytes: " + format_kilobytes(rxBps[i] / 1024));
            txLabels[i]->set_text("Server " + to_string(i + 1) + " (" + ips[i] + ") | Transmitted Bytes: " + format_kilobytes(txBps[i] / 1024));
        }
    });
    app.register_exit_key('q');
    app.run(root);
}