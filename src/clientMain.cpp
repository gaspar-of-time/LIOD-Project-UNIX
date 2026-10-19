#include "cpptui.hpp"
#include "httplib.h"
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
using namespace std;
// these below are stored as variables for ease of testing values and reuse.

// update interval in milliseconds for testing
int updateInterval = 1000;

// length of history in seconds
int dataHistory = 10;

// vectors should be initialized to store the length of time for the interval specified
int vectorLength = dataHistory * 1000 / updateInterval;

// variables for storing warning thresholds for cpu, memory, swap, and storage
double cpuWarn = 100.0;
double memWarn = 100.0;
double swapWarn = 100.0;
double storageWarn = 100.0;

// to support unlimited ips need to use vector vector
// these store the specified time of historical data value for the graphs
vector<vector<double>> cpuUsage;
vector<vector<double>> memoryUsage;
vector<vector<double>> swapUsage;
// unable to do rx and tx as a graph since cpptui expects doubles instead of integers and the

// these store non-graph values for each server
vector<unsigned long long> memoryMax;
vector<unsigned long long> memoryUsed;
vector<double> storageUsage;
vector<unsigned long long> rxBps;
vector<unsigned long long> txBps;
// to prevent colors from changing each update store it in a vector and calculate it once
vector<cpptui::Color> serverColors;

// to store the stats from the server
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

