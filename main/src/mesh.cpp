#include "mesh.h"

meshtastic::meshtastic()
{
    radio = new Module(8, 14, 12, 13);
    int id = 0;
};

meshtastic::start_radio()
{
    // freq, bw, sf, cr, syncWord, power, preambleLen, gain, useTCXO
    int state = radio.begin(
        frequency,        // frequency in MHz
        bandwidth,        // bandwidth in kHz (7.8–500)
        spreading_factor, // spreading factor (6–12)
        coding_rate,      // coding rate denominator (5–8)
        sync_word,        // sync word (0x34 public, 0x12 private)
        power,            // power in dBm
        preamble_length,  // preamble length
        gain,             // gain (0 = auto)
        TCXO              // TCXO enabled
    );

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("failed (%d)\n", state);
    }

    // Heltec V3 routes DIO2 to the RF switch — enable it
    radio.setDio2AsRfSwitch(true);

    // QoL
    radio.setCRC(true);
    radio.setLowDataRateOptimize(); // auto when needed

    initialized = true;
    return true;
}

meshtastic::init_mesh(){
    // start the mesh

};

meshtastic::sleep_radio()
{
    if (initialized)
    {
        SPI.end();
        radio.sleep();
    }
}

meshtastic::~meshtastic()
{
    if (initialized)
    {
        SPI.end();
        radio.sleep();
        delete radio;
    }
}