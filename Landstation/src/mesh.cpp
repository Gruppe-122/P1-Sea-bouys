#include "mesh.h"

#define LORA_CS 8
#define LORA_DIO1 14
#define LORA_RST 12
#define LORA_BUSY 13

meshalternativ* meshalternativ::instance = nullptr;

meshalternativ::meshalternativ() : initialized(false), id(0), receivedFlag(false), mod(nullptr), radio(nullptr)
{
    instance = this;
}

void meshalternativ::start_radio()
{
    mod = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
    radio = new SX1262(mod);

    meshLog.logln("Module and radio created", "INFO", true);
    
    int state = radio->begin(frequency);
    
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("Radio initialization failed (%d)\n", state);
        return;
    }

    meshLog.logln("Radio initialized successfully!", "INFO", true);
    
    radio->setDio2AsRfSwitch(true);
    radio->setBandwidth(bandwidth);
    radio->setSpreadingFactor(spreading_factor);
    radio->setCodingRate(5);
    radio->setPreambleLength(preamble_length);
    radio->setSyncWord(sync_word);
    radio->setOutputPower(power);
    radio->setCRC(true);
    radio->setPacketReceivedAction(setFlagReceive);

    int listenState = radio->startReceive();
    if (listenState == RADIOLIB_ERR_NONE)
    {
        meshLog.logln("LoRa listening started successfully!", "INFO", true);
    }
    else
    {
        meshLog.log("Failed to start listening, code ", "INFO", true);
        meshLog.logln(listenState, "INFO", false);
    }

    initialized = true;
}

void meshalternativ::send_data(const BuoyData& data)
{
    if (!initialized)
    {
        meshLog.logln("Radio not initialized!", "INFO", true);
        return;
    }

    bool flagSaved = receivedFlag;
    int state = radio->transmit((uint8_t*)&data, sizeof(BuoyData));
    receivedFlag = flagSaved;
    
    if (state == RADIOLIB_ERR_NONE)
    {
        meshLog.logln("Struct sent successfully!", "INFO", true);
    }
    else
    {
        meshLog.log("Struct transmission failed, code ", "INFO", true);
        meshLog.logln(state, "INFO", false);
    }

    radio->startReceive();
}

bool meshalternativ::receive_data(BuoyData& data)
{
    if (!initialized)
    {
        return false;
    }

    if (receivedFlag)
    {
        receivedFlag = false;
        
        int state = radio->readData((uint8_t*)&data, sizeof(BuoyData));
        
        if (state == RADIOLIB_ERR_NONE)
        {
            meshLog.logln("Struct received successfully!", "INFO", true);
            // Received Signal Strength Indicator
            meshLog.log("RSSI: ", "INFO", true);
            meshLog.log(radio->getRSSI(), "INFO", false);
            meshLog.logln(" dBm", "INFO", false);

            // Signal to noise ratio
            meshLog.log("SNR: ", "INFO", true);
            meshLog.log(radio->getSNR(), "INFO", false);
            meshLog.logln(" dB", "INFO", false);
            
            radio->startReceive();
            
            return true;
        }
        else
        {
            meshLog.log("Struct read failed, code ", "INFO", true);
            meshLog.logln(state, "INFO", false);
            
            radio->startReceive();

            return false;
        }
    }
    
    return false;
}

void meshalternativ::sleep_radio()
{
    if (initialized && radio != nullptr)
    {
        radio->sleep();
    }
}

void meshalternativ::setFlagReceive(void)
{
    if (instance != nullptr)
    {
        instance->receivedFlag = true;
    }
}

meshalternativ::~meshalternativ()
{
    if (initialized && radio != nullptr)
    {
        radio->sleep();
    }
    
    if (radio != nullptr)
    {
        delete radio;
        radio = nullptr;
    }
    
    if (mod != nullptr)
    {
        delete mod;
        mod = nullptr;
    }
}