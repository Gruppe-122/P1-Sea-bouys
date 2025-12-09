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

#define CURRENT_POWER_PIN 0
#define VOLTAGE_POWER_PIN 2

// Structs
nmeaData GNSSData;
BuoyData ownData;
BuoyData receivedData;

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

void loop()
{
  // Wake up
  // initialized = 0 somewhere in wake up
  int initialized = 0;
  if (initialized == 0)
  {
    collectSensorData();
    logBuoyData(ownData, "TEST");

    // Use time to check when to send (maybe a delay on found time vs expected time sequence starts)
    // Comment above only works if we can get milliseconds; system needs changing if not
    // Buoy ID in seconds + 0.5 seconds before sending
    delay((BUOY_ID * 1000) + 500);
    buoy.send_data(ownData);

    // Adding own buoy to the array of sent bouys
    idCheck[0] = BUOY_ID;
    receivedIDs++;

    // Create a struct, get data and start listening again
    initialized = 1;
  }

  buoy.receive_data(receivedData);
  bool alreadySent;

  // Amount of IDs received, check if already in array
  for (int i = 0; i < receivedIDs; i++)
  {
    if (receivedData.buoy_number == idCheck[i])
    {
      alreadySent = true;
    }
    }

    // If it's from a buoy it hasn't gotten info from before, and it's maximum 3 buoys above my own ID
    if (alreadySent = false && BUOY_ID < receivedData.sent_from < BUOY_ID + 4)
    {
      // If it's 1 buoy above, don't delay. Otherwise, delay with +0,6 sekunder pr afstand væk
      int amountAway = receivedData.sent_from - BUOY_ID - 1;
      amountAway = amountAway * 600;
      delay(amountAway);

      // Take buoy number, put into idCheck with received IDs number, add a new received ID for the next buoy
      idCheck[receivedIDs] = receivedData.buoy_number;
      receivedIDs++;

      // Send data onwards
      receivedData.sent_from = BUOY_ID;
      buoy.send_data(receivedData);

      delay(300);
      // After a certain amount of time, check how long it's been awake
      // Then GoToSleep
    }
  }
