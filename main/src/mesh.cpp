#include "mesh.h"

// Define the LoRa control pins
#define LORA_CS 8
#define LORA_DIO1 14
#define LORA_RST 12
#define LORA_BUSY 13

static const int SAFETY_DELAY = 50;

// Used for setFlagReceive() to 
meshalternativ* meshalternativ::instance = nullptr;

meshalternativ::meshalternativ() : initialized(false), id(0), receivedFlag(false), mod(nullptr), radio(nullptr)
{
    instance = this;
}

void meshalternativ::start_radio()
{
  // I heap i stedet for stack, så det ikke forsvinder når funktionen forsvinder
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
    
    // Set DIO2 as RF switch
    radio->setDio2AsRfSwitch(true);
    
    radio->setBandwidth(bandwidth);
    radio->setSpreadingFactor(spreading_factor);
    radio->setCodingRate(5);
    radio->setPreambleLength(preamble_length);
    radio->setSyncWord(sync_word);
    radio->setOutputPower(power);
    
    // Enable CRC
    radio->setCRC(true);

    // Set the callback for received packets
    radio->setPacketReceivedAction(setFlagReceive);

    // Start listening
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

    uint8_t buffer[sizeof(BuoyData)];
    memcpy(buffer, &data, sizeof(BuoyData));

    // Save data of transmission to ignore own message pings receivedFlag (Doesn't save its own message)
    bool hadPendingReceive = receivedFlag;
    
    int state = radio->transmit(buffer, sizeof(BuoyData));
    delay(SAFETY_DELAY);
    
    receivedFlag = hadPendingReceive;
    
    // Debug down from here
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("Message sent successfully!"));
        Serial.printf("sent %d bytes\n", sizeof(BuoyData));
    }
    else
    {
        Serial.print(F("Transmission failed, code "));
        Serial.println(state);
    }


    // Return to receive mode after transmitting
    radio->startReceive();
}

bool meshalternativ::receive_data(BuoyData& data)
{
    if (!initialized)
    {
        return false;  // Don't print error every loop
    }

    // Check if a packet was received
    if (receivedFlag)
    {
        receivedFlag = false; // Reset flag
        uint8_t buffer[256];
        int state = radio->readData(buffer, sizeof(buffer));
        
        if (state == RADIOLIB_ERR_NONE)
        {
            if (radio->getPacketLength() == sizeof(BuoyData))
            {

                memcpy(&data, buffer, sizeof(BuoyData));

                Serial.println(F("Struct received successfully!"));
                Serial.printf("Received %d bytes\n", sizeof(BuoyData));
            
                // Print signal quality
                Serial.print(F("RSSI: "));
                Serial.print(radio->getRSSI());
                Serial.println(F(" dBm"));
                
                Serial.print(F("SNR: "));
                Serial.print(radio->getSNR());
                Serial.println(F(" dB"));
            
                // Return to receive mode
                radio->startReceive();
            
                return true;
            }
            else
            {
                Serial.printf("Size mismatch: expected %d, got %d bytes\n", sizeof(BuoyData), radio->getPacketLength());
                radio->startReceive();
                return false;
            }
        }
        else
        {
            Serial.print(F("Read failed, code "));
            Serial.println(state);
            
            // Return to receive mode
            radio->startReceive();
            
            return false;
        }
    }
    
    // No message received
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