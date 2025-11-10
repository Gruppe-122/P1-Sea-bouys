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

public:
    meshtastic();
    ~meshtastic();
    
    start_radio();
    sleep_radio();
    init_mesh();
};

#endif