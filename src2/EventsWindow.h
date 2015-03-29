//
// Created by starksm on 3/28/15.
//

#ifndef NATIVESCANNER_EVENTSWINDOW_H
#define NATIVESCANNER_EVENTSWINDOW_H

#include <stdint.h>
#include <map>
#include <memory>
#include "hcidumpinternal.h"
#include "EventsBucket.h"

using namespace std;
//
//using EventsBucket = map<int32_t, beacon_info>;

/**
 * A class that represents a collection of beacon_info events seen within a time window
 */
class EventsWindow {
private:
    int32_t windowSizeSeconds;
    // Current analyze window begin/end in milliseconds to be compatible with beacon_info.time
    int64_t begin;
    int64_t end;
    map<int32_t, beacon_info> eventsMap;
public:

    static int64_t currentMilliseconds();

    int32_t getWindowSizeSeconds() const {
        return windowSizeSeconds;
    }

    int64_t getBegin() const {
        return begin;
    }

    int64_t getEnd() const {
        return end;
    }

    int64_t reset(int32_t sizeInSeconds);
    unique_ptr<EventsBucket> addEvent(const beacon_info& info);
};

#endif //NATIVESCANNER_EVENTSWINDOW_H
