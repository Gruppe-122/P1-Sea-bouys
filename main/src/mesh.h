#ifndef MESH_H
#define MESH_H

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <vector>

struct BuoyData {
    uint8_t buoy_number;
    float battery_voltage;
    bool lamp_current;
    bool accelerometer_jerk;
    float gps_latitude;
    float gps_longitude;
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

    volatile bool receivedFlag;

    const float frequency = 868.1;
    const float bandwidth = 125.0;
    const int spreading_factor = 7;
    const int coding_rate = 5;
    const int sync_word = 0x34;
    const float power = 17;
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