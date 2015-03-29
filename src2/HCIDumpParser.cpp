#include <Beacon.h>
#include "HCIDumpParser.h"
#include "BeaconEventConsumer.h"

// Uncomment to publish the raw byte[] for the beacon, otherwise, properties are sent
//#define SEND_BINARY_DATA



void HCIDumpParser::processHCI(HCIDumpCommand& parseCommand) {
    HCIDumpParser::parseCommand = parseCommand;
    eventConsumer.setParseCommand(parseCommand);
    string clientID(parseCommand.getClientID());
    if(clientID.empty())
        clientID = parseCommand.getScannerID();
    publisher.reset(MsgPublisher::create(parseCommand.getPubType(), parseCommand.getBrokerURL(), clientID, "", ""));
    timeWindow.reset(parseCommand.getAnalyzeWindow());
    if(parseCommand.isAnalyzeMode()) {
        printf("Running in analyze mode, window=%d seconds, begin=%lld\n", parseCommand.getAnalyzeWindow(), timeWindow.getBegin());
    }
    else if(!parseCommand.isSkipPublish()) {
        publisher->setUseTopics(!parseCommand.isUseQueues());
        publisher->setDestinationName(parseCommand.getDestinationName());
        if(batchCount > 0) {
            publisher->setUseTransactions(true);
            printf("Enabled transactions\n");
        }
        publisher->start(parseCommand.isAsyncMode());
        // Create a thread for the consumer
        eventExchanger.reset(new EventExchanger);
        eventConsumer.init(eventExchanger, publisher);
        consumerThread.reset(new thread(&BeaconEventConsumer::publishEvents, eventConsumer));
    }
    else {
        printf("Skipping publish of parsed beacons\n");
    }

    char cdev = parseCommand.getHciDev().at(parseCommand.getHciDev().size()-1);
    int device = cdev - '0';
    scan_frames(device,  beacon_event_callback);
}

void HCIDumpParser::beaconEvent(const beacon_info& info) {
    // Check for heartbeat
    bool isHeartbeat = scannerUUID.compare(info.uuid) == 0;
    // Queue the event into the
    unique_ptr<EventsBucket> bucket = timeWindow.addEvent(info);
    // Now handle
    if(bucket) {
        if(parseCommand.isAnalyzeMode()) {
            printBeaconCounts(bucket);
        }
        else if(!parseCommand.isSkipPublish()) {
            beacon_info* event = new beacon_info(info);
            event->isHeartbeat = isHeartbeat;
            eventExchanger->putEvent(event);
        }
    }
    else {
        Beacon beacon(parseCommand.getScannerID(), info.uuid, info.code, info.manufacturer, info.major, info.minor,
                      info.power, info.calibrated_power, info.rssi, info.time);
        const char *info = isHeartbeat ? "heartbeat" : "event";
        if(!isHeartbeat || (isHeartbeat && !parseCommand.isSkipHeartbeat()))
            printf("Parsed(%s): %s\n", info, beacon.toString().c_str());
    }
}

void HCIDumpParser::cleanup() {
    if(publisher)
        publisher->stop();
}

void HCIDumpParser::printBeaconCounts(const unique_ptr<EventsBucket> &bucket) {
#ifdef PRINT_DEBUG
    printf("updateBeaconCounts(%d); begin=%lld, end=%lld, info.time=%lld\n", beaconCounts.size(), begin, end, info.time);
#endif
    char timestr[256];
    struct timeval tv;
    tv.tv_sec = timeWindow.getBegin()/1000;
    tv.tv_usec = 0;
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    strftime(timestr, 128, "%r", &tm);
    // Report the stats for this time window and then reset
    printf("+++ Beacon counts for window(%ld,%d): %s\n", bucket->size(), parseCommand.getAnalyzeWindow(), timestr);
    printf("\t");
    EventsBucket::iterator iter = bucket->begin();
    while(iter != bucket->end()) {
        printf("+%2d: %2d; ", iter->first, iter->second.count);
        iter ++;
    }
    printf("\n");
}