// get the stats from the server
stats getStats(string address)
{
    httplib::Client cli(address);
    // throw an exception if the ip doesn't connect
    if (!cli.Get("/")) {
        throw "DNE";
        return { 0, 0, 0, 0, 0, 0, 0, 0 };
    }
    // otherwise keep going
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

// to test if the ip is valid on client start it checks if the api exists on this address
bool tryStats(string address)
{
    httplib::Client cli(address);
    // returns false if it fails to get
    return cli.Get("/");
}

// function from Adrian's version of ui, modified to support infinitely many ips
// recieves and stores the stats for each ip address provided on start
void fetch_worker(vector<string> _ips)
{
    // constantly loops to get the stats from the server every specified interval
    while (true) {

        // loop through each ip address
        for (int i = 0; i < _ips.size(); i++) {
            // using try to handle disconnects by recieving the throw from getStats
            try {
                stats latest = getStats(_ips[i]);
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
            } catch (...) {
                // Silently handle server disconnects during the background poll
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(updateInterval));
    }
}

// Adrian's byte formatting function, adjusted since memory is returned in kilobytes
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

// used for generating colors for each server
float randfloat()
{
    return (float)rand() / RAND_MAX;
}

// the meat and potatoes of the client
// recieves servers as commandline arguments
int main(int argc, char* argv[])
{
    // seed the random number function using a static number since the colors don't need to be truly random
    srand(9);

    // since the program is counted as an argument, make the number of addresses one less
    int numberOfAddr = argc - 1;

    // if there's no servers then do nothing since the program needs a server to monitor because that's it's whole point
    if (numberOfAddr <= 0) {
        return 0;
    }

    // allocate a vector of strings to store all the ip addresses
    // not strictly necessary but I initially got the addresses in program and this is a bit of a holdover
    // I could very well just use the argument array but then I would have to adjust all of the code
    vector<string> ips;

    for (int i = 0; i < numberOfAddr; i++) {
        // make sure each address exists before continuing to execute
        if (!tryStats(argv[i + 1])) {
            cout << "Invalid Arguement for Server " + to_string(i + 1) << endl
                 << "Make sure you typed in the ip and port correctly and that the server is running." << endl;
            return 0;
        }

        // if the address is valid then add it to the vector
        ips.push_back(argv[i + 1]);

        // create the vectors to store in the vector containing vectors for each ip address
        vector<double> cpuData(vectorLength, 0);
        vector<double> memoryData(vectorLength, 0);
        vector<double> swapData(vectorLength, 0);
        vector<double> storageData(vectorLength, 0);

        // and add these vectors to their containing vectors
        cpuUsage.push_back(cpuData);
        memoryUsage.push_back(memoryData);
        swapUsage.push_back(swapData);

        // since these aren't vector vectors, just add a zero for each server until the data is populated
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
    // uses a single thread since that seems to work fine in testing
    std::thread worker(fetch_worker, ips);
    worker.detach();

    // create base ui for the program
    cpptui::App app;

    // set ui theme
    cpptui::Theme::set_theme(cpptui::Theme::Dark());

    // make root that will contain all other elements
    auto root = make_shared<cpptui::Vertical>();

    // make header that contains usage information and the name of the program
    auto header = make_shared<cpptui::Label>("Linux Infrastructure Observability Dashboard | Updating every " + to_string(updateInterval / 1000) + " second(s) (probably) | Press q to exit. | Press e for settings");
    header->fixed_height = 1;
    header->underline = true;
    root->add(header);

    // make grid for all graphs to go into
    // auto statsboard = make_shared<cpptui::Grid>();
    // root->add(statsboard);
    auto statsUpper = make_shared<cpptui::Horizontal>();
    auto statsLower = make_shared<cpptui::Horizontal>();
    auto statsAll = make_shared<cpptui::SplitPane>();
    statsAll->set_panes(statsUpper, statsLower);
    statsAll->ratio = .7;
    statsAll->vertical = true;
    statsAll->max_height = 0;
    root->add(statsAll);

    // make border for cpu chart for readability and labeling
    auto cpuBlock = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);

    // define the title of the border
    cpuBlock->set_title("CPU Usage (%)", cpptui::Alignment::Left);

    // add chart
    auto cpuChart = make_shared<cpptui::LineChart>();

    // add chart to housing
    cpuBlock->add(cpuChart);

    // define the specifics of the chart, 10 ticks displays strangely so using 5
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

    // make border for memory chart
    auto memBlock = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);

    // set the title of the border
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

    // set the title of the border
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

    // make vectors to dynamically create the storage bar elements and labels
    vector<shared_ptr<cpptui::ProgressBar>> storageBars(numberOfAddr);
    vector<shared_ptr<cpptui::Label>> storageBarsLabels(numberOfAddr);

    // make vectors to dynamically create the text for the rx and tx metrics
    // this was originally elsewhere but I didn't want to make another for loop
    vector<shared_ptr<cpptui::Label>> rxLabels(numberOfAddr);
    vector<shared_ptr<cpptui::Label>> txLabels(numberOfAddr);

    // define the borders for the transmitted and recived bytes, and set their titles
    auto rxBorder = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    rxBorder->set_title("Recieved Bytes (B/s)", cpptui::Alignment::Left);
    auto txBorder = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    txBorder->set_title("Transmitted Bytes (B/s)", cpptui::Alignment::Left);

    // make a block to go within the border that will store the text labels
    auto rxBlock = make_shared<cpptui::ScrollableVertical>();
    auto txBlock = make_shared<cpptui::ScrollableVertical>();

    // add the rx and tx blocks to their borders
    rxBorder->add(rxBlock);
    txBorder->add(txBlock);
    txBlock->min_height = 1;
    rxBlock->min_height = 1;

    // create the border for the storage bars and set the title
    auto storageBarBorder = make_shared<cpptui::Border>(cpptui::BorderStyle::Double);
    storageBarBorder->set_title("Storage Usage (%)", cpptui::Alignment::Left);

    // define the section for storage percentage bars and add it to the border
    auto storageBarBlock = make_shared<cpptui::ScrollableVertical>();
    storageBarBorder->add(storageBarBlock);
    storageBarBlock->min_height = 2;

    // for each ip add the storage percentage bars, and rx and tx labels
    for (int i = 0; i < numberOfAddr; i++) {
        // make the storage bar
        storageBars[i] = make_shared<cpptui::ProgressBar>();

        // set the bar's color to match the rest of the server's metrics
        storageBars[i]->color = serverColors[i];

        // label the bars and make the label text match the server's metrics
        storageBarsLabels[i] = make_shared<cpptui::Label>("Server " + to_string(i + 1) + " (" + ips[i] + ") | Total Usage: " + to_string(storageUsage[i]) + "%");
        storageBarsLabels[i]->fg_color = serverColors[i];

        // add the label before the bar so the label is above it
        storageBarBlock->add(storageBarsLabels[i]);
        storageBarBlock->add(storageBars[i]);

        // these are done in here to avoid calling another for loop
        // add the tx and rx labels to display the recieved and transmitted bytes in the form of text
        rxLabels[i] = make_shared<cpptui::Label>("Server " + to_string(i + 1) + " (" + ips[i] + ") | Recieved Bytes: " + format_kilobytes(rxBps[i] / 1024ll) + "/s");
        txLabels[i] = make_shared<cpptui::Label>("Server " + to_string(i + 1) + " (" + ips[i] + ") | Transmitted Bytes: " + format_kilobytes(txBps[i] / 1024) + "/s");

        // set their colors to match the server's color
        rxLabels[i]->fg_color = serverColors[i];
        txLabels[i]->fg_color = serverColors[i];

        // add the label to the rx and tx housings
        txBlock->add(txLabels[i]);
        rxBlock->add(rxLabels[i]);
    }

    // make settings for configuring warning thresholds
    auto settings = make_shared<cpptui::Dialog>(&app, cpptui::BorderStyle::Double);
    // height and width need to be defined to display properly
    settings->width = 60;
    settings->height = 12;
    settings->fixed_height = 12;
    settings->fixed_width = 60;
    settings->modal = true;
    settings->set_title("Settings:", cpptui::Alignment::Left);

    // make a container to hold each element of the settings
    auto settingsContainer = make_shared<cpptui::Vertical>();

    // add a slider and label for defining the cpu warning threshold.
    // text displays 100% since that's the default value and the label gets updated when the value changes
    auto cpuInputLabel = make_shared<cpptui::Label>("CPU% Warning Threshold (100%):");
    auto cpuInput = make_shared<cpptui::Slider>();

    // same but for the memory
    auto memInputLabel = make_shared<cpptui::Label>("Memory% Warning Threshold (100%):");
    auto memInput = make_shared<cpptui::Slider>();

    // same but for the swap
    auto swapInputLabel = make_shared<cpptui::Label>("Swap% Warning Threshold (100%):");
    auto swapInput = make_shared<cpptui::Slider>();

    // same but for the storage
    auto storageInputLabel = make_shared<cpptui::Label>("Storage Warning Threshold (100%):");
    auto storageInput = make_shared<cpptui::Slider>();

    // define the parameters for the cpu slider
    cpuInput->max_value = 100;
    cpuInput->min_value = 0;
    cpuInput->value = 100;
    // store the value of the slider in a variable when it changes and update the text to reflect the new value
    cpuInput->on_change = [cpuInputLabel](int v) { cpuWarn = v;  cpuInputLabel->set_text("CPU Warning Threshold ("+to_string(v)+"%):"); };

    // define parameters for memory slider
    memInput->max_value = 100;
    memInput->min_value = 0;
    memInput->value = 100;
    // store the value of the slider in a variable when it changes and update the text to reflect the new value
    memInput->on_change = [memInputLabel](int v) { memWarn = v;  memInputLabel->set_text("Memory Warning Threshold ("+to_string(v)+"%):"); };

    // define parameters for swap slider
    swapInput->max_value = 100;
    swapInput->min_value = 0;
    swapInput->value = 100;
    // store the value of the slider in a variable when it changes and update the text to reflect the new value
    swapInput->on_change = [swapInputLabel](int v) { swapWarn = v;  swapInputLabel->set_text("Swap Warning Threshold ("+to_string(v)+"%):"); };

    // define parameters for storage slider
    storageInput->max_value = 100;
    storageInput->min_value = 0;
    storageInput->value = 100;
    // store the value of the slider in a variable when it changes and update the text to reflect the new value
    storageInput->on_change = [storageInputLabel](int v) { storageWarn = v; storageInputLabel->set_text("Storage Warning Threshold ("+to_string(v)+"%):"); };

    // add toggle switch to hide legends
    auto toggleLegends = make_shared<cpptui::ToggleSwitch>("Enable Legends");
    toggleLegends->is_on = true;
    toggleLegends->on_change = [cpuChart, memChart, swapChart](bool v) {cpuChart->show_legend = v; memChart->show_legend = v; swapChart->show_legend = v; };

    // add the sliders and labels to the settings container
    settingsContainer->add(toggleLegends);
    settingsContainer->add(cpuInputLabel);
    settingsContainer->add(cpuInput);
    settingsContainer->add(memInputLabel);
    settingsContainer->add(memInput);
    settingsContainer->add(swapInputLabel);
    settingsContainer->add(swapInput);
    settingsContainer->add(storageInputLabel);
    settingsContainer->add(storageInput);

    // add the settings container to the settings
    settings->add(settingsContainer);

    // create a key to open and close settings
    // checks if settings is closed and opens it, otherwise closes it
    app.register_key('e', [settings] { if(!settings->is_open){settings->open();}else{settings->close();} });

    // add charts and headers to stats board via their housings
    // statsboard->add_item(cpuBlock, 0, 0);
    statsUpper->add(cpuBlock);
    // statsboard->add_item(memBlock, 0, 1);
    statsUpper->add(memBlock);
    // statsboard->add_item(swapBlock, 0, 2);
    statsUpper->add(swapBlock);
    // statsboard->add_item(storageBarBorder, 1, 0);
    statsLower->add(storageBarBorder);
    // statsboard->add_item(rxBorder, 1, 1);
    statsLower->add(rxBorder);
    // statsboard->add_item(txBorder, 1, 2);
    statsLower->add(txBorder);

    // create notification for threshold being passed
    auto alert = make_shared<cpptui::Notification>();
    alert->position = cpptui::Notification::Position::BottomRight;
    alert->max_visible = 5;
    alert->min_height = 0;
    alert->max_height = 1;

    // add the notification component to the root
    root->add(alert);

    // this timer runs at a regular interval to update the ui data using the data fetched by the worker thread
    app.add_timer(updateInterval, [&]() {
        // clear charts to refresh data, otherwise it will keep adding to the chart
        cpuChart->series.clear();
        memChart->series.clear();
        swapChart->series.clear();

        // update data by looping through each ip
        for (int i = 0; i < numberOfAddr; i++) {

            // add the cpu usage to its chart, label it with its current usage % and give it the server color
            cpuChart->add_series(cpuUsage[i], "Server " + to_string(i + 1) + " (" + ips[i] + ") | Usage:" + to_string(cpuUsage[i][vectorLength - 1]) + "%", serverColors[i], cpptui::LineChart::LineStyle::Braille);
            // check if the usage is above the warning threshold
            if (cpuWarn <= cpuUsage[i][vectorLength - 1]) {
                alert->show("Server " + to_string(i + 1) + "'s CPU is " + to_string(cpuUsage[i][vectorLength - 1]) + "%", cpptui::Notification::Type::Warning, 3000);
            }

            // add the memory usage to its chart, label it with its current usage % and give it the server color
            memChart->add_series(memoryUsage[i], "Server " + to_string(i + 1) + " (" + ips[i] + ") | Used:" + format_kilobytes(memoryUsed[i]) + " | Total: " + format_kilobytes(memoryMax[i]), serverColors[i], cpptui::LineChart::LineStyle::Braille);
            // check if the usage is above the warning threshold
            if (memWarn <= memoryUsage[i][vectorLength - 1]) {
                alert->show("Server " + to_string(i + 1) + "'s Memory is " + to_string(memoryUsage[i][vectorLength - 1]) + "%", cpptui::Notification::Type::Warning, 3000);
            }

            // add the swap usage to its chart, label it with its current usage % and give it the server color
            swapChart->add_series(swapUsage[i], "Server " + to_string(i + 1) + " (" + ips[i] + ") | Usage:" + to_string(swapUsage[i][vectorLength - 1]) + "%", serverColors[i], cpptui::LineChart::LineStyle::Braille);
            // check if the usage is above the warning threshold
            if (swapWarn <= swapUsage[i][vectorLength - 1]) {
                alert->show("Server " + to_string(i + 1) + "'s Swap is " + to_string(swapUsage[i][vectorLength - 1]) + "%", cpptui::Notification::Type::Warning, 3000);
            }

            // update the storage bar's percentage
            storageBars[i]->value = storageUsage[i] / 100.0;
            // check if the usage is above the warning threshold
            if (storageWarn <= storageUsage[i]) {
                alert->show("Server " + to_string(i + 1) + "'s Storage is " + to_string(storageUsage[i]) + "%", cpptui::Notification::Type::Warning, 3000);
            }
            // update the label to reflect the current usage
            storageBarsLabels[i]->set_text("Server " + to_string(i + 1) + " (" + ips[i] + ") | Usage: " + to_string(storageUsage[i]) + "%");

            // update the rx and tx text with the current bps
            rxLabels[i]->set_text("Server " + to_string(i + 1) + " (" + ips[i] + ") | Recieved Bytes: " + format_kilobytes(rxBps[i] / 1024) + "/s");
            txLabels[i]->set_text("Server " + to_string(i + 1) + " (" + ips[i] + ") | Transmitted Bytes: " + format_kilobytes(txBps[i] / 1024) + "/s");
        }
    });
    // register quit key
    app.register_exit_key('q');

    // run the ui
    app.run(root);
}