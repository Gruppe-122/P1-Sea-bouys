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
#include "time.h"

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
#define LATITUDE 57.014734549353605
#define LONGITUDE 9.98581865579486
#define METERS_PER_DEGREE_LAT 11111.0
#define MAX_DISTANCE 30
#define uS_TO_S_FACTOR 1000000ULL
#define INTERVAL_MINUTES 4
#define WAKEUP_MINUTES_BEFORE 2

#define CURRENT_POWER_PIN 0
#define VOLTAGE_POWER_PIN 2

// Vars
double maxDistance = (MAX_DISTANCE * MAX_DISTANCE) / (METERS_PER_DEGREE_LAT * METERS_PER_DEGREE_LAT);

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
logger gpsLog = logger("GPS", "DEBUG");
logger mainLog = logger("MAIN", "INFO");

void syncTime(){
  mainLog.logln("started sync", "INFO", true);
  initGNSS(GPSSerial, GPSRX, GPSTX);
  readGNSS(&GNSSData, GPSSerial);
  syncTimeFromGPS(GNSSData.utc);
  mainLog.logln("time sync", "INFO", true);
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
  mainLog.logln(data.gps_latitude, 10, level, false);

  mainLog.log("gps_longitude: ", level, true);
  mainLog.logln(data.gps_longitude, 10, level, false);

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
  readGNSS(&GNSSData, GPSSerial); //will time out after 30 seconds
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

void sleepTime(unsigned long sec) {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 1);
  esp_sleep_enable_timer_wakeup((uint64_t)sec * uS_TO_S_FACTOR);
  Serial.flush();
  delay(50);
  esp_deep_sleep_start();
}

void isWitheldDataAlreadySent();
void sendingOrderForBuoys();

// saves data on RTC RAM so it is remembered from each sleep cycle
RTC_DATA_ATTR uint8_t accelerometerHit = 0;


int idCheck[BUOY_AMOUNT];
int receivedIDs = 0;
unsigned int buoySendDelay = 0;
unsigned long lastSentMessage = 0;
unsigned long sendDataTimer;
unsigned long milliseconds_until_interval;
bool sendDelay = false;
bool initialized = false;
bool alreadySentID = false;
bool alreadySentID2 = false;
bool sendOwnMessage = false;

void setup()
{
  Serial.begin(115200);

  // GPS
  delay(10000);
  syncTime();
  // Get time and date
  // We put our time into an int
  time_t now = time(NULL);
  // We now put the adress of this integer into a command to make it into a time struct. localtime requires a pointer
  struct tm *local_time = localtime(&now);
  int current_minute = local_time->tm_min;
  int current_second = local_time->tm_sec;
  // How many minutes have passed in the interval
  int remainder = current_minute % INTERVAL_MINUTES;
  // Change to how many minutes are left in the interval
  int minutes_to_next = INTERVAL_MINUTES - remainder;
  // Make amount of time left into milliseconds
  milliseconds_until_interval = ((minutes_to_next * 60) - current_second) * 1000;
  sendDataTimer = millis();
  Serial.print("Milliseconds until interval where it should send:");
  Serial.println(milliseconds_until_interval);


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

void loop() {
    // Wake up
    // initialized = 0 somewhere in wake up

  if(!initialized) {
    // Check accelerometer, if it was hit
    // If it was hit during sleep, it's gonna be 1
    // If it was hit two cycles ago, it'll be 2, 3 will be 3, and then it resets
    // Gives a general idea of how long it was hit last, in case a message fails to send
    collectSensorData();
    ownData.sent_from = 3;
    logBuoyData(ownData, "TEST");

    if (ownData.accelerometer_jerk != 0) {
      accelerometerHit = 1;
    }
    if (accelerometerHit > 0 && accelerometerHit <= 3) {
      ownData.accelerometer_jerk = accelerometerHit;
      accelerometerHit++;
    }
    else if (accelerometerHit > 4) {
      ownData.accelerometer_jerk = 0;
      accelerometerHit = 0;
    }
    else if (accelerometerHit != 0) {
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
        double lat_diff_meters = (convertTodegrees(ownData.gps_latitude) - LATITUDE);

        // Longitude degree per meter changes from how far up you are, use original location to get a guesstimate
        double lon_diff_meters = (convertTodegrees(ownData.gps_longitude) - LONGITUDE) * cos(LONGITUDE * PI / 180.0);

      // Pythagoras to figure out if it's far away
      double distance = (lon_diff_meters * lon_diff_meters) + (lat_diff_meters * lat_diff_meters);


        // If it's over 30 meters away (30*30 = 900) - An extra check in case the buoy doesn't know it's out of its position
      if (distance > maxDistance) {
          gpsTries++;
          Serial.println("GPS Out of range! This message will appear 3 times if this is out of range!");
      }
      else {
          break;
      }
    }
  
    // Start ALARM MODE if GPS was out of range 3 times
    if (gpsTries >= 3) {
      ownData.alarm = true;
    }
    else {
      Serial.println("GPS Buoy in its right place!");
    }
    initialized = true;
    lastSentMessage = millis();
  }

  if (milliseconds_until_interval + (BUOY_ID * 1000) < millis() - sendDataTimer && !sendOwnMessage) 
  {
    Serial.println("Message has been sent!");
    buoy.send_data(ownData);
    idCheck[0] = BUOY_ID;
    receivedIDs++;
    lastSentMessage = millis();
    sendOwnMessage = true;
  }

  
  // Start listening loop!
  if (buoy.receive_data(receivedData)) {
      alreadySentID = false;

    // Amount of IDs received, check if already in array or if it hasn't received anything new
    for(int i=0; i<receivedIDs; i++){ 
      if(receivedData.buoy_number == idCheck[i]){
        alreadySentID = true;
      }
    }
    if (!alreadySentID) {
      Serial.println("Received buoy ID: ");
      Serial.println(receivedData.buoy_number);
    }
    sendingOrderForBuoys();
  }


  // Check if witholding ID sent from 2 buoys away is already sent through a closer buoy in the meantime
  alreadySentID2 = false;
  isWitheldDataAlreadySent();

  // After a certain amount of time, check how long it's been awake
  // After 30 seconds of being awake, sleep
  if (10000 < millis() - lastSentMessage && sendOwnMessage) {
    // We put our time into an int
    time_t now = time(NULL);
    // We now put the adress of this integer into a command to make it into a time struct. localtime requires a pointer
    struct tm *local_time = localtime(&now);
    int current_minute = local_time->tm_min;
    int current_second = local_time->tm_sec;
    int remainder = current_minute % INTERVAL_MINUTES;
    int seconds_to_next_full_interval = ((INTERVAL_MINUTES - remainder) * 60) - current_second;
    //int seconds_to_next_full_interval = time_to_current_interval + (INTERVAL_MINUTES * 60);
    int sleep_duration = seconds_to_next_full_interval - (WAKEUP_MINUTES_BEFORE * 60);
    if (sleep_duration <= 0) {
    //  sleep_duration = seconds_to_next_full_interval + (INTERVAL_MINUTES * 60) - (WAKEUP_MINUTES_BEFORE * 60);
      if (sleep_duration <= 0) {
        sleep_duration = 1;
      }
    }
    Serial.print("Going to sleep for ");
    Serial.print(sleep_duration);
    Serial.println(" seconds!");
    buoy.sleep_radio();
    sleepTime(sleep_duration);
  }
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
