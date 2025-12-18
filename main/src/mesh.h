#ifndef MESH_H
#define MESH_H

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <vector>
#include "log.h"

extern logger meshLog;

// Add the BuoyData struct definition
struct BuoyData
{
    bool alarm;
    uint8_t buoy_number;
    uint8_t sent_from;
    uint8_t battery_voltage;
    double gps_latitude;
    double gps_longitude;
    uint8_t accelerometer_jerk;
    bool lamp_current;
};

class meshalternativ
{
private:
    Module* mod;
    SX1262* radio;
    
    bool initialized;
    int id;
    static void setFlagReceive(void);
    static meshalternativ* instance;

    // volatile because we need the compiler to know the value can change unexpectedly from an interrupt
    volatile bool receivedFlag;

    const float frequency = 868.1;
    // Bandwidth is the width of the frequency channel
    // Wider equals higher data rate but reduces sensitivity and maximum range
    const float bandwidth = 125.0;
    // How long the "chirps" are. Going up in factor 2x's time everytime
    const int spreading_factor = 7;
    // How many times a byte is added to correct transmission error. 4/5 here, as in 4 bytes, third is a correction byte
    const int coding_rate = 5;
    // Transmitter and receiver needs same sync word to communicate
    const int sync_word = 0x24;
    // Max power is 14 dBm in this frequency spectrum
    const float power = 12;
    // Amount of symbols used to correct frequency
    const int preamble_length = 8;

public:
    meshalternativ();
    ~meshalternativ();

    void start_radio();
    void sleep_radio();
    void send_data(const BuoyData& data);
    bool receive_data(BuoyData& data);
};

#endif