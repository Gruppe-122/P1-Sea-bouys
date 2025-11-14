#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPSPlus.h>

// Wiring: GPS TXD -> ESP32 RX_PIN, GPS RXD <- ESP32 TX_PIN
constexpr int RX_PIN   = 6;            // adjust to your board if needed
constexpr int TX_PIN   = 5;
constexpr long GPS_BAUD = 9600;        // some modules use 115200

class myGps
{
public:
    myGps();
    bool sendPMTKAndWaitAck(const char* cmd, int expectedCmdId, uint16_t timeoutMs);
    void run();
    void sleep();
    void wake();
    void ping();

private:
    HardwareSerial gpsSerial;  // UART2 on ESP32
    TinyGPSPlus    parser;        // TinyGPS++ parser
    unsigned long  lastReport   = 0;
    bool           announcedFix = false;
};

#endif