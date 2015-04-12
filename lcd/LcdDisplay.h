//
// Created by Scott Stark on 4/7/15.
//

#ifndef NATIVESCANNER_LCDDISPLAY_H
#define NATIVESCANNER_LCDDISPLAY_H


#include "../src/Beacon.h"
#include "../src2/ScannerView.h"
#include "../src2/StatusInformation.h"

/**
 * A class that configures the wiringPi lcd api to work with a 4x20 lcd display.
 */
class LcdDisplay : public ScannerView {
private:
    int nCols;
    int lcdHandle;

    LcdDisplay() { }

public:
    /**
     * Singleton accessor
     */
    static LcdDisplay *getLcdDisplayInstance();
    /**
     * Initialize the display to the given size
     */
    int init(int rows=4, int cols=20);
    /**
     * Erase the display
     */
    void clear();
    /**
     * Display a text string starting at the given position. This will wrap around if the string is greater
     * than the number of columns on the display.
     */
    void displayText(const string& text, int col, int row);
    /**
     * Display a time string as HH:MM:SS.ss
     */
    void displayTime(int64_t timeInMS, int col, int row);

    /**
     * Display the given beacon information
     */
    virtual void displayBeacon(const Beacon& beacon);
    /**
     * Display the given heartbeat beacon information
     */
    virtual void displayHeartbeat(const Beacon& beacon);
    /**
     * Display the given scanner status information
     */
    virtual void displayStatus(StatusInformation const &status);
};

#endif //NATIVESCANNER_LCDDISPLAY_H
