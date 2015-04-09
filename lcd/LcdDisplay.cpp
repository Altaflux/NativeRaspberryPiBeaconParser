//
// Created by Scott Stark on 4/7/15.
//

#include "LcdDisplay.h"
#include <wiringPi.h>
#include <lcd.h>

void LcdDisplay::displayBeacon(const Beacon &beacon) {
    char tmp[80];
    sprintf(tmp, "Beacon(%d), rssi=%d", beacon.getMinor(), beacon.getRssi());
    lcdPosition(lcdHandle, 0, 0) ;
    lcdPuts(lcdHandle, tmp);
    sprintf(tmp, "Hello Scott");
    lcdPosition(lcdHandle, 0, 1) ;
    lcdPuts(lcdHandle, tmp) ;
}

void LcdDisplay::displayText(const string &text, int col, int row) {
    lcdPosition(lcdHandle, row, col);
    lcdPuts(lcdHandle, text.c_str());
}

int LcdDisplay::init(int rows, int cols) {
    wiringPiSetup () ;

    lcdHandle = lcdInit (rows, cols, 4, 11,10, 4,5,6,7,0,0,0,0) ;
    if (lcdHandle < 0)
    {
        fprintf (stderr, "lcdInit failed\n") ;
        return -1 ;
    }
    return 0;
}
