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
#include "src/log.h"

// definitions
#define R1 1000.0        // Resistor value in voltagedivider circuit
#define R2 1000.0        // Resistor value in voltagedivider circuit
#define REF_VOLTAGE 1100 // ESP32 reference voltage for calibration.
#define VOLT_PIN 4
#define ADC_RESOLUTION 12

#define CURRENTSENSOR_PIN 3
#define DC_OFFSET 2500 // voltage offset from currentsensor module

#define ADC_N_SAMPLES 20          // amount of ADC signals to base voltage reading on
#define ADC_SAMPLING_FREQUENCY 20 // time between taking ADC value ms

#define GPSRX 19
#define GPSTX 20
#define GPSSerial Serial2
#define BUOY_ID 3
#define BUOY_AMOUNT 4
#define LATITUDE 57.055533
#define LONGITUDE 9.925497
#define METERS_PER_DEGREE_LAT 111120.0

#define CURRENT_POWER_PIN 0
#define VOLTAGE_POWER_PIN 2

// Structs
nmeaData GNSSData;
BuoyData ownData;
BuoyData receivedData;
BuoyData withholdingReceivedData;

// Objects
Volt battery(VOLT_PIN, R1, R2);
CurrentSensor current(CURRENTSENSOR_PIN, DC_OFFSET);
meshalternativ buoy;

// logs
logger accelLog = logger("ACCELOMETER", "INFO");
logger currentLog = logger("CURRENT", "INFO");
logger voltLog = logger("VOLT", "INFO");
logger meshLog = logger("MESH", "INFO");
logger gpsLog = logger("GPS", "INFO");
logger mainLog = logger("MAIN", "INFO");

unsigned long lastRun = 0;
const unsigned long interval = 10UL * 1000UL; // 1 minut
void syncTime(){
  mainLog.logln("started sync", "INFO", true);
  unsigned long now = millis();
  if (now - lastRun >= interval)
  {
    lastRun = now;
    readGNSS(&GNSSData, GPSSerial);
    syncTimeFromGPS(GNSSData.utc);
    mainLog.logln("time sync", "INFO", true);
  }
}

void logBuoyData(const BuoyData &data, const char *level)
{
  mainLog.log("buoy_number: ", level, true);
  mainLog.logln(data.buoy_number, level, false);

  mainLog.log("sent_from: ", level, true);
  mainLog.logln(data.sent_from, level, false);

  mainLog.log("battery_voltage: ", level, true);
  mainLog.logln(data.battery_voltage, level, false);

  mainLog.log("gps_latitude: ", level, true);
  mainLog.logln(data.gps_latitude, level, false);

  mainLog.log("gps_longitude: ", level, true);
  mainLog.logln(data.gps_longitude, level, false);

  mainLog.log("accelerometer_jerk: ", level, true);
  mainLog.logln(data.accelerometer_jerk, level, false);

  mainLog.log("lamp_current: ", level, true);
  mainLog.logln(data.lamp_current, level, false);
}

void collectSensorData()
{
  mainLog.logln("collect sensors data", "INFO", true);

  // pin power setup
  digitalWrite(CURRENT_POWER_PIN, HIGH);
  digitalWrite(VOLTAGE_POWER_PIN, HIGH);

  // wait for hardware to be ready
  delay(100);

  // Accelometer
  ownData.accelerometer_jerk = accelerometer();

  // Voltage
  int avg_ADC = 0;
  for (int i = 0; i < 100; i++)
  {
    avg_ADC += battery.moving_avg_ADC();
  }
  ownData.battery_voltage = battery.ADC_to_mV(avg_ADC);

  // GPS
  readGNSS(&GNSSData, GPSSerial); //will time out after 6 seconds
  //TODO: check if valid
  ownData.gps_latitude = GNSSData.lat;
  ownData.gps_longitude = GNSSData.lon;

  // UTC Missing
  bool isLampCurrent = current.measure_current_mA() > 0 ? true : false;
  ownData.lamp_current = isLampCurrent;

  // pin power clean up
  digitalWrite(CURRENT_POWER_PIN, LOW);
  digitalWrite(VOLTAGE_POWER_PIN, LOW);
}


