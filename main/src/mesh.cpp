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
    
    Serial.println("Module and radio created");
    
    int state = radio->begin(frequency);
    
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("Radio initialization failed (%d)\n", state);
        return;
    }

    Serial.println("Radio initialized successfully!");
    
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
        Serial.println(F("LoRa listening started successfully!"));
    }
    else
    {
        Serial.print(F("Failed to start listening, code "));
        Serial.println(listenState);
    }

    initialized = true;
}

void meshalternativ::send_data(const BuoyData& data)
{
    if (!initialized)
    {
        Serial.println("Radio not initialized!");
        return;
    }

    int state = radio->transmit((uint8_t*)&data, sizeof(BuoyData));
    
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("Struct sent successfully!"));
    }
    else
    {
        Serial.print(F("Struct transmission failed, code "));
        Serial.println(state);
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
            Serial.println(F("Struct received successfully!"));
            
            Serial.print(F("RSSI: "));
            Serial.print(radio->getRSSI());
            Serial.println(F(" dBm"));
            
            Serial.print(F("SNR: "));
            Serial.print(radio->getSNR());
            Serial.println(F(" dB"));
            
            radio->startReceive();
            
            return true;
        }
        else
        {
            Serial.print(F("Struct read failed, code "));
            Serial.println(state);
            
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