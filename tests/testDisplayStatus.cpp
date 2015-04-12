#include <chrono>
#include <iostream>
#include <memory>
#include <string.h>
#include "../lcd/LcdDisplay.h"
#include "../src2/HealthStatus.h"

using namespace std;
using namespace std::chrono;

int main() {
    unique_ptr<LcdDisplay> lcd(LcdDisplay::getLcdDisplayInstance());
    lcd->init();
    StatusInformation status;
    beacon_info beacon;
    memset(&beacon, sizeof(beacon), 0);
    status.setScannerID("Room206");

    // Add some events
    sprintf(beacon.uuid, "UUID-%.10d", 66);
    beacon.minor = 66;
    beacon.major = 1;
    beacon.manufacturer = 0xdead;
    beacon.code = 0xbeef;
    beacon.count = 3;
    beacon.rssi = -68;
    milliseconds now = duration_cast<milliseconds >(high_resolution_clock::now().time_since_epoch());
    beacon.time = now.count();
    status.addEvent(beacon, false);

    beacon.isHeartbeat = true;
    beacon.minor = 206;
    beacon.time += 10;
    status.addEvent(beacon, true);

    beacon.isHeartbeat = false;
    beacon.minor = 56;
    beacon.time += 10;
    status.addEvent(beacon, false);

    beacon.isHeartbeat = true;
    beacon.minor = 206;
    beacon.time += 10;
    status.addEvent(beacon, true);

    HealthStatus healthStatus;
    healthStatus.calculateStatus(status);
    Properties statusProps = healthStatus.getLastStatus();
    char tmp[20];
    snprintf(tmp, 20, "%s:%.5d;%d", status.getScannerID().c_str(), status.getHeartbeatCount(), status.getHeartbeatRSSI());
    lcd->displayText(tmp, 0, 0);
    string uptime = statusProps["Uptime"];
    printf("%s; length=%d\n", uptime.c_str(), uptime.size());
    snprintf(tmp, 20, "UP D:dd H:hh M:mm");
    lcd->displayText(tmp, 0, 1);
    const char *load = statusProps["LoadAverage"].c_str();
    lcd->displayText(load, 0, 2);


    cout << "Enter any key to exit: ";
    std::string line; std::getline(std::cin, line);
    lcd->clear();
}
