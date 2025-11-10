#ifndef MESH_H
#define MESH_H

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <vector>

class meshtastic
{
private:
    SX1262 radio;
    
    bool initialized;
    int id;

    const float frequency = 868.0;
    const float bandwidth = 125.0;
    const int spreading_factor = 7;
    const int coding_rate = 7;
    const int sync_word = 0x34;
    const float power = 20;
    const int preamble_length = 8;
    const int gain = 0;
    const bool TCXO = false;

public:
    meshtastic();
    ~meshtastic();

    start_radio();
    sleep_radio();
    init_mesh();
};

#endif