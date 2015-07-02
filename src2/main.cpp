#include <cstdio>
#include <string>
#include <regex>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <execinfo.h>
// http://tclap.sourceforge.net/manual.html
#include "tclap/CmdLine.h"
#include "HCIDumpParser.h"
#include "MsgPublisherTypeConstraint.h"
#include "LcdDisplayTypeConstraint.h"
#include "MockScannerView.h"
#include "BeaconMapper.h"
#include "../lcd/AbstractLcdView.h"

static HCIDumpParser parserLogic;

#define STOP_MARKER_FILE "/var/run/scannerd.STOP"

inline bool stopMarkerExists() {
    struct stat buffer;
    bool stop = (stat (STOP_MARKER_FILE, &buffer) == 0);
    if(stop) {
        printf("Found STOP marker file, will exit...\n");
        fflush(stdout);
    }
    return stop;
}
static long eventCount = 0;
static long maxEventCount = 0;
static int64_t lastMarkerCheckTime = 0;

/**
* Callback invoked by the hdidumpinternal.c code when a LE_ADVERTISING_REPORT event is seen on the stack
*/
extern "C" bool beacon_event_callback(beacon_info * info) {
#if PRINT_DEBUG
    printf("beacon_event_callback(%ld: %s, code=%d, time=%lld)\n", eventCount, info->uuid, info->code, info->time);
#endif
    parserLogic.beaconEvent(*info);
    eventCount ++;
    // Check for a termination marker every 1000 events or 5 seconds
    bool stop = false;
    int64_t elapsed = info->time - lastMarkerCheckTime;
    if((eventCount % 1000) == 0 || elapsed > 5000) {
        lastMarkerCheckTime = info->time;
        stop = stopMarkerExists();
        printf("beacon_event_callback, status eventCount=%ld, stop=%d\n", eventCount, stop);
        fflush(stdout);
    }
    // Check max event count limit
    if(maxEventCount > 0 && eventCount >= maxEventCount)
        stop = true;
    return stop;
}

using namespace std;

static ScannerView *getDisplayInstance(LcdDisplayType type) {
    ScannerView *view = nullptr;
#ifdef HAVE_LCD_DISPLAY
    AbstractLcdView *lcd = AbstractLcdView::getLcdDisplayInstance(type);
    lcd->init();
    lcd->clear();
    view = lcd;
#else
    view = new MockScannerView();
#endif
    return view;
}

void printStacktrace() {
    void *array[20];
    int size = backtrace(array, sizeof(array) / sizeof(array[0]));
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}

static void terminateHandler() {
    std::exception_ptr exptr = std::current_exception();
    if (exptr != nullptr) {
        // the only useful feature of std::exception_ptr is that it can be rethrown...
        try {
            std::rethrow_exception(exptr);
        } catch (std::exception &ex) {
            std::fprintf(stderr, "Terminated due to exception: %s\n", ex.what());
        }
        catch (...) {
            std::fprintf(stderr, "Terminated due to unknown exception\n");
        }
    }
    else {
        std::fprintf(stderr, "Terminated due to unknown reason :(\n");
    }
    printStacktrace();
    exit(100);
}

