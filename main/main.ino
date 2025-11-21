// -----------------------------------------------------------------------
// | Main sketch Bouy project                                            |
// | Board: Heltec WiFi LoRa 32(V3)                                      |
// | Pin layout:                                                         |
// |  ACS712  -> pin x                                                   |
// |                                                                     |
// | Make sure ESP32 is properly inserted in breadboard or DC_OFFSET     |
// | will increase                                                       |
// -----------------------------------------------------------------------

#include "src/accel.h"
#include "src/current.h"
#include "src/gps_parser.h"
#include "src/volt.h"
#include "src/mesh.h"

//definitions
#define R1 1000.0        //Resistor value in voltagedivider circuit
#define R2 1000.0        //Resistor value in voltagedivider circuit
#define REF_VOLTAGE 1100 //ESP32 reference voltage for calibration.
#define VOLT_PIN 7
#define ADC_RESOLUTION 12

#define CURRENTSENSOR_PIN 6
#define DC_OFFSET 2500   //voltage offset from currentsensor module

#define ADC_N_SAMPLES 20          //amount of ADC signals to base voltage reading on
#define ADC_SAMPLING_FREQUENCY 20 //time between taking ADC value ms

#define GPSRX 19
#define GPSTX 20
#define GPSSerial Serial2
#define BUOY_ID 3 
#define BUOY_AMOUNT 4

//Variables
nmeaData GNSSData;
meshalternativ buoy;
BuoyData ownData;
BuoyData receivedData;

//Objects
Volt battery(VOLT_PIN, R1, R2, ADC_11db, ADC_RESOLUTION);
CurrentSensor current(CURRENTSENSOR_PIN, DC_OFFSET);

void setup()
{
  delay(1000);
  Serial.begin(115200);
  buoy.start_radio();
  initGNSS(GPSSerial, GPSRX, GPSTX);
  battery.set_sampling(ADC_N_SAMPLES, ADC_SAMPLING_FREQUENCY);
  current.set_sampling(ADC_N_SAMPLES, ADC_SAMPLING_FREQUENCY);
  ownData.buoy_number = BUOY_ID;
  current.begin();
}

  int idCheck[BUOY_AMOUNT];
  int receivedIDs = 0;


void loop() {
    // Wake up
    // initialized = 0 somewhere in wake up
  int initialized = 0;
    if(initialized == 0) {
      // Check accelerometer
      // if (accelerometer() == 0){
      //   ownData.accelerometer_jerk = true;
      // };
      // Check battery
      //   ownData.battery_voltage = 0.0;
      // Check GPS & time
      //   Use time from GPS to calibrate here
      // readGNSS(&GNSSData, GPSSerial);
      // PrintGPSData(GNSSData);
      //   ownData.gps_latitude = 0.0;
      //   ownData.gps_longitude = 0.0;
      // Check Lamp (UTC)
      //   ownData.lamp_current = true;
      // Use time to check when to send (maybe a delay on found time vs expected time sequence starts)
      // Comment above only works if we can get milliseconds; system needs changing if not
      // Buoy ID in seconds + 0.5 seconds before sending
      delay((BUOY_ID*1000)+500);
      buoy.send_data(ownData);
      // Adding own buoy to the array of sent bouys
      idCheck[0] = BUOY_ID;
      receivedIDs++;
      // Create a struct, get data and start listening again
      initialized = 1;
    }

  buoy.receive_data(receivedData);
  bool alreadySent;
    for(int i=0; i<receivedIDs; i++){ // Amount of IDs received, check if already in array
      if(receivedData.buoy_number == idCheck[i]){
        alreadySent = true;
      }
    }
          // If it's from a buoy it hasn't gotten info from before, and it's maximum 3 buoys above my own ID
      if(alreadySent = false && BUOY_ID < receivedData.sent_from < BUOY_ID + 4){
          // If it's 1 buoy above, don't delay. Otherwise, delay with +0,6 sekunder pr afstand væk
        int amountAway = receivedData.sent_from - BUOY_ID - 1; 
        amountAway = amountAway*600;
        delay(amountAway);
        // Take buoy number, put into idCheck with received IDs number, add a new received ID for the next buoy
        idCheck[receivedIDs] = receivedData.buoy_number;
        receivedIDs++;
        // Send data onwards
        receivedData.sent_from = BUOY_ID;
        buoy.send_data(receivedData);
      }
         
  delay(2000);
  // After a certain amount of time, check how long it's been awake
  // Then GoToSleep
  
}