// saves data on RTC RAM so it is remembered from each sleep cycle
RTC_DATA_ATTR uint8_t accelerometerHit;

void isWitheldDataAlreadySent();
void sendingOrderForBuoys();

void setup()
{
  delay(1000);
  Serial.begin(115200);

  // GPS
  initGNSS(GPSSerial, GPSRX, GPSTX);
  syncTime();

  // voltage measurements
  pinMode(VOLT_PIN, INPUT);
  pinMode(VOLTAGE_POWER_PIN, OUTPUT);
  digitalWrite(VOLTAGE_POWER_PIN, LOW);

  // mesh
  buoy.start_radio();
  ownData.buoy_number = BUOY_ID;

  // Current sensor
  pinMode(CURRENTSENSOR_PIN, INPUT);
  current.set_sampling(ADC_N_SAMPLES, ADC_SAMPLING_FREQUENCY);
  current.begin();
  pinMode(CURRENT_POWER_PIN, OUTPUT);
  digitalWrite(CURRENT_POWER_PIN, LOW);

  // Accelometer
  accelSetup();
  calibrate();
}

int idCheck[BUOY_AMOUNT];
int receivedIDs = 0;
unsigned int buoySendDelay = 0;
unsigned long lastSentMessage = 0;
bool sendDelay = false;
bool initialized = false;
bool alreadySentID = false;
bool alreadySentID2 = false;

void loop() {
    // Wake up
    // initialized = 0 somewhere in wake up

  if(!initialized) {
    // Check accelerometer, if it was hit
    // If it was hit during sleep, it's gonna be 1
    // If it was hit two cycles ago, it'll be 2, 3 will be 3, and then it resets
    // Gives a general idea of how long it was hit last, in case a message fails to send

    if (accelerometer() != 0) {
      accelerometerHit = 1;
    }
    if (accelerometerHit > 0 && accelerometerHit <= 3) {
      ownData.accelerometer_jerk = accelerometerHit;
      accelerometerHit++;
    }
    else if (accelerometerHit == 4) {
      ownData.accelerometer_jerk = 0;
      accelerometerHit = 0;
    }
    else if (accelerometer != 0) {
      Serial.println("Error! 'accelerometerHit' was not within the interval [0,4]");
    }
    // Check battery
    //   ownData.battery_voltage = 0.0;
    // Check Lamp (UTC)
    //   ownData.lamp_current = true;

    // GPS, check position, send 3 tries
    uint8_t gpsTries = 0;
    while (gpsTries < 3) {

        // TJEK GPS HER
        // readGNSS(&GNSSData, GPSSerial);
        //   ownData.gps_latitude = 0.0;
        //   ownData.gps_longitude = 0.0;
        // PrintGPSData(GNSSData);

        // Original position to GPS position
        double lat_diff_meters = (ownData.gps_latitude - LATITUDE) * METERS_PER_DEGREE_LAT;

        // Longitude degree per meter changes from how far up you are, use original location to get a guesstimate
        double lon_diff_meters = (ownData.gps_longitude - LONGITUDE) * metersPerDegreeLon(LONGITUDE);

        // Pythagoras to figure out if it's far away
        double distance = (lon_diff_meters * lon_diff_meters) + (lat_diff_meters * lat_diff_meters);


        // If it's over 30 meters away (30*30 = 900) - An extra check in case the buoy doesn't know it's out of its position
      if (distance > 900.0) {
          gpsTries++;
      }
      else {
          break;
      }
    }
  
    // Start ALARM MODE if GPS was out of range 3 times
    if (gpsTries >= 3) {
        ownData.alarm = true;
    }
    // Make a delay here based on TIME until when you want to send (Specific time of day + ID)
    delay(BUOY_ID*1000);

    buoy.send_data(ownData);
    // Adding own buoy to the array of sent bouys
    idCheck[0] = BUOY_ID;
    receivedIDs++;
    initialized = true;
    lastSentMessage = millis();
    collectSensorData();
    logBuoyData(ownData, "TEST");
  }

  
  // Start listening loop!
  while (30000 > millis() - lastSentMessage) {
    if (buoy.receive_data(receivedData)) {
      alreadySentID = false;


      // Amount of IDs received, check if already in array or if it hasn't received anything new
      for(int i=0; i<receivedIDs; i++){ 
        if(receivedData.buoy_number == idCheck[i]){
          alreadySentID = true;
        }
      }
      sendingOrderForBuoys();
      }
    // Check if witholding ID sent from 2 buoys away is already sent through a closer buoy in the meantime
    alreadySentID2 = false;
    isWitheldDataAlreadySent();
  }

  

  // ALARM MODE! BUOY IS NOT IN LOCATION
  // NOT allowed to go beyond Duty Cycle of 1% transmission time of an hour, that is 36 seconds of total transmission time.
  // Remember, all the other buoys repeating the message will also be repeating higher ID buoy messages.



  // After a certain amount of time, check how long it's been awake
  // After 30 seconds of being awake, sleep
  buoy.sleep_radio();
  esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);  // 10 seconds as an example of sleep instead of WiFi
  esp_deep_sleep_start();
}

