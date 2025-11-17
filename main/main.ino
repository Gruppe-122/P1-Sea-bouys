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
//Variables

//Objects
Volt battery(VOLT_PIN, REF_VOLTAGE, R1, R2, ADC_ATTEN_DB_11, ADC_RESOLUTION);
CurrentSensor current(CURRENTSENSOR_PIN, DC_OFFSET);

void setup() {
  delay(1000);
  Serial.begin(115200);
  initGNSS(GPSSerial, GPSRX, GPSTX);
  battery.set_sampling(ADC_N_SAMPLES, ADC_SAMPLING_FREQUENCY);
  current.set_sampling(ADC_N_SAMPLES, ADC_SAMPLING_FREQUENCY);
  current.begin();
}

void loop() {
  
}
