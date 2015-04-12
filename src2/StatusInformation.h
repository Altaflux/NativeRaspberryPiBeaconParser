#ifndef NATIVESCANNER_EVENTCOUNTS_H
#define NATIVESCANNER_EVENTCOUNTS_H

#include <map>
#include <string>
#include "EventsWindow.h"
#include "hcidumpinternal.h"
#include "SMA.h"

using namespace std;
using Properties = map<string, string>;

class StatusInformation {
private:
    string scannerID;
    string statusQueue;
    int32_t statusInterval;
    int32_t rawEventCount;
    int32_t publishEventCount;
    int32_t heartbeatCount;
    EventsWindow statusWindow;
    shared_ptr<EventsBucket> lastWindow;
    SMA heartbeatRSSI;
    Properties lastStatus;

public:

    StatusInformation() : scannerID(""), rawEventCount(0), publishEventCount(0), heartbeatCount(0), heartbeatRSSI(30) {}

    const string &getStatusQueue() const {
        return statusQueue;
    }

    void setStatusQueue(const string &statusQueue) {
        StatusInformation::statusQueue = statusQueue;
    }

    int getStatusInterval() const {
        return statusInterval;
    }

    void setStatusInterval(int statusInterval) {
        StatusInformation::statusInterval = statusInterval;
        statusWindow.reset(statusInterval);
    }

    void setScannerID(const string& scannerID) {
        StatusInformation::scannerID = scannerID;
    }
    const string& getScannerID() const {
        return scannerID;
    }

    int32_t getRawEventCount() const {
        return rawEventCount;
    }
    void addEvent(const beacon_info &info, bool isHeartbeat) {
        rawEventCount ++;
        if(isHeartbeat) {
            updateHeartbeatCount();
            heartbeatRSSI.add(info.rssi);
        }
        unique_ptr<EventsBucket> window = statusWindow.addEvent(info, isHeartbeat);
        if(window)
            lastWindow.reset(window.release());
    }

    int32_t getPublishEventCount() const {
        return publishEventCount;
    }
    int32_t updatePublishEventCount() {
        return publishEventCount ++;
    }

    int32_t getHeartbeatCount() const {
        return heartbeatCount;
    }
    int32_t updateHeartbeatCount() {
        return heartbeatCount ++;
    }

    int32_t getHeartbeatRSSI() const {
        return heartbeatRSSI.avg();
    }

    void resetStatusWindow(int32_t sizeInSeconds) {
        statusWindow.reset(sizeInSeconds);
    }
    shared_ptr<EventsBucket>& getStatusWindow() {
        return lastWindow;
    }

    const Properties &getLastStatus() const {
        return lastStatus;
    }
    void setLastStatus(Properties &lastStatus) {
        StatusInformation::lastStatus = lastStatus;
    }
};
#endif //NATIVESCANNER_EVENTCOUNTS_H