/**
* A version of the native scanner that directly integrates with the bluez stack hcidump command rather than parsing
* the hcidump output.
*/
int main(int argc, const char **argv) {

    printf("NativeScanner starting up...\n");
    for(int n = 0; n < argc; n ++) {
        printf("    argv[%d] = %s\n", n, argv[n]);
    }
    fflush(stdout);
    TCLAP::CmdLine cmd("NativeScanner command line options", ' ', "0.1");
    //
    TCLAP::ValueArg<std::string> scannerID("s", "scannerID",
            "Specify the ID of the scanner reading the beacon events. If this is a string with a comma separated list of names, the scanner will cycle through them.",
            true, "DEFAULT", "string", cmd);
    TCLAP::ValueArg<std::string> heartbeatUUID("H", "heartbeatUUID",
            "Specify the UUID of the beacon used to signal the scanner heartbeat event",
            false, "DEFAULT", "string", cmd);
    TCLAP::ValueArg<std::string> rawDumpFile("d", "rawDumpFile",
            "Specify a path to an hcidump file to parse for testing",
            false, "", "string", cmd);
    TCLAP::ValueArg<std::string> clientID("c", "clientID",
            "Specify the clientID to connect to the MQTT broker with",
            false, "", "string", cmd);
    TCLAP::ValueArg<std::string> username("u", "username",
            "Specify the username to connect to the MQTT broker with",
            false, "", "string", cmd);
    TCLAP::ValueArg<std::string> password("p", "password",
            "Specify the password to connect to the MQTT broker with",
            false, "", "string", cmd);
    TCLAP::ValueArg<std::string> brokerURL("b", "brokerURL",
            "Specify the brokerURL to connect to the message broker with; default tcp://localhost:5672",
            false, "tcp://localhost:5672", "string", cmd);
    TCLAP::ValueArg<std::string> destinationName("t", "destinationName",
            "Specify the name of the destination on the message broker to publish to; default beaconEvents",
            false, "beaconEvents", "string", cmd);
    TCLAP::ValueArg<int> analyzeWindow("W", "analyzeWindow",
            "Specify the number of seconds in the analyzeMode time window",
            false, 1, "int", cmd);
    TCLAP::ValueArg<std::string> hciDev("D", "hciDev",
            "Specify the name of the host controller interface to use; default hci0",
            false, "hci0", "string", cmd);
    TCLAP::ValueArg<std::string> bcastAddress("", "bcastAddress",
                                              "Address to broadcast scanner status to as backup to statusQueue if non-empty; default empty",
                                              false, "", "string", cmd);
    TCLAP::ValueArg<int> bcastPort("", "bcastPort",
                                              "Port to broadcast scanner status to as backup to statusQueue if non-empty; default empty",
                                              false, 12345, "int", cmd);
    MsgPublisherTypeConstraint pubTypeConstraint;
    TCLAP::ValueArg<std::string> pubType("P", "pubType",
            "Specify the MsgPublisherType enum for the publisher implementation to use; default AMQP_QPID",
            false, "AMQP_QPID", &pubTypeConstraint, cmd, nullptr);
    LcdDisplayTypeConstraint lcdTypeConstraint;
    TCLAP::ValueArg<std::string> lcdType("", "lcdType",
             "Specify the LcdDisplayType enum for the LCD implementation to use; default PCD8544",
             false, "PCD8544", &lcdTypeConstraint, cmd, nullptr);

    TCLAP::ValueArg<int> maxCount("C", "maxCount",
            "Specify a maxium number of events the scanner should process before exiting; default 0 means no limit",
            false, 0, "int", cmd);
    TCLAP::ValueArg<int> batchCount("B", "batchCount",
            "Specify a maxium number of events the scanner should combine before sending to broker; default 0 means no batching",
            false, 0, "int", cmd);
    TCLAP::ValueArg<int> statusInterval("I", "statusInterval",
            "Specify the interval in seconds between health status messages, <= 0 means no messages; default 30",
            false, 30, "int", cmd);
    TCLAP::ValueArg<int> rebootAfterNoReply("r", "rebootAfterNoReply",
            "Specify the interval in seconds after which a failure to hear our own heartbeat triggers a reboot, <= 0 means no reboot; default -1",
            false, -1, "int", cmd);
    TCLAP::ValueArg<std::string> statusQueue("q", "statusQueue",
            "Specify the name of the status health queue destination; default scannerHealth",
            false, "scannerHealth", "string", cmd);
    TCLAP::SwitchArg skipPublish("S", "skipPublish",
            "Indicate that the parsed beacons should not be published",
            false);
    TCLAP::SwitchArg asyncMode("A", "asyncMode",
            "Indicate that the parsed beacons should be published using async delivery mode",
            false);
    TCLAP::SwitchArg useQueues("Q", "useQueues",
            "Indicate that the destination type is a queue. If not given the default type is a topic.",
            false);
    TCLAP::SwitchArg skipHeartbeat("K", "skipHeartbeat",
            "Don't publish the heartbeat messages. Useful to limit the noise when testing the scanner.",
            false);
    TCLAP::SwitchArg analzyeMode("", "analzyeMode",
            "Run the scanner in a mode that simply collects beacon readings and reports unique beacons seen in a time window",
             false);
    TCLAP::SwitchArg generateTestData("T", "generateTestData",
             "Indicate that test data should be generated",
             false);
    TCLAP::SwitchArg noBrokerReconnect("", "noBrokerReconnect",
             "Don't try to reconnect to the broker on failure, just exit",
             false);
    TCLAP::SwitchArg skipScannerView("", "skipScannerView",
              "Skip the scanner view display of closest beacon",
              false);
    TCLAP::SwitchArg  batteryTestMode("", "batteryTestMode",
                                     "Simply monitor the raw heartbeat beacon events and publish them to the destinationName",
                                     false);
    try {
        // Add the flag arguments
        cmd.add(skipPublish);
        cmd.add(asyncMode);
        cmd.add(useQueues);
        cmd.add(skipHeartbeat);
        cmd.add(analzyeMode);
        cmd.add(generateTestData);
        cmd.add(noBrokerReconnect);
        cmd.add(batteryTestMode);
        cmd.add(skipScannerView);
        // Parse the argv array.
        printf("Parsing command line...\n");
        cmd.parse( argc, argv );
        printf("done\n");
    }
    catch (TCLAP::ArgException &e) {
        fprintf(stderr, "error: %s for arg: %s\n", e.error().c_str(), e.argId().c_str());
    }

    // Remove any stop marker file
    if(remove(STOP_MARKER_FILE) == 0) {
        printf("Removed existing %s marker file\n", STOP_MARKER_FILE);
    }

    // Validate the system time as unless the network time has syncd, just exit to wait for it to happen
    int64_t nowMS = EventsWindow::currentMilliseconds();
    // 1434954728562 = 2015-06-21 23:32:08.562
    if(nowMS < 1434954728562) {
        fprintf(stderr, "currentMilliseconds(%ld) < 1434954728562 = 2015-06-21 23:32:08.562", nowMS);
        exit(1);
    }

    HCIDumpCommand command(scannerID.getValue(), brokerURL.getValue(), clientID.getValue(), destinationName.getValue());
    command.setUseQueues(useQueues.getValue());
    command.setSkipPublish(skipPublish.getValue());
    command.setSkipHeartbeat(skipHeartbeat.getValue());
    command.setHciDev(hciDev.getValue());
    command.setAsyncMode(asyncMode.getValue());
    command.setAnalyzeMode(analzyeMode.getValue());
    command.setAnalyzeWindow(analyzeWindow.getValue());
    command.setPubType(pubTypeConstraint.toType(pubType.getValue()));
    command.setStatusInterval(statusInterval.getValue());
    command.setStatusQueue(statusQueue.getValue());
    command.setGenerateTestData(generateTestData.getValue());
    command.setBatteryTestMode(batteryTestMode.getValue());
    command.setUsername(username.getValue());
    command.setPassword(password.getValue());
    command.setBcastAddress(bcastAddress.getValue());
    command.setBcastPort(bcastPort.getValue());
    if(maxCount.getValue() > 0) {
        maxEventCount = maxCount.getValue();
        printf("Set maxEventCount: %ld\n", maxEventCount);
    }
    parserLogic.setBatchCount(batchCount.getValue());
    if(heartbeatUUID.isSet()) {
        parserLogic.setScannerUUID(heartbeatUUID.getValue());
        printf("Set heartbeatUUID: %s\n", heartbeatUUID.getValue().c_str());
    }

    // Install default terminate handler to make sure we exit with non-zero status
    std::set_terminate(terminateHandler);

    // Create a beacon mapping instance
    shared_ptr<AbstractBeaconMapper> beaconMapper(new BeaconMapper());
    printf("Loading the beacon to user mapping...\n");
    beaconMapper->refresh();

    if(!skipScannerView.getValue()) {
        // Get the scanner view implementation
        LcdDisplayType type = lcdTypeConstraint.toType(lcdType.getValue());
        if(type == LcdDisplayType::INVALID_LCD_TYPE) {
            fprintf(stderr, "Skipping scanner view due to invalid LcdDisplayType. No mapping for: %s\n", lcdType.getValue().c_str());
        } else {
            printf("Using LCD type=%s\n",  lcdType.getValue().c_str());
            shared_ptr<ScannerView> lcd(getDisplayInstance(type));
            // Set the scanner view display's beacon mapper to display the minorID to name correctly
            lcd->setBeaconMapper(beaconMapper);
            parserLogic.setScannerView(lcd);
        }
    }

    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("Begin scanning, cwd=%s...\n", cwd);
    fflush(stdout);
    parserLogic.processHCI(command);
    parserLogic.cleanup();
    printf("End scanning\n");
    fflush(stdout);
    return 0;
}