float metersPerDegreeLon(float lon) {
  // cos uses cosine with radians, so we change degrees to radians
  float metersPerDegreeLon = METERS_PER_DEGREE_LAT * cos(lon * PI / 180.0);
  return metersPerDegreeLon;
}

void sendingOrderForBuoys() {
    // If it's from a buoy further away (next to it or once removed). Odds of both disappearing is small, no need to complicate the system.
  if(alreadySentID == false && BUOY_ID < receivedData.sent_from && receivedData.sent_from < (BUOY_ID + 3)){
    // If it's from an ID+1 of its own, don't delay.
    if(receivedData.sent_from == (BUOY_ID + 1)){
      
      // Take buoy number, put into idCheck with received IDs number, add a new received ID for the next buoy
      // also if statement to make sure it doesnt crash if you receive ID above what buoy was meant to
      if (receivedIDs < BUOY_AMOUNT) {
        idCheck[receivedIDs] = receivedData.buoy_number;
        receivedIDs++;
      }
      // Send data onwards
      receivedData.sent_from = BUOY_ID;
      buoy.send_data(receivedData);
      lastSentMessage = millis();
    }
    // If it's from an ID+2 of its own, start timer to delay sending it.
    if(receivedData.sent_from == (BUOY_ID + 2)){
      withholdingReceivedData = receivedData;
      // Delay for when it should send so it doesn't interrupt with the other ID, and also can hear if another ID
      buoySendDelay = millis();
      sendDelay = true;
    }
  }
}

void isWitheldDataAlreadySent() {
  if (600 < millis() - buoySendDelay && sendDelay == true && withholdingReceivedData.buoy_number != 0) {
    for(int i=0; i<receivedIDs; i++){
      if(withholdingReceivedData.buoy_number == idCheck[i]){
        alreadySentID2 = true;
        withholdingReceivedData.buoy_number = 0;
        sendDelay = false;
      }
    }
    if(alreadySentID2 == false){
      // Save ID in sent buoys list, also if statement to make sure it doesnt crash if you receive ID above what buoy was meant to
      if (receivedIDs < BUOY_AMOUNT) {
        idCheck[receivedIDs] = withholdingReceivedData.buoy_number;
        receivedIDs++;
      }
      // Send data onwards
      withholdingReceivedData.sent_from = BUOY_ID;
      buoy.send_data(withholdingReceivedData);
      withholdingReceivedData.buoy_number = 0;
      lastSentMessage = millis();
      sendDelay = false;
    }
  }
}
